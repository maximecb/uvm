RUST_BACKTRACE=1 cargo run -- $* && cd ../vm && cargo run --features count_insns ../ncc/out.asm && cd ../vm
