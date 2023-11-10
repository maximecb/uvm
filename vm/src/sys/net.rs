use std::collections::{HashMap, VecDeque};
use std::slice;
use std::thread;
use std::net::{TcpListener, TcpStream};
use std::io::{self, Read, Write};
use std::sync::{Arc, Weak, Mutex};
use crate::vm::{VM, Value, ExitReason};

// State for the networking subsystem
pub struct NetState
{
    /// Next socket id to use
    next_id: u64,

    /// Map of open sockets
    sockets: HashMap<u64, Socket>,
}

impl Default for NetState
{
    fn default() -> Self
    {
        Self {
            // Start at FFFF so we can reserve the low values for error codes
            next_id: 0xFF_FF,
            sockets: HashMap::default(),
        }
    }
}

// State associated with a socket
enum Socket
{
    Listen {
        listener: TcpListener,

        /// Incoming connections
        incoming: VecDeque<TcpStream>,
    },

    Stream {
        stream: TcpStream,

        // Read buffer
        read_buf: Vec<u8>,
    }
}

/// TCP listening thread
fn listen_thread(
    vm_mutex: Weak<Mutex<VM>>,
    listener: TcpListener,
    socket_id: u64,
    on_new_conn: u64
)
{
    // Block until a connection can be accepted
    for result in listener.incoming() {
        let stream = match result {
            Ok(s) => s,

            Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {
                // Sleep for a bit, then try again
                thread::sleep(std::time::Duration::from_millis(10));
                continue;
            }

            Err(e) => panic!("encountered IO error: {e}"),
        };

        let arc = vm_mutex.upgrade().unwrap();
        let mut vm = arc.lock().unwrap();

        // Add the new connection to the queue
        let mut net_state = &mut vm.sys_state.net_state;
        match net_state.sockets.get_mut(&socket_id) {
            Some(Socket::Listen{ incoming, .. }) => {
                incoming.push_back(stream);
            }

            // Socket closed
            _ => {
                break;
            }
        }

        // Call on_new_conn to signal an incoming connection
        match vm.call(on_new_conn, &[Value::from(socket_id)]) {
            ExitReason::Return(val) => {}
            _ => panic!()
        }
    }
}

// Syscall to create a TCP listening socket to accept incoming connections
// u64 socket_id = net_listen(
//     const char* listen_addr,    // Network interface address to listen on, null for any address
//     callback on_new_connection, // Called on new incoming connection
// )
pub fn net_listen(
    vm: &mut VM,
    listen_addr: Value,
    on_new_conn: Value,
) -> Value
{
    // Get the input address and port to listen on
    let listen_addr = vm.get_heap_str(listen_addr.as_usize());

    // TODO: return 0 on failure
    let listener = TcpListener::bind(listen_addr).unwrap();

    // Set the listener to non-blocking
    // We do this because Rust offers us no way to close the TcpListener
    // from another thread, and so the listening thread has to periodically
    // check if it should exit.
    listener.set_nonblocking(true).expect("Cannot set non-blocking");

    // Assign a socket id to the socket
    let mut net_state = &mut vm.sys_state.net_state;
    let socket_id = net_state.next_id;
    net_state.next_id += 1;
    net_state.sockets.insert(
        socket_id,
        Socket::Listen {
            listener: listener.try_clone().unwrap(),
            incoming: VecDeque::default(),
        }
    );

    // Create a listening thread to accept incoming connections
    let vm_mutex = vm.sys_state.mutex.clone();
    let on_new_conn = on_new_conn.as_u64();
    thread::spawn(move || {
        listen_thread(
            vm_mutex,
            listener,
            socket_id,
            on_new_conn,
        )
    });

    // Return the socket id
    Value::from(socket_id)
}

/// TCP read thread
fn read_thread(
    vm_mutex: Weak<Mutex<VM>>,
    mut stream: TcpStream,
    socket_id: u64,
    on_incoming_data: u64
)
{
    loop
    {
        let mut buf: [u8; 16384] = [0; 16384];

        match stream.read(&mut buf) {
            // End of file, connection closed
            Ok(0) => {
                break;
            }

            Ok(num_bytes) => {
                let arc = vm_mutex.upgrade().unwrap();
                let mut vm = arc.lock().unwrap();

                // Append to the read buffer
                let mut net_state = &mut vm.sys_state.net_state;
                match net_state.sockets.get_mut(&socket_id) {
                    Some(Socket::Stream { read_buf, .. }) => {
                        read_buf.extend_from_slice(&buf[0..num_bytes]);
                    }

                    Some(_) => panic!(),

                    // net_close removes the socket
                    // Stop the read thread
                    None => break
                }

                // Call on_incoming_data to signal an incoming data
                match vm.call(on_incoming_data, &[Value::from(socket_id), Value::from(num_bytes)]) {
                    ExitReason::Return(val) => {}
                    _ => panic!()
                }
            }

            Err(e) => {
                println!("error in read thread: {e}");
                break
            }
        }
    }
}

// Syscall to accept a new connection
// Writes the client address in the buffer you specify
// u64 socket_id = net_accept(u64 socket_id, char* client_addr, u64 client_addr_len, callback on_incoming_data)
pub fn net_accept(
    vm: &mut VM,
    socket_id: Value,
    client_addr_buf: Value,
    addr_buf_len: Value,
    on_incoming_data: Value,
) -> Value
{
    let socket_id = socket_id.as_u64();
    let client_addr_buf = client_addr_buf.as_usize();
    let addr_buf_ptr: *mut u8 = vm.get_heap_ptr(client_addr_buf);
    let addr_buf_len = addr_buf_len.as_usize();
    let on_incoming_data = on_incoming_data.as_u64();

    let mut net_state = &mut vm.sys_state.net_state;

    // If there is a connection waiting
    match net_state.sockets.get_mut(&socket_id) {
        Some(Socket::Listen { incoming, .. }) => {
            if incoming.len() == 0 {
                panic!();
            }

            let stream = incoming.pop_front().unwrap();
            stream.set_nonblocking(false).expect("could not set stream to blocking");

            // TODO: handle the error case here
            // The connection could have dropped
            // Copy the client address into the buffer
            let peer_addr = stream.peer_addr().unwrap();
            let mut addr_str = peer_addr.to_string().into_bytes();
            addr_str.push(0);
            let num_bytes = std::cmp::min(addr_str.len(), addr_buf_len);
            unsafe {
                std::ptr::copy_nonoverlapping(addr_str.as_ptr(), addr_buf_ptr, num_bytes);
            }

            // Assign a socket id to the socket
            let socket_id = net_state.next_id;
            net_state.next_id += 1;
            net_state.sockets.insert(
                socket_id,
                Socket::Stream {
                    stream: stream.try_clone().unwrap(),
                    read_buf: Vec::default(),
                }
            );

            // Create a listening thread to accept incoming connections
            let vm_mutex = vm.sys_state.mutex.clone();
            thread::spawn(move || {
                read_thread(
                    vm_mutex,
                    stream,
                    socket_id,
                    on_incoming_data,
                )
            });

            // Return the socket id
            Value::from(socket_id)
        }
        _ => panic!()
    }
}

// Syscall to read data from a given socket into a buffer you specify
// u64 num_bytes_read = net_read(u64 socket_id, void* buf_ptr, u64 buf_len)
pub fn net_read(
    vm: &mut VM,
    socket_id: Value,
    buf_ptr: Value,
    buf_len: Value,
) -> Value
{
    let socket_id = socket_id.as_u64();
    let buf_len = buf_len.as_usize();
    let buf_ptr = buf_ptr.as_usize();
    let buf_ptr: *mut u8 = vm.get_heap_ptr(buf_ptr);

    let mut net_state = &mut vm.sys_state.net_state;
    match net_state.sockets.get_mut(&socket_id) {
        Some(Socket::Stream { read_buf, .. }) => {
            let num_bytes = std::cmp::min(buf_len, read_buf.len());

            unsafe {
                std::ptr::copy_nonoverlapping(read_buf.as_ptr(), buf_ptr, num_bytes);
            }

            read_buf.rotate_left(num_bytes);
            read_buf.truncate(read_buf.len() - num_bytes);

            Value::from(num_bytes)
        }
        _ => panic!("invalid socket id {} in net_read", socket_id)
    }
}

// Syscall to write data on a given socket
// u64 num_bytes = net_write(u64 socket_id, void* buf_ptr, u64 buf_len);
pub fn net_write(
    vm: &mut VM,
    socket_id: Value,
    buf_ptr: Value,
    buf_len: Value,
) -> Value
{
    let socket_id = socket_id.as_u64();
    let buf_len = buf_len.as_usize();
    let buf_ptr = buf_ptr.as_usize();
    let buf_ptr: *mut u8 = vm.get_heap_ptr(buf_ptr);

    let mut net_state = &mut vm.sys_state.net_state;
    match net_state.sockets.get_mut(&socket_id) {
        Some(Socket::Stream { stream, .. }) => {
            let mem_slice = unsafe { slice::from_raw_parts(buf_ptr, buf_len) };
            stream.write_all(&mem_slice).unwrap();
            Value::from(buf_len)
        }
        _ => panic!()
    }
}

// Syscall to close a socket
// net_close(u64 socket_id)
pub fn net_close(
    vm: &mut VM,
    socket_id: Value,
)
{
    let socket_id = socket_id.as_u64();

    let mut net_state = &mut vm.sys_state.net_state;

    match net_state.sockets.get_mut(&socket_id) {
        Some(Socket::Stream { stream, .. }) => {
            stream.shutdown(std::net::Shutdown::Both).unwrap();
        }

        Some(Socket::Listen { listener, .. }) => {
            // The listen thread will detect that the socket state
            // has been removed and exit
        }

        _ => panic!()
    }

    // This drops the socket
    net_state.sockets.remove(&socket_id);
}
