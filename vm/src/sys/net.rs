use std::net::{TcpListener, TcpStream};
use std::io::{self, Read};
use std::thread;
use std::collections::HashMap;
use std::os::fd::{RawFd, AsRawFd};
use polling::{Event, Events, Poller, AsRawSource};
use crate::vm::{VM, Value};





/// State for the networking subsystem
#[derive(Default)]
pub struct NetState
{
    /// Map of open sockets
    sockets: HashMap<u64, RawFd>,

    /// Next socket id to use
    next_id: u64,

    /// Poller to poll the sockets
    poller: Option<Poller>,
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
    let net_state = &vm.sys_state.net_state;




    todo!();
}





// Just like the audio thread, this thread will need its own
// reference to the VM
/*
let handle = thread::spawn(|| {
    for i in 1..10 {
        println!("hi number {} from the spawned thread!", i);
        thread::sleep(Duration::from_millis(1));
    }
});
*/




fn test()
{

    // Create a TCP listener.
    let socket = TcpListener::bind("127.0.0.1:8000").unwrap();
    socket.set_nonblocking(true).unwrap();
    let key = 7; // Arbitrary key identifying the socket.

    // Create a poller and register interest in readability on the socket.
    let poller = Poller::new().unwrap();
    unsafe { poller.add(&socket, Event::readable(key)).unwrap() };

    // The event loop.
    let mut events = Events::new();


    let mut sockets: HashMap<usize, RawFd> = HashMap::default();


    loop {
        // Wait for at least one I/O event.
        events.clear();
        poller.wait(&mut events, None).unwrap();

        for ev in events.iter() {
            if ev.key == key {
                // Perform a non-blocking accept operation.
                let (stream, addr) = socket.accept().unwrap();

                // Set interest in the next readability event.
                poller.modify(&socket, Event::readable(key)).unwrap();


                let stream_fd = stream.as_raw_fd();

                let new_key = 8;
                unsafe { poller.add(&stream_fd, Event::readable(new_key)).unwrap() };

                sockets.insert(new_key, stream_fd);



            }
        }
    }




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