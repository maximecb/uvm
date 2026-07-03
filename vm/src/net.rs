// Simple blocking TCP networking API.
//
// Each open socket is identified by a small integer socket id handed out to
// the guest. Sockets come in two flavors:
//   - Listeners, created by net_listen and consumed by net_accept
//   - Streams, created by net_accept and used by net_read / net_write
//
// The API is blocking: net_accept blocks until a connection arrives and
// net_read blocks until data is available. UVM programs are expected to use
// the thread syscalls (thread_spawn / thread_join) to handle multiple
// connections concurrently, typically spawning one thread per connection.
//
// This subsystem deliberately does not spawn any threads of its own.
//
// Fallible calls report errors through signed return values, matching the
// POSIX read()/write() convention (and the file I/O syscalls): a byte count or
// socket id is non-negative, -1 signals an error, and net_read returns -2 to
// distinguish a timeout from a closed connection (0).

use std::collections::HashMap;
use std::io::{ErrorKind, Read, Write};
use std::net::{Shutdown, TcpListener, TcpStream};
use std::sync::{Arc, Mutex, OnceLock};
use std::thread::sleep;
use std::time::Duration;
use crate::vm::{Thread, Value};
use crate::constants::{NET_ERROR, NET_TIMEOUT};

/// How often net_accept wakes up to check whether its listening socket has
/// been closed. This bounds both the latency of accepting a new connection and
/// how quickly a blocked net_accept notices a net_close from another thread.
const ACCEPT_POLL_MS: u64 = 20;

/// An open socket, either a listening socket or a connected stream.
///
/// Handles are stored behind an Arc so that a blocking operation can clone the
/// handle, release the socket-table lock, and then block without holding it.
/// This keeps one blocked socket from stalling operations on other sockets,
/// and lets net_close shut down a stream to wake a thread blocked in net_read.
enum Socket
{
    Listener(Arc<TcpListener>),
    Stream(Arc<TcpStream>),
}

/// Global table of open sockets.
///
/// This is process-global rather than stored on the VM: sockets are shared
/// across guest threads (net_accept runs on one thread, the resulting socket
/// is typically handed to another), and keeping the table independent of the
/// VM lock avoids serializing socket operations against unrelated VM activity.
struct NetState
{
    /// Next socket id to assign. Starts at 1 so that 0 is never a valid id
    /// and can be returned to signal failure.
    next_id: u64,

    /// Map of open sockets by id
    sockets: HashMap<u64, Socket>,
}

impl Default for NetState
{
    fn default() -> Self
    {
        Self {
            next_id: 1,
            sockets: HashMap::new(),
        }
    }
}

impl NetState
{
    /// Insert a socket into the table and return its freshly assigned id
    fn add_socket(&mut self, socket: Socket) -> u64
    {
        let id = self.next_id;
        self.next_id += 1;
        self.sockets.insert(id, socket);
        id
    }
}

/// Get a handle to the global socket table, initializing it on first use
fn net_state() -> &'static Mutex<NetState>
{
    static NET_STATE: OnceLock<Mutex<NetState>> = OnceLock::new();
    NET_STATE.get_or_init(|| Mutex::new(NetState::default()))
}

/// Look up a listening socket by id, cloning out its handle.
/// Returns None if the id is unknown or refers to a stream.
fn get_listener(socket_id: u64) -> Option<Arc<TcpListener>>
{
    let state = net_state().lock().unwrap();
    match state.sockets.get(&socket_id) {
        Some(Socket::Listener(listener)) => Some(listener.clone()),
        _ => None,
    }
}

/// Look up a connected stream by id, cloning out its handle.
/// Returns None if the id is unknown or refers to a listening socket.
fn get_stream(socket_id: u64) -> Option<Arc<TcpStream>>
{
    let state = net_state().lock().unwrap();
    match state.sockets.get(&socket_id) {
        Some(Socket::Stream(stream)) => Some(stream.clone()),
        _ => None,
    }
}

/// Whether a listening socket with this id is still open.
/// net_accept polls this so it can bail out once net_close removes the socket.
fn listener_present(socket_id: u64) -> bool
{
    let state = net_state().lock().unwrap();
    matches!(state.sockets.get(&socket_id), Some(Socket::Listener(_)))
}

/// Copy a string into a guest buffer, NUL-terminated and truncated to fit.
/// Does nothing if the buffer has no room for even the terminator.
fn write_cstr(thread: &mut Thread, buf_ptr: usize, buf_len: usize, s: &str)
{
    if buf_len == 0 {
        return;
    }

    let bytes = s.as_bytes();
    let num_copy = bytes.len().min(buf_len - 1);
    let buf: &mut [u8] = thread.get_heap_slice_mut(buf_ptr, buf_len);
    buf[..num_copy].copy_from_slice(&bytes[..num_copy]);
    buf[num_copy] = 0;
}

/// Open a listening TCP socket bound to the given address.
/// Returns a positive socket id, or -1 on failure.
/// i64 net_listen(const char* listen_addr)
pub fn net_listen(thread: &mut Thread, listen_addr: Value) -> Value
{
    let listen_addr = thread.get_heap_str(listen_addr.as_usize()).to_owned();

    let listener = match TcpListener::bind(&listen_addr) {
        Ok(listener) => listener,
        Err(_) => return Value::from(NET_ERROR),
    };

    // The listener is non-blocking so net_accept can poll it and stay
    // cancelable; net_accept sets each accepted stream back to blocking.
    if listener.set_nonblocking(true).is_err() {
        return Value::from(NET_ERROR);
    }

    let socket_id = net_state().lock().unwrap().add_socket(
        Socket::Listener(Arc::new(listener))
    );

    Value::from(socket_id as i64)
}

/// Block until an incoming connection is received on a listening socket, then
/// create a new stream socket for it. Writes the client's address into
/// client_addr_buf as a NUL-terminated string. Returns a positive socket id, or
/// -1 if the listening socket is closed (by net_close) or on error.
/// i64 net_accept(u64 socket_id, char* client_addr_buf, u64 addr_buf_len)
pub fn net_accept(
    thread: &mut Thread,
    socket_id: Value,
    client_addr_buf: Value,
    addr_buf_len: Value,
) -> Value
{
    let listen_id = socket_id.as_u64();
    let listener = match get_listener(listen_id) {
        Some(listener) => listener,
        None => return Value::from(NET_ERROR),
    };

    // std has no portable way to interrupt a blocking accept(), so the listener
    // is non-blocking and we poll it. Between attempts we check whether the
    // listening socket has been closed (removed from the table by net_close),
    // which lets another thread cancel this accept. The socket-table lock is
    // never held while we wait, so other sockets keep working.
    let (stream, peer_addr) = loop {
        match listener.accept() {
            Ok(conn) => break conn,

            Err(ref e) if e.kind() == ErrorKind::WouldBlock => {
                if !listener_present(listen_id) {
                    // The listening socket was closed: cancel the accept
                    return Value::from(NET_ERROR);
                }
                sleep(Duration::from_millis(ACCEPT_POLL_MS));
            }

            Err(_) => return Value::from(NET_ERROR),
        }
    };

    // The accepted stream inherits the listener's non-blocking flag on some
    // platforms; force it back to blocking so net_read behaves as documented.
    if stream.set_nonblocking(false).is_err() {
        return Value::from(NET_ERROR);
    }

    // Copy the client address into the guest-provided buffer
    write_cstr(
        thread,
        client_addr_buf.as_usize(),
        addr_buf_len.as_usize(),
        &peer_addr.to_string(),
    );

    let socket_id = net_state().lock().unwrap().add_socket(
        Socket::Stream(Arc::new(stream))
    );

    Value::from(socket_id as i64)
}

/// Read data from a socket into a guest buffer, blocking until at least one
/// byte is available (or the read timeout elapses, if one was set). Returns the
/// number of bytes read, 0 when the connection has been closed by the peer, -1
/// on error, or -2 if the read timed out.
/// i64 net_read(u64 socket_id, u8* buf_ptr, u64 buf_len)
pub fn net_read(
    thread: &mut Thread,
    socket_id: Value,
    buf_ptr: Value,
    buf_len: Value,
) -> Value
{
    let stream = match get_stream(socket_id.as_u64()) {
        Some(stream) => stream,
        None => return Value::from(NET_ERROR),
    };

    // Read straight into the guest heap. The heap only ever grows in place
    // (existing pages are never moved or unmapped), so this slice stays valid
    // for the duration of the blocking read.
    let buf: &mut [u8] = thread.get_heap_slice_mut(buf_ptr.as_usize(), buf_len.as_usize());

    // Read/Write are implemented for &TcpStream, so a shared handle suffices
    match (&*stream).read(buf) {
        // 0 bytes means the peer performed an orderly shutdown (EOF)
        Ok(num_bytes) => Value::from(num_bytes as i64),

        // A read timeout surfaces as WouldBlock on Unix and TimedOut on Windows
        Err(ref e) if e.kind() == ErrorKind::WouldBlock
                   || e.kind() == ErrorKind::TimedOut => Value::from(NET_TIMEOUT),

        Err(_) => Value::from(NET_ERROR),
    }
}

/// Write data from a guest buffer to a socket, blocking until the whole buffer
/// has been written. Returns the number of bytes written, or -1 if the
/// connection was lost.
/// i64 net_write(u64 socket_id, const u8* buf_ptr, u64 buf_len)
pub fn net_write(
    thread: &mut Thread,
    socket_id: Value,
    buf_ptr: Value,
    buf_len: Value,
) -> Value
{
    let stream = match get_stream(socket_id.as_u64()) {
        Some(stream) => stream,
        None => return Value::from(NET_ERROR),
    };

    let buf_len = buf_len.as_usize();
    let buf: &[u8] = thread.get_heap_slice_mut(buf_ptr.as_usize(), buf_len);

    match (&*stream).write_all(buf) {
        Ok(()) => Value::from(buf_len as i64),
        Err(_) => Value::from(NET_ERROR),
    }
}

/// Close an open socket. Returns 0 on success, or NET_ERROR if the socket id is
/// unknown. Closing a listening socket cancels a thread blocked in net_accept;
/// closing a connected stream wakes a thread blocked in net_read.
/// i64 net_close(u64 socket_id)
pub fn net_close(thread: &mut Thread, socket_id: Value) -> Value
{
    let socket = net_state().lock().unwrap().sockets.remove(&socket_id.as_u64());

    match socket {
        // Shut the stream down after releasing the table lock. This unblocks
        // any other thread parked in net_read on the same connection.
        Some(Socket::Stream(stream)) => {
            let _ = stream.shutdown(Shutdown::Both);
            Value::from(0_i64)
        }

        // A blocked net_accept notices the listener is gone on its next poll.
        Some(Socket::Listener(_)) => Value::from(0_i64),

        // Unknown socket id (never opened, or already closed)
        None => Value::from(NET_ERROR),
    }
}

/// Set the read timeout on a connected socket, in milliseconds. A timeout of 0
/// clears it, making subsequent reads block indefinitely. Returns 0 on success,
/// or -1 if the socket id is unknown or the option could not be set.
/// i64 net_set_read_timeout(u64 socket_id, u64 timeout_ms)
pub fn net_set_read_timeout(thread: &mut Thread, socket_id: Value, timeout_ms: Value) -> Value
{
    let stream = match get_stream(socket_id.as_u64()) {
        Some(stream) => stream,
        None => return Value::from(NET_ERROR),
    };

    // A zero Duration is rejected by the OS, so map 0 ms to "no timeout"
    let timeout_ms = timeout_ms.as_u64();
    let timeout = if timeout_ms == 0 {
        None
    } else {
        Some(Duration::from_millis(timeout_ms))
    };

    match stream.set_read_timeout(timeout) {
        Ok(()) => Value::from(0_i64),
        Err(_) => Value::from(NET_ERROR),
    }
}

#[cfg(test)]
mod tests
{
    use super::{net_listen, net_accept, net_read, net_write, net_close, net_set_read_timeout};
    use crate::asm::Assembler;
    use crate::vm::{VM, Thread, Value, MEM_BASE};
    use std::io::{Read, Write};
    use std::net::{TcpListener, TcpStream};
    use std::thread;
    use std::time::Duration;

    // Stand up a VM with a chunk of writable heap and return a Thread we can
    // use to drive the net syscalls directly.
    fn test_thread() -> Thread
    {
        let asm = Assembler::new();
        let prog = asm.parse_str("push 0; ret;").unwrap();
        let vm = VM::new(prog);
        // Map 64 KiB of heap starting at MEM_BASE for scratch buffers
        vm.lock().unwrap().grow_heap(MEM_BASE + 0x1_0000);
        VM::new_thread(&vm)
    }

    // Write a NUL-terminated string into the guest heap
    fn put_cstr(thread: &mut Thread, addr: usize, s: &str)
    {
        let bytes = s.as_bytes();
        let buf: &mut [u8] = thread.get_heap_slice_mut(addr, bytes.len() + 1);
        buf[..bytes.len()].copy_from_slice(bytes);
        buf[bytes.len()] = 0;
    }

    // Bind an ephemeral port, then release it so the guest can bind it.
    // There is a small reuse window, but it is fine for a local test.
    fn free_port() -> u16
    {
        let probe = TcpListener::bind("127.0.0.1:0").unwrap();
        let port = probe.local_addr().unwrap().port();
        port
    }

    #[test]
    fn listen_accept_read_write_close()
    {
        let port = free_port();
        let mut thread = test_thread();

        // net_listen("127.0.0.1:<port>")
        let addr_ptr = MEM_BASE;
        put_cstr(&mut thread, addr_ptr, &format!("127.0.0.1:{}", port));
        let listen = net_listen(&mut thread, Value::from(addr_ptr));
        assert!(listen.as_i64() > 0, "net_listen failed");

        // Client: connect, send a message, expect it echoed back
        let client = thread::spawn(move || {
            let mut stream = connect(port);
            stream.write_all(b"hello").unwrap();
            let mut buf = [0u8; 5];
            stream.read_exact(&mut buf).unwrap();
            assert_eq!(&buf, b"hello");
            // Dropping the stream closes the connection (EOF for the server)
        });

        // net_accept blocks until the client connects
        let addr_buf = MEM_BASE + 0x100;
        let conn = net_accept(
            &mut thread,
            listen,
            Value::from(addr_buf),
            Value::from(128_usize),
        );
        assert!(conn.as_i64() > 0, "net_accept failed");

        // The client address should have been written as a NUL-terminated string
        assert!(thread.get_heap_str(addr_buf).starts_with("127.0.0.1:"));

        // net_read the message, then echo it back with net_write
        let data_buf = MEM_BASE + 0x400;
        let n = net_read(&mut thread, conn, Value::from(data_buf), Value::from(64_usize)).as_i64();
        assert_eq!(n, 5);
        assert_eq!(&thread.get_heap_slice_mut::<u8>(data_buf, 5)[..], b"hello");

        let w = net_write(&mut thread, conn, Value::from(data_buf), Value::from(5_usize)).as_i64();
        assert_eq!(w, 5);

        // Once the client closes, the next read reports EOF as 0 bytes
        let eof = net_read(&mut thread, conn, Value::from(data_buf), Value::from(64_usize)).as_i64();
        assert_eq!(eof, 0);

        client.join().unwrap();

        net_close(&mut thread, conn);
        net_close(&mut thread, listen);
    }

    #[test]
    fn close_unblocks_blocked_reader()
    {
        let port = free_port();
        let mut thread = test_thread();

        let addr_ptr = MEM_BASE;
        put_cstr(&mut thread, addr_ptr, &format!("127.0.0.1:{}", port));
        let listen = net_listen(&mut thread, Value::from(addr_ptr));
        assert!(listen.as_i64() > 0);

        // A client that connects but never sends anything, keeping the
        // connection open so the server's read genuinely blocks.
        let client = thread::spawn(move || {
            let _stream = connect(port);
            thread::sleep(Duration::from_millis(500));
        });

        let conn = net_accept(
            &mut thread,
            listen,
            Value::from(MEM_BASE + 0x100),
            Value::from(128_usize),
        );
        assert!(conn.as_i64() > 0);

        // From another thread, close the connection while we block in net_read
        let vm = thread.vm.clone();
        let closer = thread::spawn(move || {
            thread::sleep(Duration::from_millis(50));
            let mut t = VM::new_thread(&vm);
            net_close(&mut t, conn);
        });

        // This blocks until the closer shuts the socket down. The read then
        // reports the end of the connection: either 0 (EOF, if it was already
        // blocked) or -1 (if the socket was removed before the read began).
        let n = net_read(
            &mut thread,
            conn,
            Value::from(MEM_BASE + 0x400),
            Value::from(64_usize),
        ).as_i64();
        assert!(n <= 0, "net_read should end once the socket is closed, got {}", n);

        closer.join().unwrap();
        client.join().unwrap();

        net_close(&mut thread, listen);
    }

    // net_close on a listening socket should cancel a thread blocked in
    // net_accept (this is the whole point of the non-blocking poll loop).
    #[test]
    fn close_listener_cancels_accept()
    {
        let port = free_port();
        let mut thread = test_thread();

        put_cstr(&mut thread, MEM_BASE, &format!("127.0.0.1:{}", port));
        let listen = net_listen(&mut thread, Value::from(MEM_BASE));
        assert!(listen.as_i64() > 0);

        // Close the listening socket shortly after we start blocking in accept.
        // No client ever connects, so accept only returns because of the close.
        let vm = thread.vm.clone();
        let closer = thread::spawn(move || {
            thread::sleep(Duration::from_millis(100));
            let mut t = VM::new_thread(&vm);
            net_close(&mut t, listen);
        });

        let res = net_accept(
            &mut thread,
            listen,
            Value::from(MEM_BASE + 0x100),
            Value::from(128_usize),
        ).as_i64();
        assert_eq!(res, -1, "net_accept should return -1 when the listener is closed");

        closer.join().unwrap();
    }

    // A read timeout should make net_read return -2 instead of blocking forever.
    #[test]
    fn read_timeout()
    {
        let port = free_port();
        let mut thread = test_thread();

        put_cstr(&mut thread, MEM_BASE, &format!("127.0.0.1:{}", port));
        let listen = net_listen(&mut thread, Value::from(MEM_BASE));
        assert!(listen.as_i64() > 0);

        // Client connects, waits, then sends. It stays silent long enough for
        // the server's first timed read to expire.
        let client = thread::spawn(move || {
            let mut stream = connect(port);
            thread::sleep(Duration::from_millis(200));
            stream.write_all(b"data").unwrap();
            thread::sleep(Duration::from_millis(200));
        });

        let conn = net_accept(
            &mut thread,
            listen,
            Value::from(MEM_BASE + 0x100),
            Value::from(128_usize),
        );
        assert!(conn.as_i64() > 0);

        // Arm a 50 ms read timeout
        let set = net_set_read_timeout(&mut thread, conn, Value::from(50_u64)).as_i64();
        assert_eq!(set, 0);

        // No data has arrived yet, so the first read times out
        let data_buf = MEM_BASE + 0x400;
        let first = net_read(&mut thread, conn, Value::from(data_buf), Value::from(64_usize)).as_i64();
        assert_eq!(first, -2, "expected a timeout, got {}", first);

        // Keep reading (each attempt times out) until the client's data arrives
        let mut got = 0;
        for _ in 0..100 {
            let n = net_read(&mut thread, conn, Value::from(data_buf), Value::from(64_usize)).as_i64();
            if n > 0 {
                got = n;
                break;
            }
            assert_eq!(n, -2, "expected timeout or data, got {}", n);
        }
        assert_eq!(got, 4);
        assert_eq!(&thread.get_heap_slice_mut::<u8>(data_buf, 4)[..], b"data");

        client.join().unwrap();
        net_close(&mut thread, conn);
        net_close(&mut thread, listen);
    }

    #[test]
    fn bad_socket_and_bind_failure()
    {
        let mut thread = test_thread();

        // Operations on an unknown socket id return -1 rather than panicking
        assert_eq!(net_read(&mut thread, Value::from(999_u64), Value::from(MEM_BASE), Value::from(1_usize)).as_i64(), -1);
        assert_eq!(net_write(&mut thread, Value::from(999_u64), Value::from(MEM_BASE), Value::from(1_usize)).as_i64(), -1);
        assert_eq!(net_accept(&mut thread, Value::from(999_u64), Value::from(MEM_BASE), Value::from(1_usize)).as_i64(), -1);
        assert_eq!(net_set_read_timeout(&mut thread, Value::from(999_u64), Value::from(10_u64)).as_i64(), -1);

        // Closing an unknown socket reports NET_ERROR (-1)
        assert_eq!(net_close(&mut thread, Value::from(999_u64)).as_i64(), -1);

        // Binding to a bogus address returns -1 instead of panicking
        put_cstr(&mut thread, MEM_BASE, "not a valid address");
        assert_eq!(net_listen(&mut thread, Value::from(MEM_BASE)).as_i64(), -1);
    }

    // Connect to the given loopback port, retrying until the server is listening
    fn connect(port: u16) -> TcpStream
    {
        for _ in 0..200 {
            if let Ok(stream) = TcpStream::connect(("127.0.0.1", port)) {
                return stream;
            }
            thread::sleep(Duration::from_millis(5));
        }
        panic!("could not connect to 127.0.0.1:{}", port);
    }
}
