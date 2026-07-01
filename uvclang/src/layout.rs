//! Type layout: sizes, alignments, and struct field offsets.
//!
//! UVM is a 64-bit machine and `doom.ll` uses the x86_64 datalayout, so this
//! is the standard C layout: pointers are 8 bytes, integers have natural
//! alignment, structs use field padding, arrays stride by the (padded)
//! element size.

use crate::ast::{Module, StructBody, Type};
use rustc_hash::FxHashMap as HashMap;

pub struct Layout<'a>
{
    structs: &'a HashMap<String, Option<StructBody>>,
}

impl<'a> Layout<'a>
{
    pub fn new(module: &'a Module) -> Self
    {
        Layout { structs: &module.struct_types }
    }

    /// Size in bytes a value of this type occupies in memory, including any
    /// trailing padding (so arrays stride correctly).
    pub fn size_of(&self, ty: &Type) -> u64
    {
        match ty {
            Type::Void => 0,
            Type::Int(w) => int_bytes(*w),
            Type::Ptr => 8,
            Type::Array { len, elem } => len * self.size_of(elem),
            Type::Struct(body) => self.struct_size(body),
            Type::NamedStruct(name) => self.struct_size(self.resolve(name)),
            Type::Func(_) | Type::Label | Type::Metadata => {
                panic!("type has no storage size: {:?}", ty)
            }
        }
    }

    /// Alignment in bytes required by this type.
    pub fn align_of(&self, ty: &Type) -> u64
    {
        match ty {
            Type::Void => 1,
            Type::Int(w) => int_bytes(*w),
            Type::Ptr => 8,
            Type::Array { elem, .. } => self.align_of(elem),
            Type::Struct(body) => self.struct_align(body),
            Type::NamedStruct(name) => self.struct_align(self.resolve(name)),
            Type::Func(_) | Type::Label | Type::Metadata => {
                panic!("type has no alignment: {:?}", ty)
            }
        }
    }

    /// Byte offset of field `idx` within a struct.
    pub fn field_offset(&self, body: &StructBody, idx: usize) -> u64
    {
        let mut offset = 0u64;
        for (i, field) in body.fields.iter().enumerate() {
            if !body.packed {
                offset = align_up(offset, self.align_of(field));
            }
            if i == idx {
                return offset;
            }
            offset += self.size_of(field);
        }
        panic!("field index {} out of range", idx);
    }

    /// Resolve a named struct to its body.
    pub fn resolve(&self, name: &str) -> &'a StructBody
    {
        match self.structs.get(name) {
            Some(Some(body)) => body,
            Some(None) => panic!("opaque struct %{} has no layout", name),
            None => panic!("undefined struct type %{}", name),
        }
    }

    fn struct_size(&self, body: &StructBody) -> u64
    {
        let mut offset = 0u64;
        let mut max_align = 1u64;
        for field in &body.fields {
            let a = self.align_of(field);
            if !body.packed {
                offset = align_up(offset, a);
                max_align = max_align.max(a);
            }
            offset += self.size_of(field);
        }
        if body.packed {
            offset
        } else {
            align_up(offset, max_align)
        }
    }

    fn struct_align(&self, body: &StructBody) -> u64
    {
        if body.packed {
            return 1;
        }
        body.fields.iter().map(|f| self.align_of(f)).max().unwrap_or(1)
    }
}

/// Storage size (in bytes) of an integer of `w` bits (`i1` is one byte).
fn int_bytes(w: u32) -> u64
{
    match w {
        1 | 8 => 1,
        16 => 2,
        32 => 4,
        64 => 8,
        128 => 16,
        // Not produced by C, but keep a sane fallback.
        _ => ((w as u64) + 7) / 8,
    }
}

/// Round `off` up to a multiple of `align` (a power of two).
fn align_up(off: u64, align: u64) -> u64
{
    (off + align - 1) & !(align - 1)
}

#[cfg(test)]
mod tests
{
    use super::*;
    use crate::ast::{Module, StructBody, Type};

    fn i(w: u32) -> Type { Type::Int(w) }
    fn array(len: u64, elem: Type) -> Type { Type::Array { len, elem: Box::new(elem) } }
    fn strukt(packed: bool, fields: Vec<Type>) -> Type { Type::Struct(StructBody { packed, fields }) }

    fn layout() -> Layout<'static>
    {
        // Leak an empty module so the borrow is 'static for test convenience.
        let m: &'static Module = Box::leak(Box::new(Module::default()));
        Layout::new(m)
    }

    #[test]
    fn scalars()
    {
        let l = layout();
        assert_eq!((l.size_of(&i(1)), l.align_of(&i(1))), (1, 1));
        assert_eq!((l.size_of(&i(8)), l.align_of(&i(8))), (1, 1));
        assert_eq!((l.size_of(&i(16)), l.align_of(&i(16))), (2, 2));
        assert_eq!((l.size_of(&i(32)), l.align_of(&i(32))), (4, 4));
        assert_eq!((l.size_of(&i(64)), l.align_of(&i(64))), (8, 8));
        assert_eq!((l.size_of(&Type::Ptr), l.align_of(&Type::Ptr)), (8, 8));
    }

    #[test]
    fn arrays()
    {
        let l = layout();
        assert_eq!(l.size_of(&array(5, i(32))), 20);
        assert_eq!(l.align_of(&array(5, i(32))), 4);
        // Array of array: [2 x [4 x i32]] = 32 bytes.
        assert_eq!(l.size_of(&array(2, array(4, i(32)))), 32);
    }

    #[test]
    fn structs()
    {
        let l = layout();
        // { i32, i32 } -> size 8, align 4, offsets 0 and 4.
        let s = StructBody { packed: false, fields: vec![i(32), i(32)] };
        assert_eq!((l.struct_size(&s), l.struct_align(&s)), (8, 4));
        assert_eq!((l.field_offset(&s, 0), l.field_offset(&s, 1)), (0, 4));

        // { i8, i32 } -> i8@0, pad, i32@4, size 8.
        let s = StructBody { packed: false, fields: vec![i(8), i(32)] };
        assert_eq!(l.struct_size(&s), 8);
        assert_eq!((l.field_offset(&s, 0), l.field_offset(&s, 1)), (0, 4));

        // { i32, i8 } -> i32@0, i8@4, pad to 8.
        let s = StructBody { packed: false, fields: vec![i(32), i(8)] };
        assert_eq!(l.struct_size(&s), 8);
        assert_eq!((l.field_offset(&s, 0), l.field_offset(&s, 1)), (0, 4));

        // { i8, i8, i32 } -> i8@0, i8@1, pad, i32@4, size 8.
        let s = StructBody { packed: false, fields: vec![i(8), i(8), i(32)] };
        assert_eq!(l.struct_size(&s), 8);
        assert_eq!(l.field_offset(&s, 2), 4);

        // { i32, [3 x i8] } -> i32@0, arr@4 (size 3), total 7 -> pad to 8.
        let s = StructBody { packed: false, fields: vec![i(32), array(3, i(8))] };
        assert_eq!(l.struct_size(&s), 8);
        assert_eq!(l.struct_align(&s), 4);
    }

    #[test]
    fn packed_struct()
    {
        let l = layout();
        // <{ i8, i32 }> -> no padding: size 5, align 1, offsets 0 and 1.
        let s = StructBody { packed: true, fields: vec![i(8), i(32)] };
        assert_eq!((l.struct_size(&s), l.struct_align(&s)), (5, 1));
        assert_eq!((l.field_offset(&s, 0), l.field_offset(&s, 1)), (0, 1));
    }

    #[test]
    fn named_struct()
    {
        let mut m = Module::default();
        m.struct_types.insert("pair".to_string(), Some(StructBody { packed: false, fields: vec![i(32), i(32)] }));
        let l = Layout::new(&m);
        let named = Type::NamedStruct("pair".to_string());
        assert_eq!((l.size_of(&named), l.align_of(&named)), (8, 4));
        // Array of a named struct strides by its size.
        assert_eq!(l.size_of(&array(3, named)), 24);
    }

    #[test]
    fn nested_via_named()
    {
        // struct line { point a, b }; struct point { i32 x, y }  -> line is 16 bytes
        let mut m = Module::default();
        m.struct_types.insert("point".to_string(), Some(StructBody { packed: false, fields: vec![i(32), i(32)] }));
        m.struct_types.insert(
            "line".to_string(),
            Some(StructBody { packed: false, fields: vec![Type::NamedStruct("point".to_string()), Type::NamedStruct("point".to_string())] }),
        );
        let l = Layout::new(&m);
        let line = Type::NamedStruct("line".to_string());
        assert_eq!(l.size_of(&line), 16);
        assert_eq!(l.field_offset(l.resolve("line"), 1), 8);
    }
}
