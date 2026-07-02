# uvclang — C/C++ → UVM Compiler: Design & Plan

`uvclang` is a C/C++ compiler that targets the UVM virtual machine, using
**clang as its front-end**: clang lowers C/C++ to textual LLVM IR (`.ll`), and
uvclang's own back-end lowers that IR to UVM assembly. Reusing clang (rather than
a hand-written parser) is what buys full C/C++ and the LLVM optimizer for free —
that's the reason the project exists and the reason for the name.

**Current stage — front-end wired in.** uvclang now drives the whole pipeline
from one command: `uvclang foo.c` runs clang in-process to emit textual LLVM IR
and lowers that IR to UVM assembly (Phase 9, done — see `src/frontend.rs`). A
`.ll` argument still skips the clang step and feeds the back-end directly, so the
`tests/gen_ll.sh`-based back-end suite is unchanged. The clang invocation in
`tests/gen_ll.sh` is kept as the flag reference the driver mirrors. The ultimate
back-end target is `doom.ll` at the repo root.

Note the sibling `ncc/` is a *separate*, self-contained C→UVM compiler with its
own hand-written front-end; uvclang deliberately does the opposite and delegates
the front-end to clang, so its scope is the IR→UVM lowering plus the thin driver.

## Guiding principles

1. **Correctness first.** No optimization passes until `doom.ll` compiles and
   runs correctly. The naive-but-obviously-correct lowering wins every time.
2. **Gradual bring-up.** Work up through the test programs, smallest first
   (`empty_main` → … → `doom.ll`). Each instruction/feature is added only when a
   test needs it, and is validated before moving on.
3. **Validate two ways** (both, per decision):
   - **Self-checking tests:** programs `assert()` their own results and exit
     non-zero on failure.
   - **Differential vs native:** compile each `.c` natively with clang *and* to
     UVM via `uvclang`; run both; compare exit code and stdout.
4. **Use natural-width ops.** Emit 32-bit ops (`add_u32`, `lt_i32`, …) for
   values of width ≤ 32, and 64-bit ops only for 64-bit values. Don't promote
   everything to 64-bit. `doom.ll` is overwhelmingly `i32`, so this is the
   common path, not an afterthought.

## Current status

- **IR parser complete.** (This is the front-end *of the back-end* — clang is
  the compiler's actual front-end.) It handles every test file and fully parses
  `doom.ll` (771 fns, 1817 globals, 12090 blocks, 69286 insts), counts matching
  the `llvm-ir` crate. See `src/{lexer,ast,parser}.rs`.
- **Back-end codegen: Phases 0–8 done** (arithmetic, control flow, memory,
  globals, calls, intrinsics, corner cases) and **Phase 9 done** (the
  clang-driver front-end — `uvclang foo.c` drives clang + back-end in one
  command). Scalar **`float` (f32) is supported** (arithmetic, compare, int↔float
  conversions, libm f32 calls; see *Floating point* below). The back-end
  differential suite (`.ll` path) is **102 pass / 0 fail / 0 skip**; the
  end-to-end front-end suite (`tests/run_frontend_tests.sh`, single-command
  `uvclang foo.c`) is **117 pass / 0 fail / 0 skip**. `doom.ll` compiles through
  every construct. Callee-side `va_arg` is now supported (see *Calls* below),
  which cleared the previous skips.
- **Remaining:** Phase 10 (doom bring-up on a **real, graphical UVM runtime** —
  build syscall-backed `vm_*` with a window, framebuffer, and input from the
  start, then compile and run `doom.ll` as an actually-playable game; use
  `-timedemo` as the deterministic benchmark once it renders) and, after that,
  Phase 11 (optimization, gated on the timedemo baseline).

## The UVM target (reference)

- **64-bit stack machine.** Two sections: `.data;` and `.code;`.
- **Two stacks:**
  - The **operand/local-slot stack**: `get_arg N`, `get_local N`, `set_local N`.
    Function entry reserves slots with `push_0n N` (N ≤ 255; otherwise emit a
    loop of `push 0`). Holds SSA values and non-addressable locals.
  - A software **stack-alloc bump region** in `.data` (`__stack_alloc_sp__`,
    `__stack_alloc_max__`), the convention `ncc` uses, for addressable memory
    (`alloca`, address-of-local).
- **ISA highlights** (typed; operate on the operand stack):
  - Arithmetic: `add_u32/u64`, `sub_*`, `mul_*`, `div_{i,u}{32,64}`,
    `mod_{i,u}{32,64}`, `lshift_*`, `rshift_{i,u}*`, `and/or/xor/not_*`.
  - Compare (push 1/0): `eq/ne_*`, `lt/le/gt/ge_{i,u}{32,64}`.
  - Convert: `sx_i8_i32`, `sx_i8_i64`, `sx_i16_i32/i64`, `sx_i32_i64`,
    `trunc_u8/u16/u32`.
  - Memory: `load_u8/u16/u32/u64`, `store_u8/u16/u32/u64` (address on stack;
    loads zero-extend to 64-bit).
  - Stack: `push`, `push_0`, `push_0n`, `pop`, `dup`, `swap`, `getn`, `setn`,
    `mov`, `select`.
  - Control: `jmp`, `jz`, `jnz`, `call name, nargs`, `call_fp`, `ret`,
    `syscall name`, `panic`, `nop`. Varargs: `get_argc`, `get_var_arg`.
- **Data directives:** `.u8/.u16/.u32/.u64`, `.f32`, `.zero N`, `.stringz`,
  `.fill`, `.align N`, and `label:`.
- **Program entry / exit:** the `.code` section begins with `call main, 0; ret;`.
  The VM exits with the top-level return value as the process exit code
  (`exit(ret_val.as_i32())`). Note: native unix truncates exit codes to 8 bits,
  so differential tests should compare `native_code == uvm_code & 0xFF` (or keep
  return values in 0..255).
- **Run a program:** `cd vm && cargo run -- <file.asm>` (add `--parse-only` to
  validate without executing).

## Codegen design

### Pipeline & modules
```
C/C++  ──clang──▶  .ll  ──parse──▶  type layout  ──▶  per-fn lowering  ──▶  asm
       front-end        └──────────────────── uvclang back-end ──────────────┘
```
(The clang stage is the Phase 9 driver front-end (`src/frontend.rs`), run
in-process; the back-end pipeline — `parse → type layout → per-function
lowering → emit asm text` — then runs on the captured IR. A `.ll` input skips
clang and feeds the back-end directly, matching `tests/gen_ll.sh`.)
New source files:
- `layout.rs` — `size_of` / `align_of` / struct field offsets, computed from the
  type tree (resolving named structs); the x86_64 layout the datalayout encodes.
- `codegen.rs` — the emitter: module → `.data` + `.code` text.
- `runtime.rs` (or a checked-in `runtime.asm` prelude) — the stack-alloc region
  and any shared helpers (memcpy/memset/etc.).

### Value model (SSA → stack)
- Each **instruction result** is assigned a **local slot**. **Parameters are NOT
  copied into slots** — they are read directly via `get_arg N` on demand
  (avoids extra slots and entry copies). So an operand reference lowers to
  `get_arg N` if it's a parameter, `get_local S` if it's an instruction result,
  a `push` if it's a constant, or `push label` if it's a global.
- Lowering one instruction = push its operands → emit the typed op →
  `set_local` the result.
- Operand fetch is by slot/arg; no attempt to keep values on the stack across
  instructions (that's a later optimization).
- Slot count may exceed 255 → reserve with a `push 0` loop, not a single
  `push_0n`.

### ABI note — the calling convention is already simple
LLVM IR abstracts the target calling convention for normal scalar args: the
x86_64 register/stack assignment happens in the backend (which we replace), so
`call i32 @f(i32 %a)` lowers however we choose (`push args; call name, nargs`).
The x86_64 ABI only leaks into the IR via (a) struct-by-value
(`byval`/`sret`/`.coerce`) and (b) `va_arg` (the `%struct.__va_list_tag`
register-save-area). **`doom.ll` has zero of both** — every function passes only
scalars; structs go by pointer. So no clang flag change is needed; keep `-O2`
and inlining.

Consequence — **test-authoring guideline:** pass structs **by pointer**, not by
value, in test `.c` files, to avoid clang emitting x86_64 struct coercion. (Add
a deliberate struct-by-value test later only if we choose to support that ABI.)
The `va_arg` register-save-area is x86_64 ABI, not a toggleable optimization, so
rather than switch target triples (e.g. riscv64, a broad datalayout change) we
emulate that va_list memory layout in the prologue of variadic functions — see
*Calls* below (`gen_va_start`). doom itself never uses `va_arg`; this exists to
support ordinary variadic C (the `printf` family).

### Integer representation & casts
- **Invariant:** a value of type `iW` lives in a 64-bit slot holding its low `W`
  bits, upper bits zero (zero-extended representation).
- **Op width = value width:** width ≤ 32 → `_u32`/`_i32` ops; width 64 → `_u64`/
  `_i64`. After an op that can set high bits (e.g. `i8`/`i16` arithmetic done in
  32-bit), re-truncate (`trunc_uW`) to restore the invariant.
- **Signed ops** (`sdiv`, `srem`, `ashr`, signed `icmp`) need sign-extended
  operands: sign-extend from `W` to the op width first; use the `_i*` op.
- **`zext`** = already satisfied by the invariant (or mask/`trunc` to source
  width). **`sext`** = `sx_iW_iW2`. **`trunc`** = `trunc_uW`.

### Floating point (f32) — DONE
UVM has **single-precision only** — a fixed set of `*_f32` ops and no f64
instructions at all — so uvclang supports `float` and rejects `double`
*arithmetic* (a `double` value can still be stored/moved, just not computed on;
`fpext`/`fptrunc` error). This mirrors ncc, which is also f32-only.
- **Slot representation is identical to `i32`:** a float lives in the low 32
  bits of its slot as the IEEE-754 pattern, upper bits zero (`Value::as_f32`
  reads `f32::from_bits(low32)`). So `load float`/`store float` are just
  `load_u32`/`store_u32`, and float phi/select/call-args/returns reuse the exact
  integer value-moving paths (see `scalar_width`).
- **Arithmetic/compare:** `fadd/fsub/fmul/fdiv` → `add/sub/mul/div_f32`;
  `fneg` → xor the sign bit (`^ 0x80000000`); `fcmp` → the six ordered VM ops
  (`eq/lt/le/gt/ge_f32`, all NaN-false) with `ne_f32` for `une`, and the
  remaining unordered/`one`/`ord`/`uno` predicates composed by boolean
  complement/disjunction.
- **Conversions:** `sitofp` sign-extends to i64 then `i64_to_f32`; `uitofp` uses
  `i64_to_f32` on the already-zero-extended slot; `fptosi`/`fptoui` →
  `f32_to_i32` then re-truncate to restore the width invariant (target width
  ≤ 32; float→i64 is unsupported). Float constants lower to `push_f32` (finite)
  or the raw bit pattern (inf/NaN); `float` globals emit `.f32`.
- **libm / intrinsics:** `sinf/cosf/tanf/asinf/acosf/atanf/sqrtf/powf` (libcalls)
  and `llvm.sqrt/sin/cos/fabs/pow.f32` map to the matching `*_f32` op; `fabsf`/
  `llvm.fabs` clear the sign bit; `llvm.fmuladd.f32` → `mul_f32; add_f32` (two
  roundings — matches a baseline no-FMA native target; a float test compares
  transcendentals with tolerance to absorb any last-bit host/libm difference).
- **Header:** a minimal ISO `<math.h>` in `uvclang/include/` declares the f32
  functions (+ `M_PI`/`M_PI_F` constants). Covered by `tests/floats.c`
  (differential vs native + self-checking, -O0/-O1/-O2).

### Memory & pointers
- **`alloca`:** assign each alloca a fixed offset within the function's
  stack-alloc frame; the frame is bump-allocated from `__stack_alloc_sp__` on
  entry and the pointer restored on every `ret`. The alloca result (a pointer)
  is `bp + offset`, stored to its slot like any value.
- **`load`/`store`:** address on stack, then `load_uW`/`store_uW` by the access
  type's size.
- **`getelementptr`:** fold to address arithmetic — base + Σ(index × stride),
  where strides come from `layout.rs` (array element size, struct field offset).
  Constant indices fold to a single added constant.
- **`ptrtoint`/`inttoptr`:** no-ops on representation (pointers are 64-bit ints),
  modulo width truncation.

### Globals & constants
- Emit each global into `.data` with a label and the right directives; aggregate
  initializers (arrays/structs/`c"..."`/nested/`zeroinitializer`) lower to raw
  typed bytes with correct padding/alignment.
- `@name` references push the label address.
- Constant expressions (`getelementptr`/`add`/`sub`/conv) referenced by globals
  are evaluated to a data-section address or constant.

### Control flow
- Each basic block → a label (`<fn>__<block>:`). `br` → `jmp` / `jz` / `jnz`
  (cond on stack). `switch` → a chain of `eq` + `jnz` (jump tables later).
- **phi:** resolved as a per-edge **parallel copy** — on each predecessor edge,
  push all incoming values, then `set_local` them into the phi slots (reads
  before writes ⇒ correct simultaneous update). **Split critical edges** (insert
  a block on edges from a multi-successor pred to a multi-pred succ) so the copy
  has somewhere to live.

### Calls
- Direct: push args left-to-right, `call name, nargs`; result on stack →
  `set_local`. Void calls discard.
- **Indirect:** push args + function pointer, `call_fp`.
- **Varargs (call side):** caller just pushes all actual args (fixed + variadic)
  and calls — needed for `doom.ll` (it has one indirect call through a
  varargs-typed pointer).
- **Varargs (callee `va_arg` consumption):** SUPPORTED (`gen_va_start`). clang
  lowers `va_arg` via the **x86_64 SysV ABI** — it expands every `va_arg` into
  ordinary IR that walks a `%struct.__va_list_tag { i32 gp_offset, i32 fp_offset,
  ptr overflow_area, ptr reg_save_area }` (all of which the back-end already
  lowers). So only `llvm.va_start` needs handling: at the intrinsic site uvclang
  copies every actual argument (via `get_argc`/`get_var_arg`) into one contiguous
  8-byte-per-slot buffer bump-allocated from the stack-alloc region, then points
  `reg_save_area` at it and `overflow_area` at `+48`, seeding `gp_offset = 8·F`
  (F = fixed named params). `llvm.va_end` is a no-op. Not needed for doom
  (`doom.ll` has no variadic definitions), but it unblocks the `printf` family.
  **Limitation:** general-purpose (integer/pointer) varargs only — a
  `double`/`float` `va_arg` uses the XMM save area (`fp_offset`), which is not
  built. Covered by `variadic.c` (register + overflow paths, int/long/pointer).

### Intrinsics (lowered inline — `gen_intrinsic` in `codegen.rs`)
Dispatched by name prefix (`llvm.*`) before the ordinary call path, so no
runtime helper or call overhead. The UVM `select` op indexes slots (not stack
values), so min/max/abs use a compare + branch, mirroring `gen_select`.
| Intrinsic | Lowering |
|---|---|
| `llvm.lifetime.start/end` | drop (no-op) |
| `llvm.memcpy.p0.p0.i64` | `syscall memcpy` (native host, drops the `i1 volatile`) |
| `llvm.memset.p0.i64` | `syscall memset` (native host) |
| `llvm.abs.i32` | `x<0 ? 0-x : x` via branch (INT_MIN wraps ⇒ poison-ok) |
| `llvm.smin/smax/umin/umax` | compare + branch |
| `llvm.scmp/ucmp` | `(a>b)-(a<b)` — two compares then `sub` |
| `llvm.usub.sat.i32` | `a>b ? a-b : 0` via branch |
| `llvm.bitreverse.i8` | branchless `((b*0x0202020202)&0x010884422010)%1023` |

`strlen` and friends are **not** intrinsics — they are ordinary functions
supplied by uvclang's `<string.h>` (see *C standard library* below) and compiled
like any other code.

### Syscalls & the include directory (`uvclang/include/`)
UVM syscalls reach clang-compiled code through a generated header,
`uvclang/include/uvm/syscalls.h` (point clang at it with `-Iuvclang/include`). The
header is produced by `spec/` from `syscalls.json` — the same source of truth as
the ncc header and the VM's `constants.rs` — and is **dual-mode**, gated on
`#ifdef __clang__`:
- **clang / uvclang:** each syscall is `extern <ret> __uvm_<name>(...)` plus a
  function-like macro binding the natural name (so `memcpy(...)`, `putchar(...)`
  expand to `__uvm_memcpy(...)` etc., bypassing clang's builtin declarations with
  no signature clash — and never taking a symbol's address).
- **ncc:** the original inline-asm `asm(...) -> T { syscall name; }` macros.
- Constants (`KEY_*`, `OPEN_*`, `EVENT_*`, ...) are shared by both branches.

ncc predefines nothing and gates `#include`/`#define` on the active branch, so it
cleanly takes the `#else` path (verified: `ncc tests/graphics.c` still builds).
The identical file is written to both `ncc/include/uvm/` and `uvclang/include/uvm/`.

uvclang lowers a `call @__uvm_<name>(args...)` inline (`gen_syscall` in
`codegen.rs`): push args left-to-right, emit `syscall <name>;`, then store/`pop`
the result for value-returning syscalls. No runtime helper, no call overhead —
same strategy as the `memcpy`/`memset` intrinsics. Verified end-to-end
(`print_str`/`putchar`/`print_i64`/`print_endl` → correct UVM stdout + exit).

### C standard library (`uvclang/include/`)
Beyond the syscalls, uvclang ships its own C stdlib headers under `uvclang/include/`,
implemented on top of the UVM primitives the same way ncc's headers are (ported
from `ncc/include/`). So far: `string.h`, `ctype.h`, `stdlib.h`, `stdio.h`, and
`assert.h` — see *C standard library coverage* below for the function-by-function
status and what remains. Unlike
ncc's headers, these are plain standard-signature C bodies — clang lowers them to
LLVM IR and uvclang compiles them like any other function, which is what resolves an
external such as `@strlen` without a native libc. `size_t`/`NULL` come from
clang's own freestanding `<stddef.h>`; `memcpy`/`memset`/`memcmp` are left to the
syscalls/intrinsics (not redefined here).

Only the **UVM build** sees these headers (clang with `-Iuvclang/include`, added in
`gen_ll.sh`). The **native reference build never adds `-I`**, so it uses the
platform's own libc and clang's headers — that asymmetry is deliberate and is
what makes stdlib headers *differentially testable*: `tests/libc_string.c` and
`tests/strings.c` compile against uvclang's `string.h` for UVM and the system
`string.h` natively, and their outputs must match (verified: both pass at
-O0/-O1/-O2). Loop-idiom at -O2 rewrites some hand-written loops into
`llvm.memset`/`llvm.memcpy` (handled) but does **not** produce self-recursive
`@strlen`/`@memcpy` libcalls.

Note: `doom.ll` is a pre-generated artifact compiled *without* these headers, so
its `@strlen` stays external until doom is regenerated with `-Iuvclang/include` and
`#include <string.h>`.

### Host functions (`vm_*`) — a real UVM runtime
**Decision (revised):** build a **real UVM runtime** *before* the doom run,
rather than compiling the null stubs as-is. PureDOOM reaches the host through a
small `vm_*` callback layer (`vm_malloc/free`, `vm_open/close/read/write/seek/
tell/eof`, `vm_gettime`, `vm_print`, `vm_getenv`, `vm_poll_input`,
`vm_present_frame`, plus the `doom_set_exit` hook) wired up in `main.c`. The
reference SDL host (`main_sdl.c` in the PureDOOM tree) is a complete, working
implementation of that same layer — the UVM runtime is a **graphical** port of
it (window + framebuffer + input included from the start, not a headless
subset). All the syscalls it needs already exist in the VM (`vm/src/host.rs` +
`spec/syscalls.json`); the runtime just binds them:

| `vm_*` callback | UVM syscall(s) | Notes |
|---|---|---|
| `vm_malloc`/`vm_free` | `vm_grow_heap` / `vm_heap_size` (or uvclang's `<stdlib.h>` `malloc`/`free` on top of them) | doom's zone allocator does **one ~12 MB `malloc`** (`I_GetHeapSize` = `mb_used·1MB`, `mb_used = 6·(sizeof(void*)/4) = 12` on 64-bit) plus a few small allocs (two screen buffers 320·200 and 320·200·4, WAD-path strings). The stub `vm_malloc` returns null, so this is the one piece that must be real for anything to run. |
| `vm_open`/`close`/`read`/`write`/`seek`/`tell`/`eof` | `file_open`/`file_close`/`file_read`/`file_write`/`file_seek`/`file_tell`/`file_size` | Loads `doom1.wad`. Caveats: `file_open` is path-**sandboxed** (`is_safe_path` — keep the WAD in cwd, no `..`) and takes `OPEN_READ/WRITE/…` flags; `file_seek` is **absolute-position only** (no whence), so `vm_seek`'s `SEEK_CUR`/`SEEK_END` must be synthesized from `file_tell`/`file_size`. `getenv("DOOMWADDIR")` → null → `"."`, so doom opens `./doom1.wad`. |
| `vm_gettime` | `time_current_ms` | Split ms into `*sec`/`*usec`. Feeds `I_GetTime` (realtics + RNG seed). |
| `vm_print` | `print_str` | So the `-timedemo` result line is visible. |
| `doom_exit` (via `doom_set_exit`) | `exit` | **Must** actually exit: `I_Error` (the timedemo end path) prints then calls `doom_exit`; the current `main.c` override is an empty body, so replace it with an `exit(code)` so the run terminates cleanly. |
| `vm_getenv` | — | Return null (fine). |
| `vm_poll_input` | `window_poll_event` | Drain the event queue each frame; translate UVM events → doom input: `EVENT_KEYDOWN/UP` → `doom_key_down/up` (map UVM `KEY_*` → `doom_key_t`, mirroring `main_sdl.c`'s scancode switch; note UVM letter keys are ASCII, arrows are `16001..`), `EVENT_MOUSEDOWN/UP/MOVE` → `doom_button_down/up`/`doom_mouse_move`, `EVENT_QUIT` → exit. Event struct is `{u16 kind, window_id, key, button; i32 x,y; char text[64]}` (see `ncc/include/uvm/window.h`). |
| `vm_present_frame` | `window_draw_frame` (window made once via `window_create` in `main`) | **Byte-order fix required:** `doom_get_framebuffer(4)` returns **RGBA** (`R,G,B,255`), but `window_draw_frame` wants **BGRA** (B at lowest address). So swap R↔B per pixel into a runtime buffer before drawing — or, since we rebuild `doom.ll` from source anyway, patch `doom_get_framebuffer`'s `channels==4` branch to emit BGRA (zero per-frame cost). Frame is 320·200·4 bytes; `window_create(320, 200, …)`. |

Because uvclang compiles a **single `.ll`**, the runtime is delivered by writing
a real UVM `main.c` (the syscall-backed `vm_*` above, replacing the null stubs)
and **regenerating `doom.ll`** from PureDOOM with it (same clang flags as
`compile_llvm.sh`/`gen_ll.sh`), rather than by linking a separate object.

## Validation strategy

A `tests/` harness (shell script or `cargo test`) that, for each `tests/*.c` at
`-O0`, `-O1` and `-O2` (each level emits a different IR shape and so exercises
different uvclang code paths):
1. Compiles natively: `clang <flags> file.c -o ref`; runs `./ref`; records exit
   code + stdout.
2. Compiles to UVM: `uvclang file.ll > file.asm`; runs `cd vm && cargo run file.asm`;
   records exit code + stdout.
3. Asserts `native == uvm` (exit code mod 256, stdout exact).

Self-checking tests additionally `assert()` internally and return non-zero on
failure, so even a missing reference catches regressions. Only the `.c` sources
are checked in: the `.ll` (and `.asm`) are build artifacts, regenerated fresh
from the `.c` by `gen_ll.sh` into a temp dir at test time and never kept around.

---

## C standard library coverage

Status of the hosted libc surface uvclang ships in `uvclang/include/`, ordered
by header priority (most-used first). The *freestanding* headers — `<stddef.h>`,
`<stdint.h>`, `<stdarg.h>`, `<limits.h>`, `<stdbool.h>`, … — come from clang and
define no functions, so they are not listed. **Impl** = a body/macro exists in
`uvclang/include/`; **Test** = a `tests/*.c` exercises it. Legend: ✅ done · 🔸
partial (non-standard, or only the passing/exit path, or not differentially
comparable) · ⬜ none.

Note on differential-testability: functions whose result isn't stable across
libc implementations (`rand`/`srand` sequence, `malloc` addresses) or that the
host libc lacks (`itoa`/`ltoa`) can't be diff-tested — they're marked 🔸 and
validated by self-checking instead. `memcmp`/`memset32` currently live only in
`<uvm/syscalls.h>` as syscall macros (not in a header body), so `#include
<string.h>` alone can't resolve them and the native reference build can't see
them either.

### `<stdio.h>` — highest priority
| Function | Impl | Test |
|---|---|---|
| `puts` `putchar` | ✅ | ✅ (`stdio.c`) |
| `getchar` | ✅ | 🔸 no stdin in harness |
| `printf` `sprintf` `snprintf` `vprintf` `vsnprintf` `fputs` `fputc` | ⬜ | ⬜ — callee-side `va_arg` is now supported (`variadic.c`), so the `printf` family is unblocked; build it on `putchar`/`print_str` |
| file I/O: `fopen` `fclose` `fread` `fwrite` `fgets` `fgetc` `scanf` `sscanf` | ⬜ | — deferred (needs UVM file syscalls) |

### `<stdlib.h>`
| Function | Impl | Test |
|---|---|---|
| `abs` `malloc` `free` | ✅ | ✅ (`stdlib.c`) |
| `exit` | ✅ | 🔸 exit path only |
| `rand` `srand` | ✅ | 🔸 sequence not comparable to host |
| `itoa` `ltoa` (non-standard) | 🔸 | 🔸 not in host libc |
| `calloc` `realloc` `labs` `llabs` `atoi` `atol` `strtol` `strtoul` `abort` `qsort` `bsearch` `div` `ldiv` | ⬜ | ⬜ |
| `atof` `strtod` (floating point) | ⬜ | — deferred (floats) |

### `<string.h>`
| Function | Impl | Test |
|---|---|---|
| `strlen` `strcmp` `strncmp` `strchr` `strstr` `strncpy` `strncat` | ✅ | ✅ (`libc_string.c`) |
| `strcasecmp` (POSIX) | ✅ | ✅ |
| `memcpy` `memset` (intrinsic/syscall) | ✅ | ✅ (`intrinsics.c`) |
| `memcmp` (syscall macro only) | 🔸 | ⬜ — expose via `<string.h>` + test |
| `strcpy` `strcat` `strrchr` `strnlen` `memchr` `strspn` `strcspn` `strpbrk` `strtok` | ⬜ | ⬜ |
| `memmove` | ⬜ | — deferred (overlap-safe copy; `llvm.memmove`) |

### `<ctype.h>`
| Function | Impl | Test |
|---|---|---|
| `isalnum` `isalpha` `isdigit` `islower` `isupper` `isprint` `isspace` `iscntrl` `isgraph` `ispunct` `isxdigit` `isblank` | ✅ | ✅ (`ctype.c`) |
| `tolower` `toupper` | ✅ | ✅ (`ctype.c`) |

### `<assert.h>`
| Function | Impl | Test |
|---|---|---|
| `assert` | ✅ | ✅ passing path (`uvm_*` self-checks) + failure path (`xfail_assert.c`) |

### `<math.h>` — partial (f32)
A minimal ISO `<math.h>` ships in `uvclang/include/`, declaring the
single-precision functions uvclang lowers to UVM `*_f32` ops
(`sinf`/`cosf`/`tanf`/`asinf`/`acosf`/`atanf`/`sqrtf`/`fabsf`/`powf`) plus the
`M_*`/`M_*_F` constants. Exercised by `tests/floats.c`. **Not** provided: the
`double` variants (UVM has no f64), and f32 ops with no UVM instruction
(`floorf`/`ceilf`/`fmodf`/`expf`/`logf`/`atan2f`/…) — add when a test needs
them. Note `uvm/math.h` is a separate UVM-specific header (min/max/lerp macros),
not ISO `<math.h>`.

### Out of scope for now
`<time.h>`, `<locale.h>`, `<signal.h>`, `<setjmp.h>`, `<errno.h>`, `<wchar.h>` —
no planned support; revisit if a real program needs them.

## Milestone checklist

### Phase 0 — scaffolding  ✅ DONE
- [x] `layout.rs`: `size_of`/`align_of`/field offsets for all types (ints, ptr,
      array, named/anon struct, nested); unit tests vs known x86_64 sizes (6 tests).
- [x] `codegen.rs` skeleton: emit `.data;`/`.code;`, program entry
      (`call main, 0; ret;`), function labels.
- [x] Driver: `uvclang <file.ll> [-o out.asm] [--stats]`.
- [x] Test harness `tests/run_tests.sh`: native-vs-UVM exit-code/stdout diff.
- [x] (VM tweak) `push_0n` now takes a u16 operand, so the prologue reserves
      up to 65535 slots in one instruction (matches `get_local`'s u16 index).

### Phase 1 — empty main & returns  (`empty_main.c`)  ✅ DONE
- [x] Function prologue (`push_0n <num_slots>`) and `ret`.
- [x] `ret <iN const>`, `ret void` (push 0 for void).
- [x] `empty_main` runs in UVM, exit 0, matches native.

### Phase 2 — integer arithmetic & values  (`arithmetic.c`)  ✅ DONE
- [x] SSA-result → slot allocation; params read via `get_arg`.
- [x] Integer constants (width-masked); `add/sub/mul/udiv/sdiv/urem/srem/and/or/xor/shl/lshr/ashr`.
- [x] `trunc/zext/sext` (zext is a no-op given the zero-extended invariant);
      natural-width op selection; sub-32-bit normalization (trunc after, sign-
      extend operands for signed ops).
- [x] `arithmetic.c` matches native (exit 135); `casts.c` also compiles+matches.
- Note: `-O2` const-folds these test `main`s, so exit-code validation here is
  shallow (pipeline + no-crash on every opcode); deep arithmetic validation
  arrives once calls/memory land and functions are actually exercised.

### Phase 3 — control flow  (`control_flow.c`)  ✅ DONE
- [x] Block labels (`<fn>__<block>`); `br` (cond/uncond) → `jz`/`jmp`.
- [x] `icmp` (all 10 predicates; signed ops sign-extend sub-32 operands).
- [x] `select` (lowered via a branch, so constant operands work).
- [x] `phi` parallel-copy (push all incoming, store in reverse). Per-branch edge
      copies make critical edges correct without a separate splitting pass.
- [x] `switch` → compare chain (`eq` + `jnz` per case, fall-through default).
- [x] `control_flow.c` matches native (exit 114).
- [x] Runtime-validated during development against **`lli`** (LLVM interpreter)
      with hand-written `.ll`: loop+switch+select+edge-phis, mutually-referencing
      phi parallel-copy (Fibonacci), nested loops. All matched `lli` exactly.
      This was the real validation — `control_flow.c`'s C `main` is const-folded
      by `-O2`, so its exit code alone is shallow.

### Phase 4 — memory & pointers  (`pointers.c`)  ✅ DONE
- [x] Stack-alloc region: `__stack_alloc_sp__` (a `.addr64` bump pointer) + an
      8 MB `__stack_alloc_buf__`; per-function frame bumped on entry, restored
      on every `ret`.
- [x] `alloca` (fixed-size): each gets a frame offset; result = bp + offset.
      Dynamic (variable-length) alloca errors for now.
- [x] `load`/`store` (`load_uN`/`store_uN` by byte size; store pushes addr then
      value, matching the VM's pop order).
- [x] `getelementptr` (byte-offset `i8` GEPs, struct fields, array index, nested,
      runtime index with sign-extension; constants folded into one add).
- [x] `ptrtoint`/`inttoptr` (already handled in Phase 2 conversions).
- [x] `pointers.c` matches native (exit 223); `alloca`+load/store/gep loop
      validated during development against `lli` with a hand-written `.ll` (exit 14).

### Phase 5 — globals & aggregates  (`globals.c`)  ✅ DONE
- [x] `.data` emission: scalars (`.u8/16/32/64`), `zeroinitializer`/uninitialized
      (`.zero`), strings & byte arrays (`.hex`), pointer-to-global/function
      (`.addr64`), arrays and structs with correct padding, nested aggregates.
- [x] `@name` address references (`push <label>`; functions & globals share the
      label space).
- [x] Constant-expression initializers (`gep`/`add`/`sub`/conv, incl. nested in
      aggregates): reserve `.zero` space + compute and `store` at startup
      (tracked as (label, byte-offset, expr)).
- [x] `globals.c` matches native (exit 0); reads validated during development
      against `lli` with a hand-written `.ll` (exit 215). All 1817 doom.ll globals emit
      cleanly (uvclang now reaches doom's function bodies, stops at `call`).

### Phase 6 — functions & calls  (`strings.c`, `recursion.c`, `funcptr.c`)
- [x] Direct calls: `call <label>, <argc>`; args pushed left-to-right (arg 0 →
      `get_arg 0`); the single UVM return value is bound with `set_local`, or
      `pop`ped for void/unused results. A call to any `@name` without a body in
      the module (external/libc/intrinsic) is rejected in codegen, so tests that
      need one SKIP rather than emit a dangling label that fails to assemble.
- [x] Recursion / mutual recursion — `recursion.c` matches native at
      -O0/-O1/-O2 (exit 31).
- [x] Indirect calls: push args, then the function pointer (on top), `call_fp
      <argc>`. `funcptr.c` (incl. the array of fn pointers) matches native at
      -O1/-O2 (exit 50); -O0 SKIPs on the array-init `llvm.memcpy` (Phase 7).
- [x] Varargs **call side** — the caller just pushes every actual arg (fixed +
      variadic) and calls; no callee-side change. `vararg_call.c` matches native
      at -O0/-O1/-O2 (exit 57). Verified on `doom.ll`: it now compiles through
      all calls (incl. its indirect call via a varargs-typed pointer) and stops
      at `llvm.memset` (Phase 7). Callee-side `va_arg` stays out of scope (see
      `variadic.c`).
- [x] `strings.c` matches native at -O0/-O1/-O2 (exit 16), now that `strlen`
      comes from uvclang's `<string.h>` (see *C standard library*).
- [x] `strlen` + libc string helpers — provided by `uvclang/include/string.h`,
      exercised differentially by `libc_string.c` (exit 49).

### Phase 7 — intrinsics  ✅ DONE
- [x] `memcpy`, `memset` → native UVM `syscall memcpy`/`memset` (not inline loops).
- [x] `abs`, `smin/smax/umin/umax`, `scmp` (+`ucmp`), `usub.sat`, `bitreverse.i8`.
- [x] `lifetime.start/end` → no-ops.
- [x] Focused test `intrinsics.c` — all intrinsics exercised with runtime-derived
      inputs (so `-O2` can't fold them away), differential vs native at
      -O0/-O1/-O2 (exit 205). Also un-blocked `loops`/`funcptr`/`structs`, which
      previously SKIPped on `llvm.smax`/`llvm.memcpy`.
- Note: `doom.ll` now compiles cleanly through every intrinsic. Its one
  remaining external, `@strlen`, is now implementable via uvclang's `<string.h>`,
  but `doom.ll` is a pre-generated artifact so it must be regenerated with
  `-Iuvclang/include` + `#include <string.h>` to pick the body up.
- `llvm.va_start`/`llvm.va_end` (callee-side varargs) are now handled — see
  *Calls* above and `variadic.c`. Deferred: `llvm.memmove` (needs overlap-safe
  copy — not in doom).

### Phase 8 — more tests & corner cases  ✅ DONE
All differential vs native at -O0/-O1/-O2 (`run_tests.sh`) unless noted. No
codegen changes were needed — these exercised the existing lowering and passed.
- [x] `while`/`do-while`/nested loops + break/continue (`loops.c`). Passes at
      -O0; -O1/-O2 SKIP on the `llvm.smax` clang folds the loop bounds into
      (Phase 7), so the loop shapes themselves are validated at -O0.
- [x] Integer width conversions, signed/unsigned, 64-bit (`casts.c`), plus
      narrow compares (`narrow.c`).
- [x] `goto` + hand-rolled loops and deeper recursion (`goto.c`); division/shift
      and well-defined `INT_MIN` edge cases (`intdiv.c`) — avoids the INT_MIN/-1
      and -INT_MIN UB, sums into an unsigned accumulator.
- [x] `i8`/`i16` arithmetic and unsigned overflow wrap-around (`narrow.c`).
- [x] Nested structs, arrays of structs, and a function pointer in a struct
      field with an indirect call through it (`structs.c`); aggregates are
      static + passed by pointer to avoid memcpy.
- [x] Large/sparse `switch` (negative & INT_MAX cases), fall-through groups,
      and `unreachable`→`panic` (`switch_edge.c`).
- [x] Pointer/array boundaries: one-past-the-end, pointer difference (stride
      division), mixed element sizes (`ptr_bounds.c`); non-power-of-two stride
      (a 12-byte struct ⇒ `sdiv i64`, not a shift) in `ptrdiff.c`.
- [x] Extra corner cases that hit otherwise-untested paths: void / discarded
      calls (`voidcall.c` — the caller `pop` path), little-endian type punning
      via mixed-width load/store (`endian.c`), 2-D arrays with runtime indices
      on both dimensions (`arr2d.c`), 64-bit edges incl. `LLONG_MIN` and
      signed/unsigned compare divergence (`wide64.c`), and signed i8/i16
      div/rem/arith-shift (`narrow_signed.c`).
- [x] Callee-side `va_arg` via x86_64 va_list emulation (`gen_va_start`) —
      `variadic.c` (register + overflow paths; int/long/pointer) matches native
      at -O0/-O1/-O2. GP args only; FP varargs unsupported. (Not on the doom
      path, but unblocks the `printf` family — see *Calls* and Phase 12.)

### Phase 9 — clang-driver front-end  (`uvclang foo.c`)  ✅ DONE (core)
Fold the clang invocation (currently in `tests/gen_ll.sh`) into the `uvclang`
driver so one command compiles C/C++ straight to UVM assembly. This is the step
that makes uvclang a *C/C++* compiler rather than only an IR back-end.
Implemented in `src/frontend.rs` (clang invocation) + `src/main.rs` (driver);
end-to-end suite is `tests/run_frontend_tests.sh` (102 pass / 0 fail / 0 skip).
- [x] `uvclang foo.c`: locate clang (honor `$CLANG`/`$CLANGXX`/Homebrew LLVM/PATH
      as `gen_ll.sh` does), emit IR with the canonical flags (target triple,
      `-S -emit-llvm`, `-fno-discard-value-names`, the `-fno-*` set,
      `-Iuvclang/include` — found via `$UVCLANG_INCLUDE` or the compiled-in crate
      path), then run the existing IR→UVM back-end in-process — no temp `.ll` on
      disk (clang writes to stdout, captured into the lexer). Verified that a
      given `.c` at a given `-O` produces byte-identical asm through the driver
      and through the old `gen_ll.sh` + `.ll` path.
- [x] Argument pass-through: `-O0/-O1/-O2/-O3/-Os/-Oz`, `-o <out.asm>`, `-D`, `-I`
      (both joined `-DFOO` and split `-D FOO` forms). A `.ll` argument still skips
      the clang step, so the back-end test suite is unchanged. Added `--emit-ir`
      to dump the front-end IR (aids growing C coverage); unknown flags error.
- [ ] Grow C coverage: add support as clang surfaces new IR constructs, driven by
      real programs (same gradual, test-first rule as the back-end phases).
- [ ] C++ front-end: handle clang++ output (name-mangled symbols, vtables, and
      the wider aggregate/struct-by-value ABI it emits) once C is solid. This is
      where the ABI simplifications noted above (structs by pointer, no
      struct-by-value coercion) get revisited.

### Phase 10 — doom bring-up on a real, graphical UVM runtime
Merged with the former Phase 11 runtime work: build the real syscall-backed
`vm_*` runtime — **with graphics and input from the start** — *and* bring doom
up on it as an actually-playable game, in one effort (see *Host functions
(`vm_*`)* above). Order of work: get it rendering and playable first, then use
`-timedemo` to get a deterministic benchmark number, then move to Phase 11.

`-timedemo` stays the benchmark of choice because it's deterministic and
self-reporting: `DEMO1/DEMO2/DEMO3` are lumps in the shareware `doom1.wad`, a
demo replays a fixed input sequence ⇒ a fixed `gametic` count every run, and on
completion `G_CheckDemoStatus()` prints `"Error: timed <gametic> gametics in
<realtics> realtics"` via `I_Error` → `vm_print` + `exit`. So a benchmark run
prints its own result line and exits cleanly. (PureDOOM's `D_DoomLoop` is
single-step — its `while(1)` is disabled — so both interactive play and timedemo
run through the normal `doom_init` → `doom_update` embedding loop; the `gettime`
frame-throttle in `doom_update` caps interactive play at ~35Hz but does not cap
a slower-than-real-time VM.)

Milestone steps:
- [ ] Write the real graphical UVM `main.c` runtime (syscall-backed `vm_*` per
      the table above: `vm_malloc/free`, file I/O, `vm_gettime`, `vm_print`,
      `window_create`/`window_draw_frame` for `vm_present_frame` incl. the
      RGBA→BGRA fix, `window_poll_event` for `vm_poll_input`, `doom_set_exit` →
      `exit`) and regenerate `doom.ll` from PureDOOM with it.
- [ ] Compile all of `doom.ll` to `.asm`; resolve any unhandled construct.
      `--parse-only` (assembler accepts the output).
- [ ] Runs in UVM with `doom1.wad` in cwd: opens a window, loads the WAD, shows
      the title/menu, and is **playable** (keyboard + mouse move the player).
      This is the real correctness bar — the game visibly runs.
- [ ] Benchmark: `-timedemo demo3` plays the demo and prints the
      `timed N gametics in M realtics` line, then exits. Confirm `gametic` N is
      stable across runs; capture the wall-clock baseline (host `time` on the VM
      process, and/or bracket with `time_current_ms` in the runtime). This is the
      number Phase 11 optimizations are measured against.
- [ ] Spot-check deterministic internal functions vs native where feasible.
- [ ] (Deferred, not required to play) audio: `vm_*` sound/MIDI via UVM
      `audio_*` syscalls — timedemo and visual play don't need it.

### Phase 11 — optimization (gated on the timedemo baseline)
Only after Phase 10 renders/plays and the `-timedemo` baseline exists
(correctness first — see *Guiding principles*).
- [ ] Optimizations (keep values on stack, jump tables, slot reuse, peephole),
      each re-measured against the deterministic timedemo baseline.

### Phase 12 — C standard library completion & tests
Bring the hosted libc in `uvclang/include/` to a complete, tested state (the
gap is enumerated in *C standard library coverage* above). Same rules as the
back-end phases: standard signatures, differential vs native where comparable
and self-checking (`uvm_*`) otherwise, at -O0/-O1/-O2. Ordered by header
priority — this makes uvclang a more complete C compiler, and is independent of
the doom path (doom's only libc external is `@strlen`, already covered).
- [ ] `<stdio.h>`: `printf`/`sprintf`/`snprintf` (and the `v*` forms) — now
      unblocked (callee-side `va_arg` is implemented; see *Calls*), so build them
      on `putchar`/`print_str` the way ncc does. Note: printf's `%f` needs FP
      varargs, which `gen_va_start` does not support yet. File I/O
      (`fopen`/`fread`/…) deferred until UVM exposes file syscalls.
- [ ] `<stdlib.h>`: add `calloc`/`realloc`/`labs`/`llabs`/`atoi`/`atol`/`strtol`/
      `strtoul`/`abort`/`qsort`/`bsearch`/`div`/`ldiv`, each differentially
      tested; add a self-checking test for the existing `rand`/`srand`/`itoa`/
      `ltoa` (not diff-comparable). `atof`/`strtod` deferred (floats).
- [ ] `<string.h>`: add `strcpy`/`strcat`/`strrchr`/`strnlen`/`memchr`/`strspn`/
      `strcspn`/`strpbrk`/`strtok` (all differentially testable); route `memcmp`
      through `<string.h>` (today it's only a `<uvm/syscalls.h>` macro, so
      `#include <string.h>` can't resolve it) and add a test — a plain C body
      makes it differential, or keep the syscall and self-check it. `memmove`
      stays deferred (overlap-safe copy).
- [x] `<ctype.h>`: added `iscntrl`/`isgraph`/`ispunct`/`isxdigit`/`isblank`, and
      a differential `tests/ctype.c` over the ASCII range covering all 12
      classifiers + `tolower`/`toupper` (truthy results normalized to 0/1 —
      host macros return arbitrary nonzero). Matches native at -O0/-O1/-O2.
- [x] `<assert.h>`: `tests/xfail_assert.c` exercises the *failure* path (message
      + non-zero exit) — the one path the differential harness never hits. Runs
      under the harness's new `xfail_*` convention (require abnormal exit +
      diagnostic on stdout; no native reference).
- [ ] `<math.h>` and the other hosted headers: add only when a concrete program
      (or the C++ bring-up) needs them.

## Non-goals (for now)
- No optimization. Naive, correct lowering only.
- For doom, a real graphical runtime (real `malloc`, file I/O, `gettime`,
  `print`, window/framebuffer, keyboard/mouse input) is in scope (Phase 10);
  only **audio** (sound/MIDI) is deferred.
- No support for IR features clang `-O2` C output never emits (vectors,
  `double`/f64 arithmetic — UVM is f32-only, exception handling, etc.) until a
  test needs them. (Scalar `float`/f32 *is* supported — see *Floating point*.)
