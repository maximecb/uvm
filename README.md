# UVM

UVM Virtual Machine

## Build instructions

Dependencies:
- The Rust toolchain
- The SDL2 libraries

To install SDL2 on MacOS:
```
brew install sdl2
```

On MacOS, add this to `~/.zprofile`:
```
export LIBRARY_PATH="$LIBRARY_PATH:$(brew --prefix)/lib"
```

To install SDL2 on Debian/Ubuntu
```
sudo apt-get install libsdl2-dev
```

Install the rust toolchain:
```
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Compile the project:
```
cargo build
```

To run the compiled binary:
```
cargo run <input_file>
```
