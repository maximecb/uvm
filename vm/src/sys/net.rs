use std::net::{TcpListener, TcpStream};
use std::io::{self, Read};
use crate::vm::{VM, Value};

// State for the networking subsystem
#[derive(Default)]
pub struct NetState
{
    // TODO: need a list/map of open sockets
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
    address: Value,
    on_new_conn: Value,
    flags: Value,
) -> Value
{
    todo!();
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