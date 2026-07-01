# llbc — LLVM IR → UVM Compiler: Design & Plan

`llbc` compiles textual LLVM IR (the `.ll` produced by `clang -O2`; see
`tests/gen_ll.sh`) into UVM assembly. The ultimate target is `doom.ll` at the
repo root.

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
     UVM via `llbc`; run both; compare exit code and stdout.
4. **Use natural-width ops.** Emit 32-bit ops (`add_u32`, `lt_i32`, …) for
   values of width ≤ 32, and 64-bit ops only for 64-bit values. Don't promote
   everything to 64-bit. `doom.ll` is overwhelmingly `i32`, so this is the
   common path, not an afterthought.

## Current status

- **Front-end is complete.** The parser handles every test file and fully
  parses `doom.ll` (771 fns, 1817 globals, 12090 blocks, 69286 insts), with
  counts matching the `llvm-ir` crate. See `src/{lexer,ast,parser}.rs`.
- **Codegen is not started.** This document plans it.

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
parse (done) → type layout → per-function lowering → emit asm text
```
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
The `va_arg` register-save-area is x86_64 ABI, not a toggleable optimization;
the only way to a simple stack-only `va_list` is a different target triple
(e.g. riscv64), which isn't worth the broad datalayout change since doom never
uses `va_arg`.

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
- **Varargs (callee `va_arg` consumption):** NOT needed for doom (`doom.ll` has
  no `va_start`/`va_arg`/variadic definitions). Note: clang lowers `va_arg` via
  the **x86_64 SysV ABI** — a `%struct.__va_list_tag { i32, i32, ptr, ptr }`
  register-save-area with inline GEP/load — which does not map onto UVM's simple
  `get_var_arg`. Supporting it means emulating that va_list memory layout at the
  prologue of variadic functions. Deferred (Phase 8, optional).

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
supplied by llbc's `<string.h>` (see *C standard library* below) and compiled
like any other code.

### Syscalls & the include directory (`llbc/include/`)
UVM syscalls reach clang-compiled code through a generated header,
`llbc/include/uvm/syscalls.h` (point clang at it with `-Illbc/include`). The
header is produced by `spec/` from `syscalls.json` — the same source of truth as
the ncc header and the VM's `constants.rs` — and is **dual-mode**, gated on
`#ifdef __clang__`:
- **clang / llbc:** each syscall is `extern <ret> __uvm_<name>(...)` plus a
  function-like macro binding the natural name (so `memcpy(...)`, `putchar(...)`
  expand to `__uvm_memcpy(...)` etc., bypassing clang's builtin declarations with
  no signature clash — and never taking a symbol's address).
- **ncc:** the original inline-asm `asm(...) -> T { syscall name; }` macros.
- Constants (`KEY_*`, `OPEN_*`, `EVENT_*`, ...) are shared by both branches.

ncc predefines nothing and gates `#include`/`#define` on the active branch, so it
cleanly takes the `#else` path (verified: `ncc tests/graphics.c` still builds).
The identical file is written to both `ncc/include/uvm/` and `llbc/include/uvm/`.

llbc lowers a `call @__uvm_<name>(args...)` inline (`gen_syscall` in
`codegen.rs`): push args left-to-right, emit `syscall <name>;`, then store/`pop`
the result for value-returning syscalls. No runtime helper, no call overhead —
same strategy as the `memcpy`/`memset` intrinsics. Verified end-to-end
(`print_str`/`putchar`/`print_i64`/`print_endl` → correct UVM stdout + exit).

### C standard library (`llbc/include/`)
Beyond the syscalls, llbc ships its own C stdlib headers under `llbc/include/`,
implemented on top of the UVM primitives the same way ncc's headers are (ported
from `ncc/include/`). So far: `string.h` (`strlen`, `strcmp`, `strncmp`,
`strcasecmp`, `strchr`, `strstr`, `strncpy`, `strncat`) and `ctype.h`. Unlike
ncc's headers, these are plain standard-signature C bodies — clang lowers them to
LLVM IR and llbc compiles them like any other function, which is what resolves an
external such as `@strlen` without a native libc. `size_t`/`NULL` come from
clang's own freestanding `<stddef.h>`; `memcpy`/`memset`/`memcmp` are left to the
syscalls/intrinsics (not redefined here).

Only the **UVM build** sees these headers (clang with `-Illbc/include`, added in
`gen_ll.sh`). The **native reference build never adds `-I`**, so it uses the
platform's own libc and clang's headers — that asymmetry is deliberate and is
what makes stdlib headers *differentially testable*: `tests/libc_string.c` and
`tests/strings.c` compile against llbc's `string.h` for UVM and the system
`string.h` natively, and their outputs must match (verified: both pass at
-O0/-O1/-O2). Loop-idiom at -O2 rewrites some hand-written loops into
`llvm.memset`/`llvm.memcpy` (handled) but does **not** produce self-recursive
`@strlen`/`@memcpy` libcalls.

Note: `doom.ll` is a pre-generated artifact compiled *without* these headers, so
its `@strlen` stays external until doom is regenerated with `-Illbc/include` and
`#include <string.h>`.

### Host functions (`vm_*`)
Per decision, **compile `doom.ll` as-is** — the stub `vm_*` bodies (e.g.
`vm_malloc` returns null) are compiled like any other function. Milestone goal:
the program assembles and runs in UVM without crashing. Real syscall-backed I/O
is a **later, separate** effort (a hand-written UVM runtime that overrides
`vm_*`), out of scope here.

## Validation strategy

A `tests/` harness (shell script or `cargo test`) that, for each `tests/*.c` at
`-O0`, `-O1` and `-O2` (each level emits a different IR shape and so exercises
different llbc code paths):
1. Compiles natively: `clang <flags> file.c -o ref`; runs `./ref`; records exit
   code + stdout.
2. Compiles to UVM: `llbc file.ll > file.asm`; runs `cd vm && cargo run file.asm`;
   records exit code + stdout.
3. Asserts `native == uvm` (exit code mod 256, stdout exact).

Self-checking tests additionally `assert()` internally and return non-zero on
failure, so even a missing reference catches regressions. Only the `.c` sources
are checked in: the `.ll` (and `.asm`) are build artifacts, regenerated fresh
from the `.c` by `gen_ll.sh` into a temp dir at test time and never kept around.

---

## Milestone checklist

### Phase 0 — scaffolding  ✅ DONE
- [x] `layout.rs`: `size_of`/`align_of`/field offsets for all types (ints, ptr,
      array, named/anon struct, nested); unit tests vs known x86_64 sizes (6 tests).
- [x] `codegen.rs` skeleton: emit `.data;`/`.code;`, program entry
      (`call main, 0; ret;`), function labels.
- [x] Driver: `llbc <file.ll> [-o out.asm] [--stats]`.
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
      cleanly (llbc now reaches doom's function bodies, stops at `call`).

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
      comes from llbc's `<string.h>` (see *C standard library*).
- [x] `strlen` + libc string helpers — provided by `llbc/include/string.h`,
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
  remaining external, `@strlen`, is now implementable via llbc's `<string.h>`,
  but `doom.ll` is a pre-generated artifact so it must be regenerated with
  `-Illbc/include` + `#include <string.h>` to pick the body up.
- Deferred: `llvm.memmove` (needs overlap-safe copy — not in doom); `llvm.va_start`
  (callee-side varargs — see `variadic.c`, out of scope).

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
- [ ] *(optional, not on doom path)* callee-side `va_arg` via x86_64 va_list
      emulation (`variadic.c`).

### Phase 9 — doom.ll bring-up
- [ ] Compile all of `doom.ll` to `.asm`; resolve any unhandled construct.
- [ ] `--parse-only` (assembler accepts the output).
- [ ] Runs in UVM without crashing (stubbed I/O).
- [ ] Spot-check deterministic internal functions vs native where feasible.

### Phase 10 — later (out of current scope)
- [ ] Real UVM runtime implementing `vm_*` via syscalls so doom renders/plays.
- [ ] Optimizations (keep values on stack, jump tables, slot reuse, peephole).

## Non-goals (for now)
- No optimization. Naive, correct lowering only.
- No real device I/O for doom (stubs compiled as-is).
- No support for IR features clang `-O2` C output never emits (vectors, floats
  beyond what appears, exception handling, etc.) until a test needs them.

## Open questions / to revisit
- Exact stack-alloc frame size limits and overflow behavior to mirror `ncc`.
- memcpy/memset: inline loop vs shared runtime helper (decide by size/perf once
  doom runs).
