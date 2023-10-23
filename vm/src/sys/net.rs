use std::net::{TcpListener, TcpStream};
use crate::vm::{VM, Value};





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

