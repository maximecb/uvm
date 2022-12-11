# UVM

A minimalistic virtual machine designed to run self-contained applications. UVM is intended as a platform to distribute
programs that will not break and to combat code rot.

## Design Goals

UVM is designed with the following goals in mind.

**Minimalism**: to minimize the risk of breakage, keep the VM as small as possible. UVM tries to implement as few instructions as possible,
and uses an untyped design. UVM does not implement a garbage collector or type checking. The APIs it provides are low-level and have as
little surface area as possible.

**Portability**: the minimalistic design of UVM makes portability easier to achieve.

**Stability**: UVM aims to provide a foundation that remains as stable as possible over time. Ideally, existing instructions and APIs should
never change, to avoid ever breaking software that relies on UVM. The VM design is evolved slowly and carefully, and we are conservative in
choosing to implement new features.

**Self-contained**: programs that runs on UVM are self-contained in a single-file application image format. All application images must be
statically linked. This makes software easier to distribute and less likely to break.

**Easy to target**: UVM aims to be well-documented and easy to target. Its minimalistic philosophy and simple APIs make this easier to achieve.

**Performance**: although untyped, UVM is designed so that its bytecode can be easily JIT-compiled using dynamic binary translation techniques
so that it can provide a good performance level that will be adequate for most applications.
  
**Realism**: because of its minimalistic design, UVM will not be suitable for every possible use case. Its main target is software that end users
run, as well as some network services. Those who want to squeeze absolute peak performance out of an application or rely on the latest libraries
may be disappointed. However, we believe that trying to implement every feature and satisfy every possible use case would only make the design
more fragile and detract from our design goals.

## Build instructions

Dependencies:
- The [Rust toolchain](https://www.rust-lang.org/tools/install)
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

Install the Rust toolchain:
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
