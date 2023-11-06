use std::collections::{HashMap, VecDeque};
use std::os::fd::RawFd;
use std::slice;
use std::thread;
use std::net::{TcpListener, TcpStream};
use std::os::fd::AsRawFd;
use std::io::{self, Read, Write};
use std::sync::{Arc, Weak, Mutex};
use crate::vm::{VM, Value, ExitReason};

// TODO: should we split listening, TCP and UDP sockets?
// State associated with a socket
pub struct Socket
{
    fd: RawFd,

    /// Incoming connections
    incoming: VecDeque<TcpStream>,

    /// Associated TCP stream
    stream: Option<TcpStream>,

    // Read buffer
    read_buf: Vec<u8>
}

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
        let arc = vm_mutex.upgrade().unwrap();
        let mut vm = arc.lock().unwrap();

        // TODO: note, accepting the connection may error,
        // for example if the socket was closed
        let stream = result.unwrap();

        // Add the new connection to the queue
        let mut net_state = &mut vm.sys_state.net_state;
        match net_state.sockets.get_mut(&socket_id) {
            Some(socket) => {
                socket.incoming.push_back(stream);
            }
            _ => panic!()
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
//     const char* listen_addr,   // Network interface address to listen on, null for any address
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
    let socket_fd = listener.as_raw_fd();

    // Assign a socket id to the socket
    let mut net_state = &mut vm.sys_state.net_state;
    let socket_id = net_state.next_id;
    net_state.next_id += 1;
    net_state.sockets.insert(
        socket_id,
        Socket {
            fd: socket_fd,
            stream: None,
            incoming: VecDeque::default(),
            read_buf: Vec::default(),
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
            Ok(num_bytes) => {

                let arc = vm_mutex.upgrade().unwrap();
                let mut vm = arc.lock().unwrap();

                // Append to the read buffer
                let mut net_state = &mut vm.sys_state.net_state;
                match net_state.sockets.get_mut(&socket_id) {
                    Some(socket) => {
                        socket.read_buf.extend_from_slice(&buf[0..num_bytes]);
                    }
                    _ => panic!()
                }

                // Call on_incoming_data to signal an incoming data
                match vm.call(on_incoming_data, &[Value::from(num_bytes)]) {
                    ExitReason::Return(val) => {}
                    _ => panic!()
                }
            }

            Err(_) => break
        }
    }
}

// Syscall to accept a new connection
// Writes the client address in the buffer you specify
// u64 socket_id = net_accept(u64 socket_id, char* client_addr, u64 client_addr_len, callback on_incoming_data)
pub fn net_accept(
    vm: &mut VM,
    socket_id: Value,
    client_addr: Value,
    client_addr_len: Value,
    on_incoming_data: Value,
) -> Value
{
    let socket_id = socket_id.as_u64();
    let client_addr = client_addr.as_u64();
    let client_addr_len = client_addr_len.as_u64();
    let on_incoming_data = on_incoming_data.as_u64();

    let mut net_state = &mut vm.sys_state.net_state;

    // If there is a connection waiting
    match net_state.sockets.get_mut(&socket_id) {
        Some(socket) => {
            if socket.incoming.len() == 0 {
                panic!();
            }

            let stream = socket.incoming.pop_front().unwrap();
            let socket_fd = stream.as_raw_fd();

            // TODO
            // TODO: we need to write the client address into the buffer
            // TODO

            // Assign a socket id to the socket
            let socket_id = net_state.next_id;
            net_state.next_id += 1;
            net_state.sockets.insert(
                socket_id,
                Socket {
                    fd: socket_fd,
                    stream: Some(stream.try_clone().unwrap()),
                    incoming: VecDeque::default(),
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
        Some(socket) => {
            let num_bytes = std::cmp::min(buf_len, socket.read_buf.len());

            unsafe {
                std::ptr::copy_nonoverlapping(socket.read_buf.as_ptr(), buf_ptr, num_bytes);
            }

            socket.read_buf.rotate_left(num_bytes);
            socket.read_buf.truncate(socket.read_buf.len() - num_bytes);

            Value::from(num_bytes)
        }
        _ => panic!()
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
        Some(socket) => {
            let stream = socket.stream.as_mut().unwrap();

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
    socked_id: Value,
)
{
    todo!();
}
