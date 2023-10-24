use std::net::{TcpListener, TcpStream};
use std::io::{self, Read};
use crate::vm::{VM, Value};



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