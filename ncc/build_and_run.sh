RUST_BACKTRACE=1 cargo run -- $* && cd ../vm && cargo run ../ncc/out.asm && cd ../vm
