use std::collections::HashMap;
use std::os::fd::RawFd;
use std::thread;
use std::net::{TcpListener, TcpStream};
use std::os::fd::AsRawFd;
use std::io::{self, Read};
use std::sync::{Arc, Weak, Mutex};
use crate::vm::{VM, Value, ExitReason};

// State for the networking subsystem
pub struct NetState
{
    /// Map of open sockets
    sockets: HashMap<u64, RawFd>,

    /// Incoming connections
    incoming: Vec<TcpStream>,

    /// Next socket id to use
    next_id: u64,
}

impl Default for NetState
{
    fn default() -> Self
    {
        Self {
            sockets: HashMap::default(),
            incoming: Vec::default(),

            // Start at FFFF so we can reserve the low values for error codes
            next_id: 0xFF_FF,
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
        net_state.incoming.push(stream);

        // Call on_new_conn to signal an incoming connection
        match vm.call(on_new_conn, &[Value::from(socket_id)]) {
            ExitReason::Return(val) => {}
            _ => panic!()
        }
    }
}

// Syscall to create a TCP listening socket to accept incoming connections
// u64 socket_id = net_listen_tcp(
//     u16 port_no,
//     ip_space, // IPV4 / IPV6
//     const char* address, // Network interface address to listen on, null for any address
//     callback on_new_connection, // Called on new incoming connection
//     u64 flags // optional flags, default 0
// )
pub fn net_listen_tcp(
    vm: &mut VM,
    port_no: Value,
    ip_space: Value,
    bind_address: Value,
    on_new_conn: Value,
    flags: Value,
) -> Value
{
    // TODO: accept input address and port
    // TODO: VM helper function to read UTF-8 string?

    // TODO: return 0 on failure
    let listener = TcpListener::bind("127.0.0.1:80").unwrap();
    let socket_fd = listener.as_raw_fd();

    // Assign a socket id to the socket
    let mut net_state = &mut vm.sys_state.net_state;
    let socket_id = net_state.next_id;
    net_state.next_id += 1;
    net_state.sockets.insert(socket_id, socket_fd);

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



// Syscall to accept a new connection
// Gives you the client address in the buffer you define
// Will block if there is no incoming connection request
// u64 socket_id = net_accept(u64 socket_id, client_addr_t *client_addr, callback on_incoming_data)

// Syscall to read data from a given socket into a buffer you specify
// u64 num_bytes_read = net_read(u64 socket_id, void* buffer, u64 buf_len)

// Syscall to write data on a given socket
// void net_write(u64 socket_id, void* buffer, u64 buf_len);





// Syscall to close a socket
// net_close(u64 socket_id)
pub fn net_close(
    vm: &mut VM,
    socked_id: Value,
)
{
    todo!();
}









// We could create a thread for the listening socket that accepts connections,
// and so simply use this socket in blocking mode

// However, for individual TCP streams, how do we poll?
// We could use the polling crate: https://crates.io/crates/polling



/*
fn handle_client(stream: TcpStream) {
    // ...

    stream.write(&[1])?;
    stream.read(&mut [0; 128])?;

    //stream.shutdown(Shutdown::Both).expect("shutdown call failed");


}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:80")?;

    // accept connections and process them serially
    for stream in listener.incoming() {
        handle_client(stream?);
    }
    Ok(())
}
*/





// Non-blocking read from a TCP stream
/*
let mut stream = TcpStream::connect("127.0.0.1:7878")
    .expect("Couldn't connect to the server...");
stream.set_nonblocking(true).expect("set_nonblocking call failed");

let mut buf = vec![];
loop {
    match stream.read_to_end(&mut buf) {
        Ok(_) => break,
        Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {
            // wait until network socket is ready, typically implemented
            // via platform-specific APIs such as epoll or IOCP
            wait_for_fd();
        }
        Err(e) => panic!("encountered IO error: {e}"),
    };
};
println!("bytes: {buf:?}");
*/