use std::collections::HashSet;
use std::mem::transmute;
use crate::vm::Op;

pub struct ByteArray
{
    data: Vec<u8>
}

impl ByteArray
{
    pub fn new() -> Self
    {
        Self {
            data: Vec::default()
        }
    }

    /// Get the memory block size in bytes
    pub fn len(&self) -> usize
    {
        self.data.len()
    }

    pub fn push_op(&mut self, op: Op)
    {
        self.data.push(op as u8);
    }

    pub fn push_u8(&mut self, val: u8)
    {
        self.data.push(val);
    }

    pub fn push_u16(&mut self, val: u16)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    pub fn push_i8(&mut self, val: i8)
    {
        self.data.push(val as u8);
    }

    pub fn push_i32(&mut self, val: i32)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    pub fn push_u32(&mut self, val: u32)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    pub fn push_u64(&mut self, val: u64)
    {
        for byte in val.to_le_bytes() {
            self.data.push(byte);
        }
    }

    /// Write a value at the given address
    pub fn write<T>(&mut self, pos: usize, val: T) where T: Copy
    {
        unsafe {
            let buf_ptr = self.data.as_mut_ptr();
            let val_ptr = transmute::<*mut u8 , *mut T>(buf_ptr.add(pos));
            std::ptr::write_unaligned(val_ptr, val);
        }
    }

    /*
    /// Resize to a new size in bytes
    pub fn resize(&mut self, mut num_bytes: usize) -> usize
    {
        // Round up to a page size multiple
        let page_size = unsafe { libc::sysconf(libc::_SC_PAGESIZE) } as usize;
        assert!(page_size % 8 == 0);
        let rem = num_bytes % page_size;
        if rem != 0 {
            num_bytes += page_size - rem;
        }

        assert!(num_bytes % page_size == 0);
        self.data.resize(num_bytes, 0);

        num_bytes
    }
    */

    /*
    /// Read a value at the current PC and then increment the PC
    pub fn read_pc<T>(&self, pc: &mut usize) -> T where T: Copy
    {
        unsafe {
            let buf_ptr = self.data.as_ptr();
            let val_ptr = transmute::<*const u8 , *const T>(buf_ptr.add(*pc));
            *pc += size_of::<T>();
            std::ptr::read_unaligned(val_ptr)
        }
    }
    */
}

pub struct Program
{
    // Executable code
    pub code: ByteArray,

    // Data segment
    pub data: ByteArray,

    // Set of syscalls referenced by this program
    pub syscalls: HashSet<u16>,
}
