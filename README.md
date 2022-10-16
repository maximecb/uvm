# UVM

UVM Virtual Machine

## Build instructions

Dependencies:
- The Rust toolchain

Install SDL2:
```
brew install sdl2
```

Add this to `~/.zprofile`:
```
export LIBRARY_PATH="$LIBRARY_PATH:$(brew --prefix)/lib"
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
