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

### Intrinsics (lower inline or via small helpers)
| Intrinsic | Lowering |
|---|---|
| `llvm.lifetime.start/end` | drop (no-op) |
| `llvm.memcpy.p0.p0.i64` | byte/word copy loop (or runtime helper) |
| `llvm.memset.p0.i64` | `fill` / store loop |
| `llvm.abs.i32` | `dup`, compare to 0, negate-or-keep (`select`) |
| `llvm.smin/smax/umin/umax.i32` | compare + `select` |
| `llvm.scmp.i32.i32` | two compares → -1/0/1 |
| `llvm.usub.sat.i32` | `sub` then clamp at 0 |
| `llvm.bitreverse.i8` | small shift/mask helper |
| `strlen` | runtime helper (load_u8 loop) |

### Host functions (`vm_*`)
Per decision, **compile `doom.ll` as-is** — the stub `vm_*` bodies (e.g.
`vm_malloc` returns null) are compiled like any other function. Milestone goal:
the program assembles and runs in UVM without crashing. Real syscall-backed I/O
is a **later, separate** effort (a hand-written UVM runtime that overrides
`vm_*`), out of scope here.

## Validation strategy

A `tests/` harness (shell script or `cargo test`) that, for each `tests/*.c`:
1. Compiles natively: `clang <flags> file.c -o ref`; runs `./ref`; records exit
   code + stdout.
2. Compiles to UVM: `llbc file.ll > file.asm`; runs `cd vm && cargo run file.asm`;
   records exit code + stdout.
3. Asserts `native == uvm` (exit code mod 256, stdout exact).

Self-checking tests additionally `assert()` internally and return non-zero on
failure, so even a missing reference catches regressions. Keep all generated
`.ll`/`.asm` reproducible via `gen_ll.sh`.

**Hand-written `.ll` vs `lli`.** Because clang `-O2` constant-folds pure C
`main`s (so a test's *functions* compile but its exit code only reflects a
folded constant), we also keep hand-written `tests/ll/*.ll` files that exercise
real runtime behavior, and diff `llbc`→UVM against the **LLVM interpreter**
`lli` (`/opt/homebrew/opt/llvm/bin/lli`). `lli` is the LLVM reference semantics
and runs `.ll` directly — ideal for control flow, and later phases, that C-level
folding would hide. The harness runs these automatically when `lli` is present.

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
- [x] Runtime-validated against **`lli`** (LLVM interpreter) with hand-written
      `tests/ll/*.ll`: `cf` (loop+switch+select+edge-phis), `swap` (mutually-
      referencing phi parallel-copy = Fibonacci), `nested` (nested loops). All
      match `lli` exactly. This is the real validation — `control_flow.c`'s C
      `main` is const-folded by `-O2`, so its exit code alone is shallow.

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
      validated against `lli` via `tests/ll/mem.ll` (exit 14).

### Phase 5 — globals & aggregates  (`globals.c`)  ✅ DONE
- [x] `.data` emission: scalars (`.u8/16/32/64`), `zeroinitializer`/uninitialized
      (`.zero`), strings & byte arrays (`.hex`), pointer-to-global/function
      (`.addr64`), arrays and structs with correct padding, nested aggregates.
- [x] `@name` address references (`push <label>`; functions & globals share the
      label space).
- [x] Constant-expression initializers (`gep`/`add`/`sub`/conv, incl. nested in
      aggregates): reserve `.zero` space + compute and `store` at startup
      (tracked as (label, byte-offset, expr)).
- [x] `globals.c` matches native (exit 0); reads validated against `lli` via
      `tests/ll/globals_read.ll` (exit 215). All 1817 doom.ll globals emit
      cleanly (llbc now reaches doom's function bodies, stops at `call`).

### Phase 6 — functions & calls  (`strings.c`, `recursion.c`, `funcptr.c`)
- [ ] Direct calls: arg passing, return values, void calls.
- [ ] Recursion / mutual recursion (`recursion.c`).
- [ ] Indirect calls (`call_fp`), incl. array of fn pointers (`funcptr.c`).
- [ ] Varargs **call side** (push fixed + variadic args) — the one doom needs.
- [ ] `strlen` + any libc helpers used.
- [ ] `strings.c`, `recursion.c`, `funcptr.c` match native.

### Phase 7 — intrinsics
- [ ] `memcpy`, `memset`.
- [ ] `abs`, `smin/smax/umin/umax`, `scmp`, `usub.sat`, `bitreverse`.
- [ ] `lifetime.start/end` → no-ops.
- [ ] A focused test per intrinsic.

### Phase 8 — more tests & corner cases
- [ ] `while`/`do-while`/nested loops + break/continue (`loops.c`).
- [ ] Integer width conversions, signed/unsigned, 64-bit (`casts.c`).
- [ ] `goto`, deeper recursion; `INT_MIN`, division/shift edge cases.
- [ ] `i8`/`i16` arithmetic and overflow wrap-around.
- [ ] Nested structs, arrays of structs, function pointers stored in structs.
- [ ] Large/sparse `switch`; `unreachable`; fall-through.
- [ ] Pointer/array boundary and alignment cases.
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
