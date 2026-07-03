use std::sync::{Arc, Mutex};
use std::sync::atomic::{Ordering, AtomicU64, AtomicUsize};
use std::mem::{transmute, size_of, align_of};
use std::collections::{HashSet, HashMap};
use std::thread;
use std::ffi::CStr;
use crate::host::*;
use crate::program::Program;

/// Base address at which the code and data segments are loaded.
/// The region [0, MEM_BASE) is left unmapped in both address spaces so that
/// loads, stores and jumps to null/low addresses fault in hardware, without
/// the VM having to bounds-check the low end on every access. The assembler
/// relocates absolute addresses (data labels and function pointers) by this
/// amount; PC-relative offsets and integer literals are left untouched.
pub const MEM_BASE: usize = 0x1_0000;

/// Instruction opcodes
/// Note: commonly used upcodes should be in the [0, 127] range (one byte)
///       less frequently used opcodes can take multiple bytes if necessary.
#[allow(non_camel_case_types)]
#[derive(PartialEq, Copy, Clone, Debug)]
#[repr(u8)]
pub enum Op
{
    // Halt execution and produce an error
    // Panic is zero so that jumping to uninitialized memory causes panic
    panic = 0,

    // No-op (useful for code patching or patch points)
    nop,

    // Debugger breakpoint.
    // This instruction must be just one byte so it can be patched anywhere.
    breakpoint,

    // Push common constants (0, 1, 2)
    push_0,
    push_1,
    push_2,

    // Push zero n times (e.g. initialize locals)
    // push_0n <n:u8>
    push_0n,

    // push_i8 <imm:i8> (sign-extended)
    push_i8,

    // push_u32 <imm:u32>
    push_u32,

    // push_u64 <imm:u64>
    push_u64,

    // Stack manipulation
    pop,
    dup,
    swap,

    // Push the nth-value (indexed from the stack top) on top of the stack
    // getn 0 is equivalent to dup
    // getn <idx:u8>
    getn,

    // Pop the stack top and set the nth stack slot from the top to this value
    // setn 0 is equivalent to removing the value below the current stack top
    // setn <idx:u8>
    setn,

    // Get the argument count for the current stack frame
    get_argc,

    // Get the function argument at a given index
    // get_arg <idx:u8>
    get_arg,

    // Get a variadic argument with a dynamic index variable
    // get_arg (idx)
    get_var_arg,

    // Set the function argument at a given index
    // set_arg <idx:u8> (value)
    set_arg,

    // Get the local variable at a given stack slot index
    // The index is relative to the base of the stack frame
    // get_local <idx:u16>
    get_local,

    // Set the local variable at a given stack slot index
    // The index is relative to the base of the stack frame
    // set_local <idx:u16> (value)
    set_local,

    // Select between two local variables based on a popped condition.
    // Pushes local slot `a` if the condition is non-zero, else slot `b`.
    // Indices are relative to the base of the stack frame.
    // select <a:u16> <b:u16> (cond)
    select,

    // Move (copy) the value of one local variable into another.
    // Both indices are relative to the base of the stack frame.
    // mov <dst:u16> <src:u16>
    mov,

    // 32-bit bitwise operations
    and_u32,
    or_u32,
    xor_u32,
    not_u32,

    // 32-bit shift operations.
    // The shift count is taken modulo the operand bit-width: the effective
    // count is `count & 31`. A count >= 32 wraps around rather than
    // producing an unspecified result or zero. This is normative (not an
    // accident of the interpreter) and matches the native 32-bit shift
    // instructions on x86, arm64 and RISC-V, so a JIT must lower these to
    // 32-bit-wide shifts (never a 64-bit host shift on a value that happens
    // to sit in a 64-bit register).
    lshift_u32,
    rshift_u32,
    rshift_i32,

    // 32-bit integer arithmetic
    add_u32,
    sub_u32,
    mul_u32,
    div_u32,
    mod_u32,
    div_i32,
    mod_i32,

    // 32-bit integer comparisons
    eq_u32,
    ne_u32,
    lt_u32,
    le_u32,
    gt_u32,
    ge_u32,
    lt_i32,
    le_i32,
    gt_i32,
    ge_i32,

    // 64-bit bitwise operations
    and_u64,
    or_u64,
    xor_u64,
    not_u64,

    // 64-bit shift operations.
    // The shift count is taken modulo the operand bit-width: the effective
    // count is `count & 63`. A count >= 64 wraps around rather than
    // producing an unspecified result or zero. This is normative (not an
    // accident of the interpreter) and matches the native 64-bit shift
    // instructions on x86, arm64 and RISC-V.
    lshift_u64,
    rshift_u64,
    rshift_i64,

    // 64-bit integer arithmetic
    add_u64,
    sub_u64,
    mul_u64,
    div_u64,
    mod_u64,
    div_i64,
    mod_i64,

    // TODO: arithmetic with overflow
    // These instructions probably shouldn't jump directly,
    // as this would add more branch instructions to the
    // instruction set.
    // We don't need to worry about compactness.
    // add_u64_ovf,
    // sub_u64_ovf,
    // mul_i64_ovf, // produces two 64-bit words of output

    // 64-bit integer comparisons
    eq_u64,
    ne_u64,
    lt_u64,
    le_u64,
    gt_u64,
    ge_u64,
    lt_i64,
    le_i64,
    gt_i64,
    ge_i64,

    // Integer sign extension
    sx_i8_i32,
    sx_i8_i64,
    sx_i16_i32,
    sx_i16_i64,
    sx_i32_i64,

    // Truncation instructions
    trunc_u8,
    trunc_u16,
    trunc_u32,

    // 32-bit floating-point arithmetic
    add_f32,
    sub_f32,
    mul_f32,
    div_f32,

    // Floating-point math functions
    sin_f32,
    cos_f32,
    tan_f32,
    asin_f32,
    acos_f32,
    atan_f32,
    pow_f32,
    sqrt_f32,

    // 32-bit floating-point comparison instructions
    eq_f32,
    ne_f32,
    lt_f32,
    le_f32,
    gt_f32,
    ge_f32,

    // Int/float conversion
    i32_to_f32,
    i64_to_f32,
    f32_to_i32,

    // Load a value at a given adress
    // store (addr)
    load_u8,
    load_u16,
    load_u32,
    load_u64,

    // Store a value at a given adress
    // store (addr) (value)
    store_u8,
    store_u16,
    store_u32,
    store_u64,

    /*
    // TODO:
    // Load from heap at fixed address
    // This is used for reading global variables
    // The address is multiplied by the data size (x 4 or x8)
    // If we save 24 bits for the offset, then that gives us quite a lot
    load_global_u64 <addr:u24>
    */

    // Atomic load with acquire semantics
    // atomic_load (addr)
    atomic_load_u64,

    // Atomic store with release semantics
    // atomic_store (addr) (value)
    atomic_store_u64,

    // Compare-and-swap
    // Uses acquire semantics on success, relaxed on failure.
    // Store has relaxed semantics.
    // This instruction can be used to implement spin locks.
    // atomic_cas (addr) (cmp-val) (store-val)
    // Pushes the value found at the memory address
    atomic_cas_u64,

    // Set thread-local variable
    // thread_set <idx:u8> (val)
    thread_set,

    // Get thread-local variable
    // thread_get <idx:u8>
    thread_get,

    // NOTE: may want to wait for this because it's not RISC,
    //       but it could help reduce code flag
    // NOTE: should this insn have a jump offset built in?
    // - no, for consistency, let jz/jnz handle that
    // Test flag bits (logical and) with a constant
    // This can be used for tag bit tests (e.g. fixnum test)
    // Do we want to test just one specific bit, bit_idx:u8?
    // test_bit_z <bit_idx:u8>
    // test_bit_nz <bit_idx:u8>

    // TODO: we should probably have 8-bit offset versions of jump insns
    // However, this can wait. Premature optimization.
    // jmp_8, jz_8, jnz_8

    // Jump to pc offset
    // jmp <offset:i32>
    jmp,

    // Jump to pc offset if stack top is zero
    // jz <offset:i32>
    jz,

    // Jump to pc offset if stack top is not zero
    // jnz <offset:i32>
    jnz,

    // Call a function using the call stack
    // call <offset:i32> <num_args:u8> (arg0, arg1, ..., argN)
    call,

    // Call a function pointer passed as argument
    // call <num_args:u8> (arg0, arg1, ..., argN, f_ptr)
    call_fp,

    // Call into a host function
    // For example, to set up a device or to allocate more memory
    // syscall <syscall_idx:u16> (arg0, arg1, ..., argN)
    syscall,

    // Return to caller function or end thread
    // ret (value)
    ret,

    // NOTE: last opcode must have value < 255
    // Currently, every opcode is just one byte long,
    // and we hope to keep it that way, but the value
    // 255 is reserved for future 16-bit opcode extensions.
    OP_EXT = 255,
}

#[derive(Copy, Clone, Debug, PartialEq)]
pub struct Value(u64);

impl Value
{
    #[inline(always)]
    pub fn is_null(&self) -> bool {
        let Value(val) = *self;
        val == 0
    }

    #[inline(always)]
    pub fn as_u8(&self) -> u8 {
        let Value(val) = *self;
        val as u8
    }

    #[inline(always)]
    pub fn as_u16(&self) -> u16 {
        let Value(val) = *self;
        val as u16
    }

    #[inline(always)]
    pub fn as_u32(&self) -> u32 {
        let Value(val) = *self;
        val as u32
    }

    #[inline(always)]
    pub fn as_u64(&self) -> u64 {
        let Value(val) = *self;
        val as u64
    }

    #[inline(always)]
    pub fn as_usize(&self) -> usize {
        let Value(val) = *self;
        val as usize
    }

    #[inline(always)]
    pub fn as_i8(&self) -> i8 {
        let Value(val) = *self;
        val as i8
    }

    #[inline(always)]
    pub fn as_i16(&self) -> i16 {
        let Value(val) = *self;
        val as i16
    }

    #[inline(always)]
    pub fn as_i32(&self) -> i32 {
        let Value(val) = *self;
        val as i32
    }

    #[inline(always)]
    pub fn as_i64(&self) -> i64 {
        let Value(val) = *self;
        val as i64
    }

    #[inline(always)]
    pub fn as_f32(&self) -> f32 {
        let Value(val) = *self;
        f32::from_bits(val as u32)
    }
}

impl From<bool> for Value {
    #[inline(always)]
    fn from(val: bool) -> Self {
        Value(if val { 1 } else { 0 })
    }
}

impl From<u8> for Value {
    #[inline(always)]
    fn from(val: u8) -> Self {
        Value(val as u64)
    }
}

impl From<u16> for Value {
    #[inline(always)]
    fn from(val: u16) -> Self {
        Value(val as u64)
    }
}

impl From<u32> for Value {
    #[inline(always)]
    fn from(val: u32) -> Self {
        Value(val as u64)
    }
}

impl From<u64> for Value {
    #[inline(always)]
    fn from(val: u64) -> Self {
        Value(val as u64)
    }
}

impl From<usize> for Value {
    #[inline(always)]
    fn from(val: usize) -> Self {
        Value(val as u64)
    }
}

impl From<i8> for Value {
    #[inline(always)]
    fn from(val: i8) -> Self {
        Value((val as i64) as u64)
    }
}

impl From<i32> for Value {
    #[inline(always)]
    fn from(val: i32) -> Self {
        Value(val as u64)
    }
}

impl From<i64> for Value {
    #[inline(always)]
    fn from(val: i64) -> Self {
        Value(val as u64)
    }
}

impl From<f32> for Value {
    #[inline(always)]
    fn from(val: f32) -> Self {
        Value(val.to_bits() as u64)
    }
}

struct StackFrame
{
    // Previous base pointer at the time of call
    prev_bp: usize,

    // Return address
    ret_addr: usize,

    // Argument count
    argc: usize,
}






struct MemBlock
{
    // Underlying memory block
    mem_block: *mut u8,

    // Total size of the mapped memory block
    mapping_size: usize,

    // System page size
    page_size: usize,

    // Currently visible/accessible size.
    // This is a box because we need a stable pointer to access this value
    // from threads using MemView. It is atomic because grow() (serialized by
    // the VM mutex) writes it while other threads read it on every memory
    // access; the atomic prevents torn reads of the size.
    cur_size: Box<AtomicUsize>,
}

impl MemBlock
{
    pub fn new() -> MemBlock
    {
        // Try to reserve a very large block first (512GB)
        let start_size: usize = 512 * 1024 * 1024 * 1024;

        let mut alloc_size = start_size;

        let mut mem_block;

        // Try to mmap a contiguous block of memory that is
        // as large as possible. We do this so we can grow the
        // heap later without invalidating heap addresses.
        loop {
            // PROT_NONE means the data cannot be accessed yet
            mem_block = unsafe {libc::mmap(
                std::ptr::null_mut(),
                alloc_size,
                libc::PROT_NONE,
                libc::MAP_PRIVATE | libc::MAP_ANONYMOUS,
                -1,
                0
            )};

            if mem_block != libc::MAP_FAILED {
                break;
            }

            println!("mmap failed, trying again");

            // Try again with a smaller alloc size
            alloc_size /= 2;
            assert!(alloc_size > 1);
        }

        assert!(alloc_size >= 1024);

        let page_size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) } as usize;
        assert!(page_size % 8 == 0);

        // The accessible region starts at MEM_BASE: the [0, MEM_BASE) range is
        // never mapped, so null/low-address accesses fault as a hardware guard.
        assert!(MEM_BASE % page_size == 0);

        MemBlock {
            mem_block: unsafe { transmute(mem_block) },
            mapping_size: alloc_size,
            page_size,
            cur_size: Box::new(AtomicUsize::new(MEM_BASE)),
        }
    }

    /// Grow to a new size in bytes
    /// This operation is a no-op if the existing size
    /// is less than or equal to the requested size
    ///
    /// Note: this operation must be guarded by the VM
    pub fn grow(&mut self, mut new_size: usize) -> usize
    {
        // Round up to a page size multiple
        let rem = new_size % self.page_size;
        if rem != 0 {
            new_size += self.page_size - rem;
        }
        assert!(new_size % self.page_size == 0);

        let cur_size = self.cur_size.load(Ordering::Relaxed);

        if new_size <= cur_size {
            return cur_size;
        }

        // Compute the address from which to mmap new pages,
        // starting at the end of the already mapped block
        let map_addr = unsafe { transmute(self.mem_block.add(cur_size)) };

        let map_size = new_size - cur_size;
        assert!(map_size % self.page_size == 0);

        // Growing the memory block, need to map as read | write
        let mem_block = unsafe {libc::mmap(
            map_addr,
            map_size,
            libc::PROT_WRITE | libc::PROT_READ,
            libc::MAP_PRIVATE | libc::MAP_ANONYMOUS | libc::MAP_FIXED,
            -1,
            0
        )};

        if mem_block == libc::MAP_FAILED {
            panic!();
        }

        // Update the currently accessible size. The new pages are mapped by
        // the mmap above (which completes before this store), so any thread
        // that observes the new size will also see the newly mapped memory.
        self.cur_size.store(new_size, Ordering::Relaxed);

        new_size
    }

    // Create a new thread-local view of this memory block
    fn new_view(&self) -> MemView
    {
        MemView {
            mem_block: self.mem_block,
            cur_size: &*self.cur_size as *const AtomicUsize,
        }
    }
}

struct MemView
{
    // Underlying memory block
    mem_block: *mut u8,

    // Pointer to the atomic size variable owned by the parent MemBlock
    cur_size: *const AtomicUsize,
}

unsafe impl Send for MemView {}

impl MemView
{
    pub fn size_bytes(&self) -> usize
    {
        unsafe { (*self.cur_size).load(Ordering::Relaxed) }
    }

    /// Get a mutable pointer to an address/offset
    pub fn get_ptr_mut<T>(&mut self, addr: usize, num_elems: usize) -> *mut T
    {
        // Check that the address is within bounds
        let cur_size = unsafe { (*self.cur_size).load(Ordering::Relaxed) };
        let end = std::mem::size_of::<T>()
            .checked_mul(num_elems)
            .and_then(|bytes| addr.checked_add(bytes));
        if end.map_or(true, |end| end > cur_size) {
            panic!("attempting to access memory slice past end of heap");
        }

        // Check that the address is aligned
        if addr & (align_of::<T>() - 1) != 0 {
            panic!(
                "attempting to access data of type {} at unaligned address {}",
                std::any::type_name::<T>(),
                addr
            );
        }

        unsafe {
            let ptr: *mut u8 = self.mem_block.add(addr);
            transmute::<*mut u8 , *mut T>(ptr)
        }
    }

    /// Get a constant pointer to an address/offset
    pub fn get_ptr<T>(&self, addr: usize, num_elems: usize) -> *const T
    {
        // Check that the address is within bounds
        let cur_size = unsafe { (*self.cur_size).load(Ordering::Relaxed) };
        let end = std::mem::size_of::<T>()
            .checked_mul(num_elems)
            .and_then(|bytes| addr.checked_add(bytes));
        if end.map_or(true, |end| end > cur_size) {
            panic!("attempting to access memory slice past end of heap");
        }

        // Check that the address is aligned
        if addr & (align_of::<T>() - 1) != 0 {
            panic!(
                "attempting to access data of type {} at unaligned address {}",
                std::any::type_name::<T>(),
                addr
            );
        }

        unsafe {
            let ptr: *mut u8 = self.mem_block.add(addr);
            transmute::<*mut u8 , *const T>(ptr)
        }
    }

    /// Get a mutable slice inside this memory block
    pub fn get_slice_mut<T>(&mut self, addr: usize, num_elems: usize) -> &mut [T]
    {
        unsafe {
            let start_ptr = self.get_ptr_mut(addr, num_elems);
            std::slice::from_raw_parts_mut(start_ptr, num_elems)
        }
    }

    /// Read a value at an address
    /// The address does not need to be aligned
    #[inline(always)]
    pub fn read<T>(&self, addr: usize) -> T where T: Copy
    {
        // Check that the address is within bounds
        let cur_size = unsafe { (*self.cur_size).load(Ordering::Relaxed) };
        if addr.checked_add(size_of::<T>()).map_or(true, |end| end > cur_size) {
            panic!("attempting to read past end of heap");
        }

        unsafe {
            let ptr = self.mem_block.add(addr) as *const T;
            std::ptr::read_unaligned(ptr)
        }
    }

    /// Write a value at an address
    /// The address does not need to be aligned
    #[inline(always)]
    pub fn write<T>(&mut self, addr: usize, val: T) where T: Copy
    {
        // Check that the address is within bounds
        let cur_size = unsafe { (*self.cur_size).load(Ordering::Relaxed) };
        if addr.checked_add(size_of::<T>()).map_or(true, |end| end > cur_size) {
            panic!("attempting to write past end of heap");
        }

        unsafe {
            let ptr = self.mem_block.add(addr) as *mut T;
            std::ptr::write_unaligned(ptr, val);
        }
    }

    /// Read a value at the current PC and then increment the PC
    #[inline(always)]
    pub fn read_pc<T>(&self, pc: &mut usize) -> T where T: Copy
    {
        // Check that the address is within bounds
        let cur_size = unsafe { (*self.cur_size).load(Ordering::Relaxed) };
        if (*pc).checked_add(std::mem::size_of::<T>()).map_or(true, |end| end > cur_size) {
            // TODO: output name of type being read
            panic!("pc outside bounds of code space");
        }

        unsafe {
            let val_ptr = transmute::<*const u8 , *const T>(self.mem_block.add(*pc));
            *pc += size_of::<T>();
            std::ptr::read_unaligned(val_ptr)
        }
    }

    /// Read an opcode at the current PC and then increment the PC.
    /// The byte is range-checked before being interpreted as an `Op`.
    /// Reading an arbitrary byte directly as an `Op` (e.g. when a computed
    /// jump lands on operand or data bytes) would be undefined behaviour if
    /// the byte is not a valid discriminant, so we validate it here. Opcodes
    /// are assigned contiguous values from 0 (`panic`) up to and including
    /// `ret`, so any byte in that range maps to a valid `Op`.
    #[inline(always)]
    pub fn read_op(&self, pc: &mut usize) -> Op
    {
        let op_byte = self.read_pc::<u8>(pc);

        if op_byte > Op::ret as u8 {
            panic!("invalid opcode {}", op_byte);
        }

        // Safe: the range check above guarantees a valid, in-range
        // discriminant, and the discriminants are gap-free up to `ret`.
        unsafe { transmute::<u8, Op>(op_byte) }
    }
}

/// Force this OS thread's FPU into UVM's canonical floating-point
/// environment: round-to-nearest-even, with subnormals enabled (no
/// flush-to-zero). This must run on every OS thread that executes
/// bytecode, because the control register is thread-local. It defends
/// against host code (SDL, audio drivers, etc.) that may have left
/// FTZ/DAZ or a non-default rounding mode set, which would make the
/// f32 +,-,*,/ ops diverge on subnormal operands across hosts.
///
/// Forcing the *default* environment (which is exactly what the
/// compiler already assumes) is the safe direction: it can't perturb
/// constant-folding the way enabling a non-default mode would.
#[inline]
fn set_default_fp_env()
{
    #[cfg(target_arch = "x86_64")]
    unsafe {
        // MXCSR: bit 6 = DAZ, bits 13-14 = RC (00 = nearest), bit 15 = FTZ
        let mut mxcsr: u32 = 0;
        core::arch::asm!("stmxcsr [{p}]", p = in(reg) &mut mxcsr, options(nostack, preserves_flags));
        mxcsr &= !((0b11 << 13) | (1 << 15) | (1 << 6));
        core::arch::asm!("ldmxcsr [{p}]", p = in(reg) &mxcsr, options(nostack, preserves_flags));
    }

    #[cfg(target_arch = "aarch64")]
    unsafe {
        // FPCR: bits 22-23 = RMode (00 = nearest), bit 24 = FZ, bit 19 = FZ16
        let mut fpcr: u64;
        core::arch::asm!("mrs {r}, fpcr", r = out(reg) fpcr, options(nomem, nostack, preserves_flags));
        fpcr &= !((0b11 << 22) | (1 << 24) | (1 << 19));
        core::arch::asm!("msr fpcr, {r}", r = in(reg) fpcr, options(nomem, nostack, preserves_flags));
    }
}

pub struct Thread
{
    // Thread id
    pub id: u64,

    // Parent VM
    pub vm: Arc<Mutex<VM>>,

    // Code memory block
    code: MemView,

    // Heap memory block
    heap: MemView,

    // Value stack
    stack: Vec<Value>,

    // List of stack frames (activation records)
    frames: Vec<StackFrame>,

    // Thread-local variables
    locals: Vec<Value>,
}

impl Thread
{
    fn new(tid: u64, vm: Arc<Mutex<VM>>, code: MemView, heap: MemView) -> Self
    {
        Self {
            id: tid,
            vm,
            code,
            heap,
            stack: Vec::default(),
            frames: Vec::default(),
            locals: Vec::default(),
        }
    }

    #[inline(always)]
    pub fn push<T>(&mut self, val: T) where Value: From<T>
    {
        self.stack.push(Value::from(val));
    }

    #[inline(always)]
    pub fn pop(&mut self) -> Value
    {
        match self.stack.pop() {
            Some(val) => val,
            None => panic!("tried to pop when the stack is empty")
        }
    }

    /// Get the current size of the heap in bytes
    pub fn heap_size(&self) -> usize
    {
        self.heap.size_bytes()
    }

    /// Get a mutable pointer to an address/offset in the heap
    pub fn get_heap_ptr_mut<T>(&mut self, addr: usize, num_elems: usize) -> *mut T
    {
        self.heap.get_ptr_mut(addr, num_elems)
    }

    /// Get a mutable slice to access a memory region in the heap
    pub fn get_heap_slice_mut<T>(&mut self, addr: usize, num_elems: usize) -> &mut [T]
    {
        self.heap.get_slice_mut(addr, num_elems)
    }

    /// Read an UTF-8 string at a given address in the heap into a Rust string
    pub fn get_heap_str(&self, str_ptr: usize) -> &str
    {
        // Verify that there is a null-terminator for this string
        // within the bounds of the heap
        let mut str_len = 0;
        loop
        {
            let char_ptr = str_ptr + str_len;
            if char_ptr >= self.heap.size_bytes() {
                panic!("string is not properly null-terminated");
            }

            let byte_ptr: *const u8 = self.heap.get_ptr(char_ptr, 1);
            if unsafe { *byte_ptr } == 0 {
                break;
            }

            str_len += 1;
        }

        // Convert the string to a Rust string
        let char_ptr = self.heap.get_ptr(str_ptr, str_len);
        let c_str = unsafe { CStr::from_ptr(char_ptr as *const i8) };
        let rust_str = c_str.to_str().unwrap();
        rust_str
    }

    /// Call a function at a given address
    pub fn call(&mut self, callee_pc: u64, args: &[Value]) -> Value
    {
        // Pin this OS thread's FPU to UVM's canonical FP environment so
        // that f32 arithmetic is reproducible regardless of host state.
        set_default_fp_env();

        assert!(self.stack.len() == 0);
        assert!(self.frames.len() == 0);

        // Push a new stack frame
        self.frames.push(StackFrame {
            prev_bp: usize::MAX,
            ret_addr: usize::MAX,
            argc: args.len(),
        });

        // Push the arguments on the stack
        for arg in args {
            self.stack.push(*arg);
        }

        // The base pointer will point at the first local
        let mut bp = self.stack.len();
        let mut pc = callee_pc as usize;

        // For each instruction to execute
        loop
        {
            let op = self.code.read_op(&mut pc);
            //dbg!(op);

            match op
            {
                Op::panic => panic!("execution error, encountered panic opcode"),

                Op::nop => continue,

                Op::pop => {
                    self.pop();
                }

                Op::getn => {
                    let n = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.stack[self.stack.len() - (1 + n)];
                    self.push(val);
                }

                Op::setn => {
                    let n = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.pop();
                    let len = self.stack.len();
                    self.stack[len - (1 + n)] = val;
                }

                Op::dup => {
                    let val = self.pop();
                    self.push(val);
                    self.push(val);
                }

                Op::swap => {
                    let a = self.pop();
                    let b = self.pop();
                    self.push(a);
                    self.push(b);
                }

                Op::get_argc => {
                    let argc = self.frames[self.frames.len() - 1].argc;
                    self.push(Value::from(argc as u64));
                }

                Op::get_arg => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;

                    let argc = self.frames[self.frames.len() - 1].argc;
                    if idx >= argc {
                        panic!("invalid index in get_arg, idx={}, argc={}", idx, argc);
                    }

                    // Last argument is at bp - 1 (if there are arguments)
                    let stack_idx = (bp - argc) + idx;
                    self.push(self.stack[stack_idx]);
                }

                Op::get_var_arg => {
                    let idx = self.pop().as_usize();

                    let argc = self.frames[self.frames.len() - 1].argc;
                    if idx >= argc {
                        panic!("invalid index in get_arg, idx={}, argc={}", idx, argc);
                    }

                    // Last argument is at bp - 1 (if there are arguments)
                    let stack_idx = (bp - argc) + idx;
                    self.push(self.stack[stack_idx]);
                }

                Op::set_arg => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;

                    let argc = self.frames[self.frames.len() - 1].argc;
                    if idx >= argc {
                        panic!("invalid index in set_arg, idx={}, argc={}", idx, argc);
                    }

                    // Last argument is at bp - 1 (if there are arguments)
                    let stack_idx = (bp - argc) + idx;
                    let val = self.pop();
                    self.stack[stack_idx] = val;
                }

                Op::get_local => {
                    let idx = self.code.read_pc::<u16>(&mut pc) as usize;

                    if bp + idx >= self.stack.len() {
                        panic!("invalid index {} in get_local", idx);
                    }

                    self.push(self.stack[bp + idx]);
                }

                Op::set_local => {
                    let idx = self.code.read_pc::<u16>(&mut pc) as usize;
                    let val = self.pop();

                    if bp + idx >= self.stack.len() {
                        panic!("invalid index in set_local");
                    }

                    self.stack[bp + idx] = val;
                }

                Op::select => {
                    let idx_a = self.code.read_pc::<u16>(&mut pc) as usize;
                    let idx_b = self.code.read_pc::<u16>(&mut pc) as usize;
                    let cond = self.pop().as_u64();

                    let idx = if cond != 0 { idx_a } else { idx_b };
                    if bp + idx >= self.stack.len() {
                        panic!("invalid index {} in select", idx);
                    }

                    self.push(self.stack[bp + idx]);
                }

                Op::mov => {
                    let dst = self.code.read_pc::<u16>(&mut pc) as usize;
                    let src = self.code.read_pc::<u16>(&mut pc) as usize;

                    if bp + dst >= self.stack.len() || bp + src >= self.stack.len() {
                        panic!("invalid index in mov");
                    }

                    self.stack[bp + dst] = self.stack[bp + src];
                }

                Op::push_0 => {
                    self.push(0);
                }
                Op::push_1 => {
                    self.push(1);
                }
                Op::push_2 => {
                    self.push(2);
                }

                Op::push_0n => {
                    let n = self.code.read_pc::<u16>(&mut pc);
                    self.stack.resize(self.stack.len() + n as usize, Value::from(0));
                }

                Op::push_i8 => {
                    let val = self.code.read_pc::<i8>(&mut pc);
                    self.push(val);
                }

                Op::push_u32 => {
                    let val = self.code.read_pc::<u32>(&mut pc);
                    self.push(val);
                }

                Op::push_u64 => {
                    let val = self.code.read_pc::<u64>(&mut pc);
                    self.push(val);
                }

                Op::and_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() & v1.as_u32());
                }

                Op::or_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() | v1.as_u32());
                }

                Op::xor_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() ^ v1.as_u32());
                }

                Op::not_u32 => {
                    let v0 = self.pop();
                    self.push(!v0.as_u32());
                }

                // For the shift ops below, the count is taken modulo the
                // operand bit-width (count & 31). wrapping_shl/shr implement
                // exactly this masking; this is the normative behavior, see
                // the op definitions above.
                Op::lshift_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_shl(v1.as_u32())
                    );
                }

                Op::rshift_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_shr(v1.as_u32())
                    );
                }

                Op::rshift_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i32().wrapping_shr(v1.as_u32())
                    );
                }

                Op::add_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_add(v1.as_u32())
                    );
                }

                Op::sub_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_sub(v1.as_u32())
                    );
                }

                Op::mul_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32().wrapping_mul(v1.as_u32())
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32() / v1.as_u32()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u32() % v1.as_u32()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i32() / v1.as_i32()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i32() % v1.as_i32()
                    );
                }

                Op::eq_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() == v1.as_u32());
                }

                Op::ne_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() != v1.as_u32());
                }

                Op::lt_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() < v1.as_u32());
                }

                Op::le_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() <= v1.as_u32());
                }

                Op::gt_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() > v1.as_u32());
                }

                Op::ge_u32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u32() >= v1.as_u32());
                }

                Op::lt_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() < v1.as_i32());
                }

                Op::le_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() <= v1.as_i32());
                }

                Op::gt_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() > v1.as_i32());
                }

                Op::ge_i32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i32() >= v1.as_i32());
                }

                Op::and_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() & v1.as_u64());
                }

                Op::or_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() | v1.as_u64());
                }

                Op::xor_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() ^ v1.as_u64());
                }

                Op::not_u64 => {
                    let v0 = self.pop();
                    self.push(!v0.as_u64());
                }

                // For the shift ops below, the count is taken modulo the
                // operand bit-width (count & 63). wrapping_shl/shr implement
                // exactly this masking; this is the normative behavior, see
                // the op definitions above.
                Op::lshift_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_shl(v1.as_u32())
                    );
                }

                Op::rshift_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_shr(v1.as_u32())
                    );
                }

                Op::rshift_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i64().wrapping_shr(v1.as_u32())
                    );
                }

                Op::add_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_add(v1.as_u64())
                    );
                }

                Op::sub_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_sub(v1.as_u64())
                    );
                }

                Op::mul_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64().wrapping_mul(v1.as_u64())
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64() / v1.as_u64()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_u64() % v1.as_u64()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::div_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i64() / v1.as_i64()
                    );
                }

                // Division by zero will cause a panic (this is intentional)
                Op::mod_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(
                        v0.as_i64() % v1.as_i64()
                    );
                }

                Op::eq_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() == v1.as_u64());
                }

                Op::ne_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() != v1.as_u64());
                }

                Op::lt_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() < v1.as_u64());
                }

                Op::le_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() <= v1.as_u64());
                }

                Op::gt_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() > v1.as_u64());
                }

                Op::ge_u64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_u64() >= v1.as_u64());
                }

                Op::lt_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() < v1.as_i64());
                }

                Op::le_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() <= v1.as_i64());
                }

                Op::gt_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() > v1.as_i64());
                }

                Op::ge_i64 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_i64() >= v1.as_i64());
                }

                Op::sx_i8_i32 => {
                    let v = self.pop();
                    self.push(v.as_i8() as i32);
                }

                Op::sx_i8_i64 => {
                    let v = self.pop();
                    self.push(v.as_i8() as i64);
                }

                Op::sx_i16_i32 => {
                    let v = self.pop();
                    self.push(v.as_i16() as i32);
                }

                Op::sx_i16_i64 => {
                    let v = self.pop();
                    self.push(v.as_i16() as i64);
                }

                Op::sx_i32_i64 => {
                    let v = self.pop();
                    self.push(v.as_i32() as i64);
                }

                Op::trunc_u8 => {
                    let v = self.pop();
                    self.push(v.as_u8());
                }

                Op::trunc_u16 => {
                    let v = self.pop();
                    self.push(v.as_u16());
                }

                Op::trunc_u32 => {
                    let v = self.pop();
                    self.push(v.as_u32());
                }

                Op::add_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() + v1.as_f32());
                }

                Op::sub_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() - v1.as_f32());
                }

                Op::mul_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() * v1.as_f32());
                }

                // Should return NaN for invalid inputs
                Op::div_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() / v1.as_f32());
                }

                Op::sin_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.sin());
                }

                Op::cos_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.cos());
                }

                // Should return NaN for invalid inputs
                Op::tan_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.tan());
                }

                // Should return NaN for invalid inputs
                Op::asin_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.asin());
                }

                // Should return NaN for invalid inputs
                Op::acos_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.acos());
                }

                Op::atan_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.atan());
                }

                // Should return NaN for invalid inputs
                Op::pow_f32 => {
                    let v1 = self.pop().as_f32();
                    let v0 = self.pop().as_f32();
                    self.push(v0.powf(v1));
                }

                // Should return NaN for invalid inputs
                Op::sqrt_f32 => {
                    let v0 = self.pop().as_f32();
                    self.push(v0.sqrt());
                }

                Op::eq_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() == v1.as_f32());
                }

                Op::ne_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() != v1.as_f32());
                }

                Op::lt_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() < v1.as_f32());
                }

                Op::le_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() <= v1.as_f32());
                }

                Op::gt_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() > v1.as_f32());
                }

                Op::ge_f32 => {
                    let v1 = self.pop();
                    let v0 = self.pop();
                    self.push(v0.as_f32() >= v1.as_f32());
                }

                // Follows Rust semantics:
                // - Round ties to even
                // - Never panics
                Op::i32_to_f32 => {
                    let v = self.pop();
                    self.push(v.as_i32() as f32);
                }

                // Follows Rust semantics:
                // - Round ties to even
                // - Never panics
                Op::i64_to_f32 => {
                    let v = self.pop();
                    self.push(v.as_i64() as f32);
                }

                // Follows Rust semantics:
                // - Rounds towards zero (truncates)
                // - Saturates to min/max int values
                // - NaN converts to zero
                // - Never panics
                Op::f32_to_i32 => {
                    let v = self.pop();
                    self.push(v.as_f32() as i32);
                }

                // Note: load/store accept unaligned addresses
                Op::load_u8 => {
                    let addr = self.pop().as_usize();
                    let val: u8 = self.heap.read(addr);
                    self.push(val);
                }

                Op::load_u16 => {
                    let addr = self.pop().as_usize();
                    let val: u16 = self.heap.read(addr);
                    self.push(val);
                }

                Op::load_u32 => {
                    let addr = self.pop().as_usize();
                    let val: u32 = self.heap.read(addr);
                    self.push(val);
                }

                Op::load_u64 => {
                    let addr = self.pop().as_usize();
                    let val: u64 = self.heap.read(addr);
                    self.push(val);
                }

                Op::store_u8 => {
                    let val = self.pop().as_u8();
                    let addr = self.pop().as_usize();
                    self.heap.write(addr, val);
                }

                Op::store_u16 => {
                    let val = self.pop().as_u16();
                    let addr = self.pop().as_usize();
                    self.heap.write(addr, val);
                }

                Op::store_u32 => {
                    let val = self.pop().as_u32();
                    let addr = self.pop().as_usize();
                    self.heap.write(addr, val);
                }

                Op::store_u64 => {
                    let val = self.pop().as_u64();
                    let addr = self.pop().as_usize();
                    self.heap.write(addr, val);
                }

                Op::atomic_load_u64 => {
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr_mut(addr, 1);
                    let atomic = unsafe {AtomicU64::from_ptr(heap_ptr) };
                    let val = atomic.load(Ordering::Acquire);
                    self.push(Value::from(val));
                }

                Op::atomic_store_u64 => {
                    let val = self.pop().as_u64();
                    let addr = self.pop().as_usize();
                    let heap_ptr = self.get_heap_ptr_mut(addr, 1);
                    let atomic = unsafe {AtomicU64::from_ptr(heap_ptr) };
                    atomic.store(val, Ordering::Release);
                }

                // Compare-and-swap
                // Uses acquire semantics on success, relaxed on failure.
                // Store has relaxed semantics.
                // This instruction can be used to implement spin locks.
                // atomic_cas (addr) (cmp-val) (store-val)
                // Pushes the value found at the memory address
                Op::atomic_cas_u64 => {
                    let store_val = self.pop().as_u64();
                    let cmp_val = self.pop().as_u64();
                    let addr = self.pop().as_usize();

                    let heap_ptr = self.get_heap_ptr_mut(addr, 1);
                    let atomic = unsafe {AtomicU64::from_ptr(heap_ptr) };

                    let result = atomic.compare_exchange(
                        cmp_val,
                        store_val,
                        Ordering::Acquire,
                        Ordering::Relaxed,
                    );

                    match result {
                        Ok(val) => self.push(Value::from(val)),
                        Err(actual_val) => self.push(Value::from(actual_val)),
                    }
                }

                Op::thread_set => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;
                    let val = self.pop();

                    if idx >= self.locals.len() {
                        self.locals.resize(idx + 1, Value::from(0));
                    }

                    self.locals[idx] = val;
                }

                Op::thread_get => {
                    let idx = self.code.read_pc::<u8>(&mut pc) as usize;

                    if idx >= self.locals.len() {
                        self.push(Value::from(0));
                    } else {
                        self.push(self.locals[idx])
                    }
                }

                Op::jmp => {
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;
                    pc = ((pc as isize) + offset) as usize;
                }

                Op::jz => {
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;
                    let v0 = self.pop();

                    if v0.as_i64() == 0 {
                        pc = ((pc as isize) + offset) as usize;
                    }
                }

                Op::jnz => {
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;
                    let v0 = self.pop();

                    if v0.as_i64() != 0 {
                        pc = ((pc as isize) + offset) as usize;
                    }
                }

                // call <num_args:u8> <offset:i32> (arg0, arg1, ..., argN)
                Op::call => {
                    // Offset of the function to call
                    let offset = self.code.read_pc::<i32>(&mut pc) as isize;

                    // Argument count
                    let num_args = self.code.read_pc::<u8>(&mut pc) as usize;
                    assert!(num_args <= self.stack.len() - bp);

                    self.frames.push(StackFrame {
                        prev_bp: bp,
                        ret_addr: pc,
                        argc: num_args,
                    });

                    // The base pointer will point at the first local
                    bp = self.stack.len();
                    pc = ((pc as isize) + offset) as usize;
                }

                // call <num_args:u8> (arg0, arg1, ..., argN, f_ptr)
                Op::call_fp => {
                    // Absolute address of the function to call
                    let fp = self.pop();

                    // Argument count
                    let num_args = self.code.read_pc::<u8>(&mut pc) as usize;
                    assert!(num_args <= self.stack.len() - bp);

                    self.frames.push(StackFrame {
                        prev_bp: bp,
                        ret_addr: pc,
                        argc: num_args,
                    });

                    // The base pointer will point at the first local
                    bp = self.stack.len();
                    pc = fp.as_usize();
                }

                Op::syscall => {
                    let syscall_idx = self.code.read_pc::<u16>(&mut pc);
                    let syscall_fn = get_syscall(syscall_idx);

                    match syscall_fn
                    {
                        HostFn::Fn0_0(fun) => {
                            fun(self)
                        }

                        HostFn::Fn0_1(fun) => {
                            let v = fun(self);
                            self.push(v);
                        }

                        HostFn::Fn1_0(fun) => {
                            let a0 = self.pop();
                            fun(self, a0)
                        }

                        HostFn::Fn1_1(fun) => {
                            let a0 = self.pop();
                            let v = fun(self, a0);
                            self.push(v);
                        }

                        HostFn::Fn2_0(fun) => {
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1)
                        }

                        HostFn::Fn2_1(fun) => {
                            let a1 = self.pop();
                            let a0 = self.pop();
                            let v = fun(self, a0, a1);
                            self.push(v);
                        }

                        HostFn::Fn3_0(fun) => {
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1, a2)
                        }

                        HostFn::Fn3_1(fun) => {
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            let v = fun(self, a0, a1, a2);
                            self.push(v);
                        }

                        HostFn::Fn4_0(fun) => {
                            let a3 = self.pop();
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            fun(self, a0, a1, a2, a3)
                        }

                        HostFn::Fn4_1(fun) => {
                            let a3 = self.pop();
                            let a2 = self.pop();
                            let a1 = self.pop();
                            let a0 = self.pop();
                            let v = fun(self, a0, a1, a2, a3);
                            self.push(v);
                        }
                    }
                }

                Op::ret => {
                    if self.stack.len() <= bp {
                        panic!("ret with no return value on stack");
                    }

                    let ret_val = self.pop();

                    // If this is a top-level return
                    if self.frames.len() == 1 {
                        self.stack.clear();
                        self.frames.clear();
                        return ret_val;
                    }

                    assert!(self.frames.len() > 0);
                    let top_frame = self.frames.pop().unwrap();

                    // Pop all local variables and arguments
                    // We pop arguments in the callee so we can support tail calls
                    assert!(self.stack.len() >= bp - top_frame.argc);
                    self.stack.truncate(bp - top_frame.argc);

                    pc = top_frame.ret_addr;
                    bp = top_frame.prev_bp;

                    self.push(ret_val);
                }

                _ => panic!("unknown opcode {:?}", op),
            }
        }
    }
}

pub struct VM
{
    // Heap memory space
    heap: MemBlock,

    // Code memory space
    code: MemBlock,

    // Next thread id to assign
    next_tid: u64,

    // Map from actor ids to thread join handles
    threads: HashMap<u64, thread::JoinHandle<Value>>,

    // Reference to self
    // Needed to instantiate actors
    vm: Option<Arc<Mutex<VM>>>,
}

// Needed to send Arc<Mutex<VM>> to thread
unsafe impl Send for VM {}

impl VM
{
    pub fn new(prog: Program) -> Arc<Mutex<VM>>
    {
        let mut code = MemBlock::new();
        let mut heap = MemBlock::new();

        // Resize the code and memory blocks to accomodate the program.
        // Both segments are loaded at MEM_BASE so that address 0 (and the
        // rest of the guard region below MEM_BASE) stays unmapped.
        code.grow(MEM_BASE + prog.code.len());
        heap.grow(MEM_BASE + prog.data.len());

        // Copy the program code
        let mut code_view = code.new_view();
        let mut code_slice: &mut [u8] = code_view.get_slice_mut(MEM_BASE, prog.code.len());
        code_slice.clone_from_slice(prog.code.as_slice());

        // Copy the program data
        let mut heap_view = heap.new_view();
        let mut heap_slice: &mut [u8] = heap_view.get_slice_mut(MEM_BASE, prog.data.len());
        heap_slice.clone_from_slice(prog.data.as_slice());

        let vm = Self {
            code,
            heap,
            next_tid: 0,
            threads: HashMap::default(),
            vm: None,
        };

        let vm = Arc::new(Mutex::new(vm));

        // Store a reference to the mutex on the VM
        // This is so we can pass this reference to threads
        vm.lock().unwrap().vm = Some(vm.clone());

        vm
    }

    /// Grow the heap to a new size in bytes
    pub fn grow_heap(&mut self, num_bytes: usize) -> usize
    {
        self.heap.grow(num_bytes)
    }

    // Create a new thread object without beginning execution
    pub fn new_thread(vm: &Arc<Mutex<VM>>) -> Thread
    {
        // Assign a thread id
        let mut vm_ref = vm.lock().unwrap();
        let tid = vm_ref.next_tid;
        vm_ref.next_tid += 1;

        // Create thread-local code and heap memory block views
        let code = vm_ref.code.new_view();
        let heap = vm_ref.heap.new_view();

        drop(vm_ref);

        let vm_mutex = vm.clone();

        Thread::new(tid, vm_mutex, code, heap)
    }

    // Spawn a new thread and begin executing the specified function
    pub fn spawn_thread(vm: &Arc<Mutex<VM>>, callee_pc: u64, args: Vec<Value>) -> u64
    {
        let mut thread = VM::new_thread(vm);
        let tid = thread.id;

        // Spawn a new thread
        let handle = thread::spawn(move || {
            thread.call(callee_pc, args.as_slice())
        });

        // Store the join handle on the VM
        let mut vm_ref = vm.lock().unwrap();
        vm_ref.threads.insert(tid, handle);
        drop(vm_ref);

        tid
    }

    // Wait for a thread to produce a result and return it
    pub fn join_thread(vm: &Arc<Mutex<VM>>, tid: u64) -> Value
    {
        // Get the join handle, then release the VM lock
        let mut vm = vm.lock().unwrap();
        let mut handle = vm.threads.remove(&tid).unwrap();
        drop(vm);

        handle.join().expect(&format!("could join thread with id {}", tid))
    }

    // Call a function in the main actor
    pub fn call(vm: &mut Arc<Mutex<VM>>, callee_pc: u64, args: &[Value]) -> Value
    {
        // Assign a thread id
        let mut vm_ref = vm.lock().unwrap();
        let tid = vm_ref.next_tid;
        assert!(tid == 0);
        vm_ref.next_tid += 1;

        // Create thread-local code and heap memory block views
        let code = vm_ref.code.new_view();
        let heap = vm_ref.heap.new_view();

        drop(vm_ref);

        let vm_mutex = vm.clone();
        let mut thread = Thread::new(tid, vm_mutex, code, heap);
        thread.call(callee_pc, args)
    }
}

#[cfg(test)]
mod tests
{
    use super::*;
    use crate::asm::*;

    fn eval_src(src: &str) -> Value
    {
        dbg!(src);
        let asm = Assembler::new();
        let prog = asm.parse_str(src).unwrap();
        let mut vm = VM::new(prog);
        let result = VM::call(&mut vm, MEM_BASE as u64, &[]);

        result
    }

    fn eval_i64(src: &str, expected: i64)
    {
        let result = eval_src(src);
        assert_eq!(result, expected.into());
    }

    #[test]
    fn test_opcodes()
    {
        // We can have at most 254 short single-byte opcodes
        // The last opcode value (OP_EXT) is reserved for future long opcodes
        assert!(Op::ret as usize <= 254);

        // Keep track of how many short opcodes we have so far
        dbg!(Op::ret as usize);
        assert!(Op::ret as usize <= 120);
    }

    #[test]
    fn test_basics()
    {
        // Integer literals
        eval_i64("push_i8 1; ret;", 1);
        eval_i64("push_i8 -3; ret;", -3);
        eval_i64("push_u64 1_333_444; ret;", 1_333_444);
        eval_i64("push_u64 0xFF; ret;", 0xFF);
        eval_i64("push_u64 0b1101; ret;", 0b1101);

        // Push mnemonic
        eval_i64("push 0; ret;", 0);
        eval_i64("push 1; ret;", 1);
        eval_i64("push -1; ret;", -1);
        eval_i64("push 0xFFFF; ret;", 0xFFFF);
        eval_i64(".data; LABEL: .u64 0; .code; push LABEL; ret;", MEM_BASE as i64);

        // Stack manipulation
        eval_i64("push_i8 7; push_i8 3; swap; ret;", 7);
        eval_i64("push_i8 7; push_i8 3; swap; swap; pop; ret;", 7);

        // Integer arithmetic
        eval_i64("push_i8 1; push_i8 10; add_u64; ret;", 11);
        eval_i64("push_i8 5; push_i8 10; sub_u64; ret;", -5);
        eval_i64("push_i8 10; push_i8 2; sub_u64; ret;", 8);
        eval_i64("push 5; push_i8 -6; mul_u64; ret;", -30);
        eval_i64("push 1; push 2; lshift_u64; ret;", 4);

        // Shift count is taken modulo the operand bit-width. A count >= the
        // width wraps around (count & (width-1)) rather than being
        // unspecified. These lock in the normative rule against a future
        // JIT / reimplementation diverging (e.g. a naive 64-bit host shift).
        eval_i64("push 1; push 8; lshift_u32; ret;", 256);
        eval_i64("push 1; push 40; lshift_u32; ret;", 256); // 40 & 31 == 8
        eval_i64("push 1; push 32; lshift_u32; ret;", 1);   // 32 & 31 == 0
        eval_i64("push 256; push 40; rshift_u32; ret;", 1); // 40 & 31 == 8
        eval_i64("push 1; push 72; lshift_u64; ret;", 256); // 72 & 63 == 8
        eval_i64("push 1; push 64; lshift_u64; ret;", 1);   // 64 & 63 == 0

        // Comparisons
        eval_i64("push_i8 1; push_i8 10; lt_i64; ret;", 1);
        eval_i64("push_i8 11; push_i8 1; lt_i64; ret;", 0);
    }

    #[test]
    fn test_setlocal()
    {
        eval_i64(".code; push 0; push 77; set_local 0; get_local 0; ret;", 77);
    }

    #[test]
    fn test_mov()
    {
        // Copy local slot 1 into slot 0
        eval_i64(".code; push 0; push 55; mov 0, 1; get_local 0; ret;", 55);
        // mov leaves the source slot unchanged
        eval_i64(".code; push 0; push 55; mov 0, 1; get_local 1; ret;", 55);
    }

    #[test]
    fn test_get_argc()
    {
        // The top-level frame is entered with no arguments
        eval_i64("get_argc; ret;", 0);

        // get_argc reports the argument count of the current frame
        eval_i64("push 42; call FN, 1; ret; FN: get_argc; ret;", 1);
        eval_i64("push 10; push 20; push 30; call FN, 3; ret; FN: get_argc; ret;", 3);
    }

    #[test]
    fn test_floats()
    {
        eval_i64("push_f32 1.5; push_f32 2.5; add_f32; push_f32 4.0; eq_u64; ret;", 1);
    }

    #[test]
    fn test_loop()
    {
        // Simple loop
        eval_i64("push_i8 0; LOOP: push_i8 1; add_u64; dup; push_i8 10; eq_u64; jz LOOP; ret;", 10);
    }

    #[test]
    fn test_load_store()
    {
        // Store instruction (addresses are taken relative to a data label,
        // since the low addresses are now in the guard region)
        eval_i64(".data; D: .zero 255; .code; push D; push_i8 77; store_u8; push_i8 11; ret;", 11);
    }

    #[test]
    fn test_unaligned_load_store()
    {
        // The data label resolves to MEM_BASE, which is aligned, so we add an
        // odd offset to exercise unaligned accesses.

        // Store and load a u64 at an unaligned address
        eval_i64(".data; D: .zero 255; .code; push D; push 1; add_u64; push 0x1122334455667788; store_u64; push D; push 1; add_u64; load_u64; ret;", 0x1122334455667788);

        // Store and load a u32 at an unaligned address
        eval_i64(".data; D: .zero 255; .code; push D; push 3; add_u64; push 0x0AABBCCD; store_u32; push D; push 3; add_u64; load_u32; ret;", 0x0AABBCCD);

        // Store and load a u16 at an unaligned address
        eval_i64(".data; D: .zero 255; .code; push D; push 7; add_u64; push 0x1234; store_u16; push D; push 7; add_u64; load_u16; ret;", 0x1234);

        // Unaligned stores should not clobber neighbouring bytes
        eval_i64(".data; D: .zero 255; .code; push D; push 0xFF; store_u8; push D; push 1; add_u64; push 0x1122334455667788; store_u64; push D; load_u8; ret;", 0xFF);
    }

    #[test]
    fn test_setn()
    {
        // Store instruction
        eval_i64(".code; push 3; push 0; push 7; setn 1; pop; ret;", 7);
    }

    #[test]
    fn test_call_ret()
    {
        eval_i64("call FN, 0; ret; FN: push_i8 33; ret;", 33);
        eval_i64("push_i8 3; call FN, 1; ret; FN: get_arg 0; push_i8 1; add_u64; ret;", 4);

        // set_arg
        eval_i64("push_i8 3; call FN, 1; ret; FN: push 7; set_arg 0; get_arg 0; ret;", 7);

        // Two arguments and subtract (order of arguments matters)
        eval_i64("push_i8 7; push 5; call FN, 2; ret; FN: get_arg 0; get_arg 1; sub_u64; ret;", 2);

        // Recursive decrement function
        eval_i64("push 10; call DEC, 1; ret; DEC: get_arg 0; dup; jz ZERO; push 1; sub_u64; call DEC, 1; ret; ZERO: ret;", 0);

        // Regression: stack corruption
        eval_i64("push 5; call foo, 0; pop; ret; foo: push 2; push 0; ret;", 5);
    }

    #[test]
    fn test_call_fp()
    {
        eval_i64(" push FN; call_fp 0; ret; FN: push_i8 33; ret;", 33);
    }

    #[test]
    fn test_patch_fp()
    {
        // Patch a function's code address into the data segment at runtime,
        // then load it back and call it via call_fp
        eval_i64(
            ".data; FPTR: .u64 0; .code; push FPTR; push FN; store_u64; push FPTR; load_u64; call_fp 0; ret; FN: push_i8 33; ret;",
            33
        );

        // Same, but the code address is assembled into the data segment
        // statically using the .addr64 directive
        eval_i64(
            ".data; FPTR: .addr64 FN; .code; push FPTR; load_u64; call_fp 0; ret; FN: push_i8 33; ret;",
            33
        );
    }

    #[test]
    fn test_syscalls()
    {
        eval_src(".data; LABEL: .zero 256; .code; push LABEL; push 255; push 0; syscall memset; push 0; ret;");
    }

    #[test]
    fn test_cmd_args()
    {
        // Argument 0 is the program path, the rest are user-supplied args.
        // PROGRAM_ARGS is a set-once global, so this must be the only test
        // that sets it.
        crate::host::set_program_args(vec![
            "prog.asm".to_string(),
            "hello".to_string(),
            "wide".to_string(),
        ]);

        // cmd_argc returns the number of arguments
        eval_i64(".code; syscall cmd_argc; ret;", 3);

        // cmd_get_arg copies arg 1 ("hello") into the buffer and returns its length
        eval_i64(".data; BUF: .zero 64; .code; push 1; push BUF; push 64; syscall cmd_get_arg; ret;", 5);

        // The argument bytes are actually written into the buffer ('h' == 104)
        eval_i64(".data; BUF: .zero 64; .code; push 1; push BUF; push 64; syscall cmd_get_arg; push BUF; load_u8; ret;", 'h' as i64);

        // A dst_len of 0 queries the length without writing to the buffer
        eval_i64(".data; BUF: .zero 64; .code; push 2; push BUF; push 0; syscall cmd_get_arg; ret;", 4);
    }

    #[test]
    #[should_panic(expected = "invalid opcode")]
    fn test_invalid_opcode()
    {
        // A computed jump (call_fp) into the operand bytes of an instruction
        // lands on bytes that are not valid opcodes. This must fault cleanly
        // rather than decode an invalid Op value, which would be undefined
        // behaviour. BAD+1 is the first 0xFF operand byte of the push_u64.
        eval_src("push BAD; push 1; add_u64; call_fp 0; ret; BAD: push_u64 0xFFFF_FFFF_FFFF_FFFF; ret;");
    }

    #[test]
    #[should_panic]
    fn test_div_zero()
    {
        eval_src("push 8; push 0; div_u64; ret;");
    }

    #[test]
    #[should_panic]
    fn test_ret_none()
    {
        eval_src("call FN, 0; ret; FN: ret;");
    }

    #[test]
    #[should_panic]
    fn test_get_arg_none()
    {
        eval_src("call FN, 0; ret; FN: get_arg 0; push 0; ret;");
    }

    #[test]
    #[should_panic]
    fn test_load_oob()
    {
        eval_src(".data; .fill 1000, 0; .code; push 100_000_000; load_u64; ret;");
    }

    #[test]
    #[should_panic]
    fn test_memset_oob()
    {
        eval_src(".data; LABEL: .zero 1; .code; push LABEL; push 255; push 100_000_000; syscall memset; push 0; ret;");
    }

    // Regression: this used to segfault
    #[test]
    #[should_panic]
    fn test_memcmp_n1()
    {
        eval_src(".data; A: .zero 10; B: .zero 10; .code; push A; push B; push -1; syscall memcpy;");
    }

    // Regression: an address near usize::MAX used to make the bounds check
    // `addr + size_of::<T>() > cur_size` wrap around to a small value, pass,
    // and then form a wild host pointer (sandbox escape). It must panic.
    #[test]
    #[should_panic]
    fn test_load_addr_wraparound()
    {
        eval_src(".data; .fill 1000, 0; .code; push 0xFFFFFFFFFFFFFFF8; load_u64; ret;");
    }

    #[test]
    #[should_panic]
    fn test_store_addr_wraparound()
    {
        eval_src(".data; .fill 1000, 0; .code; push 0xFFFFFFFFFFFFFFF8; push 0; store_u64; ret;");
    }
}
