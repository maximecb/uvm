//! Structured representation of the subset of LLVM IR we compile to UVM.
//!
//! This intentionally models only what clang `-O2` output for C actually
//! emits (see the analysis of doom.ll): integer & pointer types, arrays and
//! structs, `float`, and a fixed set of instructions. `double` is parsed but
//! only its storage/movement is supported (UVM has f32 ops only). No vectors.
//!
//! Attributes that don't affect code generation (linkage, `nounwind`,
//! `captures(...)`, parameter attrs, metadata, ...) are dropped during
//! parsing and have no representation here.

use rustc_hash::FxHashMap as HashMap;

/// A whole translation unit.
#[derive(Debug, Default)]
pub struct Module
{
    pub source_filename: Option<String>,
    pub datalayout: Option<String>,
    pub target_triple: Option<String>,

    /// Named struct types: `%struct.foo = type { ... }`.
    /// `None` body means an opaque/forward-declared struct.
    pub struct_types: HashMap<String, Option<StructBody>>,

    pub globals: Vec<Global>,
    pub functions: Vec<Function>,
}

// ===========================================================================
// Types
// ===========================================================================

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Type
{
    Void,
    /// Integer of the given bit width: `i1`, `i8`, `i32`, ...
    Int(u32),
    /// `float` (IEEE-754 binary32). Maps to the UVM `*_f32` ops.
    Float,
    /// `double` (IEEE-754 binary64). UVM has no f64 ops, so only storage and
    /// movement of doubles is supported; arithmetic on them is rejected.
    Double,
    /// Opaque pointer: `ptr`.
    Ptr,
    /// `[N x T]`
    Array { len: u64, elem: Box<Type> },
    /// A reference to a named struct type, e.g. `%struct.foo`.
    NamedStruct(String),
    /// An inline (literal) struct type: `{ ... }` or packed `<{ ... }>`.
    Struct(StructBody),
    /// A function type, e.g. `i32 (ptr, ...)`.
    Func(Box<FuncType>),
    /// `label` (basic block reference).
    Label,
    /// `metadata` (only appears in dropped positions, kept for completeness).
    Metadata,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct StructBody
{
    pub packed: bool,
    pub fields: Vec<Type>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FuncType
{
    pub ret: Type,
    pub params: Vec<Type>,
    pub varargs: bool,
}

// ===========================================================================
// Values & constants
// ===========================================================================

/// A value operand. The associated type is supplied by the enclosing
/// instruction (LLVM writes the type once, e.g. `add i32 %a, %b`), except
/// for constant expressions and aggregates which carry their own types.
#[derive(Debug, Clone)]
pub enum Value
{
    /// `%name` or `%1` — an SSA local or basic-block-local reference.
    Local(String),
    /// `@name` — a global / function reference.
    Global(String),
    /// An integer literal (`true`/`false` are normalized to 1/0).
    Int(i128),
    /// A floating-point literal. Stored as f64 (LLVM prints even `float`
    /// constants as the exact double they widen to); narrowed to f32 at use.
    Float(f64),
    /// `null`
    Null,
    /// `undef`
    Undef,
    /// `poison`
    Poison,
    /// `zeroinitializer`
    ZeroInit,
    /// A constant expression, e.g. `getelementptr (...)`.
    ConstExpr(Box<ConstExpr>),
    /// `c"..."` byte-string constant.
    CStr(Vec<u8>),
    /// `[ ... ]` array constant.
    Array(Vec<TypedVal>),
    /// `{ ... }` struct constant.
    Struct(Vec<TypedVal>),
}

/// A `<type> <value>` pair, as written in LLVM text wherever the type is
/// spelled out explicitly (global initializers, aggregate elements, call
/// arguments, gep indices, const-expr operands, ...).
#[derive(Debug, Clone)]
pub struct TypedVal
{
    pub ty: Type,
    pub val: Value,
}

#[derive(Debug, Clone)]
pub enum ConstExpr
{
    /// `getelementptr [inbounds] (base_ty, ptr <base>, <indices...>)`
    GetElementPtr {
        inbounds: bool,
        base_ty: Type,
        base: TypedVal,
        indices: Vec<TypedVal>,
    },
    /// A conversion applied to a constant: `trunc`/`zext`/`sext`/`ptrtoint`/
    /// `inttoptr`/`bitcast`, e.g. `ptrtoint (ptr @g to i32)`.
    Conv { op: ConvOp, val: TypedVal, to: Type },
    /// A binary operation on two constants, e.g. `add (i32 <a>, i32 <b>)`.
    Bin { op: BinOp, lhs: TypedVal, rhs: TypedVal },
}

// ===========================================================================
// Globals & functions
// ===========================================================================

#[derive(Debug)]
pub struct Global
{
    pub name: String,
    pub ty: Type,
    /// `constant` vs `global`.
    pub is_const: bool,
    /// Initializer, or `None` for an external declaration.
    pub init: Option<Value>,
}

#[derive(Debug)]
pub struct Param
{
    pub ty: Type,
    /// Parameter names are absent on `declare` lines.
    pub name: Option<String>,
}

#[derive(Debug)]
pub struct Function
{
    pub name: String,
    pub ret_ty: Type,
    pub params: Vec<Param>,
    pub varargs: bool,
    /// Empty for a `declare` (no body).
    pub blocks: Vec<BasicBlock>,
}

impl Function
{
    pub fn is_decl(&self) -> bool { self.blocks.is_empty() }
}

#[derive(Debug)]
pub struct BasicBlock
{
    pub label: String,
    /// Non-terminator instructions, in order.
    pub insts: Vec<Inst>,
    /// The block-ending terminator.
    pub term: Terminator,
}

// ===========================================================================
// Instructions
// ===========================================================================

/// A non-terminator instruction with its optional result name (the `%x =`).
#[derive(Debug)]
pub struct Inst
{
    /// Result SSA name (without the `%`), or `None` for value-less insts
    /// such as `store`.
    pub dest: Option<String>,
    pub kind: InstKind,
}

#[derive(Debug)]
pub enum InstKind
{
    /// Integer binary op: `add`, `sub`, `mul`, `udiv`, `sdiv`, `urem`,
    /// `srem`, `and`, `or`, `xor`, `shl`, `lshr`, `ashr`.
    Bin { op: BinOp, ty: Type, lhs: Value, rhs: Value },
    /// Floating-point binary op: `fadd`, `fsub`, `fmul`, `fdiv`, `frem`.
    FBin { op: FBinOp, ty: Type, lhs: Value, rhs: Value },
    /// `fneg <ty> <val>` — floating-point negation.
    FNeg { ty: Type, val: Value },
    /// Width/representation conversions: `trunc`, `zext`, `sext`,
    /// `ptrtoint`, `inttoptr`, and the int<->float / float<->float casts
    /// (`sitofp`, `uitofp`, `fptosi`, `fptoui`, `fpext`, `fptrunc`).
    Conv { op: ConvOp, from_ty: Type, val: Value, to_ty: Type },
    /// `icmp <pred> <ty> <lhs>, <rhs>`
    ICmp { pred: ICmpPred, ty: Type, lhs: Value, rhs: Value },
    /// `fcmp <pred> <ty> <lhs>, <rhs>`
    FCmp { pred: FCmpPred, ty: Type, lhs: Value, rhs: Value },
    /// `select i1 <cond>, <ty> <tval>, <ty> <fval>`
    Select { cond: Value, ty: Type, tval: Value, fval: Value },
    /// `load <ty>, ptr <ptr>`
    Load { ty: Type, ptr: Value, align: Option<u64> },
    /// `store <ty> <val>, ptr <ptr>`
    Store { ty: Type, val: Value, ptr: Value, align: Option<u64> },
    /// `alloca <ty>[, <ty> <count>]`
    Alloca { ty: Type, count: Option<TypedVal>, align: Option<u64> },
    /// `getelementptr [inbounds] <base_ty>, ptr <ptr>, <indices...>`
    GetElementPtr {
        inbounds: bool,
        base_ty: Type,
        ptr: Value,
        indices: Vec<TypedVal>,
    },
    /// `phi <ty> [ <val>, <bb> ], ...`
    Phi { ty: Type, incoming: Vec<(Value, String)> },
    /// `call <ret_ty> [<fn_ty>] <callee>(<args>)`
    Call {
        ret_ty: Type,
        /// Present when an explicit function (pointer) type is spelled out,
        /// e.g. `call void (...) %fp()`.
        fn_ty: Option<FuncType>,
        callee: Value,
        args: Vec<TypedVal>,
    },
    /// `freeze <ty> <val>`
    Freeze { ty: Type, val: Value },
    /// `load atomic <ty>, ptr <ptr> <ordering>` — an atomic load. Only 64-bit
    /// types are supported (the UVM atomic ops are 64 bits wide).
    AtomicLoad { ty: Type, ptr: Value },
    /// `store atomic <ty> <val>, ptr <ptr> <ordering>` — an atomic store.
    AtomicStore { ty: Type, val: Value, ptr: Value },
    /// `atomicrmw <op> ptr <ptr>, <ty> <val> <ordering>` — an atomic
    /// read-modify-write. Lowered to a compare-and-swap retry loop; the result
    /// is the value read before the update (the "old" value).
    AtomicRmw { op: RmwOp, ty: Type, ptr: Value, val: Value },
    /// `cmpxchg [weak] ptr <ptr>, <ty> <cmp>, <ty> <new> <succ> <fail>`.
    /// Produces the aggregate `{ <ty>, i1 }` = (value read, success flag),
    /// consumed by `extractvalue`.
    Cmpxchg { ty: Type, ptr: Value, cmp: Value, new: Value },
    /// `extractvalue <agg-ty> <agg>, <index>` — read one field of an aggregate.
    /// Only supported on `cmpxchg` results (index 0 = old value, 1 = success).
    ExtractValue { agg: Value, index: u32 },
    /// `fence <ordering>` — a standalone memory fence. The UVM atomic ops carry
    /// their own acquire/release ordering, so this lowers to no code.
    Fence,
}

/// Operation performed by an `atomicrmw` instruction.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RmwOp
{
    Xchg, Add, Sub, And, Or, Xor, Nand,
}

#[derive(Debug)]
pub enum Terminator
{
    /// `ret void` or `ret <ty> <val>`
    Ret { val: Option<TypedVal> },
    /// `br label <dest>` or `br i1 <cond>, label <t>, label <f>`
    Br { cond: Option<Value>, if_true: String, if_false: Option<String> },
    /// `switch <ty> <val>, label <default> [ <ty> <c>, label <dest> ... ]`
    Switch {
        ty: Type,
        val: Value,
        default: String,
        cases: Vec<(i128, String)>,
    },
    /// `unreachable`
    Unreachable,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BinOp
{
    Add, Sub, Mul, UDiv, SDiv, URem, SRem,
    And, Or, Xor, Shl, LShr, AShr,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FBinOp
{
    FAdd, FSub, FMul, FDiv, FRem,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConvOp
{
    Trunc, ZExt, SExt, PtrToInt, IntToPtr, BitCast,
    /// signed-int -> float, unsigned-int -> float
    SIToFP, UIToFP,
    /// float -> signed-int, float -> unsigned-int
    FPToSI, FPToUI,
    /// float -> double (fpext), double -> float (fptrunc)
    FPExt, FPTrunc,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ICmpPred
{
    Eq, Ne, Ugt, Uge, Ult, Ule, Sgt, Sge, Slt, Sle,
}

/// Floating-point comparison predicate (`fcmp`). The `o*` forms are ordered
/// (false if either operand is NaN); the `u*` forms are unordered (true if
/// either is NaN). `Ord`/`Uno` test only for the absence/presence of NaN.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FCmpPred
{
    False, Oeq, Ogt, Oge, Olt, Ole, One, Ord,
    Ueq, Ugt, Uge, Ult, Ule, Une, Uno, True,
}
