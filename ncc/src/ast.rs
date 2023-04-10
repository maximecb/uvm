use std::rc::Rc;
use std::cell::RefCell;
use std::fmt;

// TODO: we may want a const type
#[derive(Clone, Debug)]
pub enum Type
{
    Void,

    UInt(usize),
    Int(usize),
    Float(usize),

    Pointer(Box<Type>),

    Array {
        elem_type: Box<Type>,
        size_expr: Box<Expr>,
    },

    Fun {
        ret_type: Box<Type>,
        param_types: Vec<Type>,
        var_arg: bool,
    },

    Struct {
        fields: Vec<(String, Type)>,
    },

    // Unresolved named reference to a typedef
    Named(String),

    // Reference to a typedef
    // This is used to handle cyclic types
    Ref(Rc<Box<RefCell<Type>>>),
}

impl Type
{
    pub fn eq(&self, other: &Type) -> bool
    {
        use Type::*;
        match (self, other) {
            (Void, Void) => true,
            (UInt(m), UInt(n)) if m == n => true,
            (Int(m), Int(n)) if m == n => true,
            (Float(m), Float(n)) if m == n => true,
            (Pointer(ta), Pointer(tb)) => ta.eq(tb),

            (Array { elem_type: elem_ta, size_expr: size_a }, Array { elem_type: elem_tb, size_expr: size_b })  => {
                if !elem_ta.eq(elem_tb) {
                    false
                } else {
                    match (size_a.as_ref(), size_b.as_ref()) {
                        (Expr::Int(a), Expr::Int(b)) => a == b,
                        _ => panic!()
                    }
                }
            }

            (Struct { fields: f_a }, Struct { fields: f_b }) => {
                if f_a.len() != f_b.len() {
                    return false;
                }

                for (idx, (na, ta)) in f_a.iter().enumerate() {
                    let (nb, tb) = &f_b[idx];

                    if na != nb {
                        return false;
                    }

                    if !ta.eq(tb) {
                        return false;
                    }
                }

                true
            }

            _ => false
        }
    }

    /// Produce the size of this type in bits
    /// Valid for pointer/integer/float types only
    pub fn num_bits(&self) -> usize
    {
        use Type::*;
        match self {
            UInt(num_bits) | Int(num_bits) | Float(num_bits) => *num_bits,
            Pointer(_) => 64,
            _ => panic!()
        }
    }

    /// Produce the size of this type in bytes
    pub fn sizeof(&self) -> usize
    {
        use Type::*;
        match self {
            Void => panic!(),
            UInt(num_bits) | Int(num_bits) | Float(num_bits) => num_bits / 8,
            Pointer(_) => 8,

            Array { elem_type, size_expr } => {
                match size_expr.as_ref() {
                    Expr::Int(num_elems) => {
                        usize::try_from(*num_elems).unwrap() * elem_type.sizeof()
                    }
                    _ => panic!()
                }
            }

            Struct { fields } => {
                let mut num_bytes: usize = 0;

                for (_, t) in fields {
                    // Align the field
                    let field_align = t.align_bytes();
                    num_bytes = (num_bytes + (field_align - 1)) & !(field_align - 1);

                    // Add the field size
                    num_bytes += t.sizeof();
                }

                num_bytes
            }

            _ => panic!("sizeof {:?}", self)
        }
    }

    /// Alignment for the type in bytes
    pub fn align_bytes(&self) -> usize
    {
        use Type::*;
        match self {
            UInt(num_bits) | Int(num_bits) | Float(num_bits) => num_bits / 8,
            Pointer(_) => 8,
            Array { elem_type, .. } => elem_type.align_bytes(),

            Struct { fields } => {
                let mut max_align = 0;
                for (name, t) in fields {
                    max_align = max_align.max(t.align_bytes());
                }
                max_align
            }

            _ => panic!()
        }
    }

    pub fn elem_type(&self) -> Type
    {
        use Type::*;
        match self {
            Pointer(t) => *t.clone(),
            _ => panic!()
        }
    }

    pub fn is_signed(&self) -> bool
    {
        use Type::*;
        match self {
            Int(_) => true,
            UInt(_) => false,
            Pointer(_) => false,
            Array{..} => false,
            _ => panic!("is_signed {:?}", self)
        }
    }
}

impl fmt::Display for Type {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        use Type::*;
        match self {
            Void => write!(f, "void"),
            UInt(n) => write!(f, "u{}", n),
            Int(n) => write!(f, "i{}", n),
            Float(n) => write!(f, "f{}", n),
            Pointer(t) => write!(f, "{}*", t.as_ref()),
            Array { elem_type, size_expr } => write!(f, "{}[]", elem_type.as_ref()),
            Struct { .. } => write!(f, "struct"),
            _ => todo!()
        }
    }
}

/// Variable/function Declaration
#[derive(Clone, Debug)]
pub enum Decl
{
    Global { name: String, t: Type },
    Arg { idx: usize, t: Type },
    Local { idx: usize, t: Type },
    Fun { name: String, t: Type },
    TypeDef { name: String, t: Rc<Box<RefCell<Type>>> },
}

impl Decl
{
    pub fn get_type(&self) -> Type
    {
        match self {
            Decl::Global { name, t } => t.clone(),
            Decl::Arg { idx, t } => t.clone(),
            Decl::Local { idx, t } => t.clone(),
            Decl::Fun { name, t } => t.clone(),
            Decl::TypeDef { name, t } => t.borrow().clone(),
        }
    }
}

/// Unary operator
#[derive(Copy, Clone, Debug, PartialEq)]
pub enum UnOp
{
    Minus,
    Not,
    BitNot,

    Deref,
    AddressOf,
}

/// Binary operator
/// https://en.cppreference.com/w/c/language/operator_precedence
#[derive(Copy, Clone, Debug, PartialEq)]
pub enum BinOp
{
    // Struct member access
    Arrow,
    Member,

    // Bitwise
    BitAnd,
    BitOr,
    BitXor,
    LShift,
    RShift,

    // Arithmetic
    Add,
    Sub,
    Mul,
    Div,
    Mod,

    // Comparison
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,

    // Logical and, logical or
    And,
    Or,

    // Assignment
    Assign,

    // Sequencing (a, b)
    Comma,
}

/// Expression
#[derive(Clone, Debug)]
pub enum Expr
{
    Int(i128),
    String(String),
    Float32(f32),

    // Array literal
    Array(Vec<Expr>),

    Ident(String),

    // Reference to a variable/function declaration
    Ref(Decl),

    // Type casting expression
    Cast {
        new_type: Type,
        child: Box<Expr>
    },

    SizeofExpr {
        child: Box<Expr>
    },

    SizeofType {
        t: Type
    },

    Unary {
        op: UnOp,
        child: Box<Expr>,
    },

    Binary {
        op: BinOp,
        lhs: Box<Expr>,
        rhs: Box<Expr>,
    },

    Ternary {
        test_expr: Box<Expr>,
        then_expr: Box<Expr>,
        else_expr: Box<Expr>,
    },

    Call {
        callee: Box<Expr>,
        args: Vec<Expr>,
    },

    // Inline assembly
    Asm {
        text: String,
        args: Vec<Expr>,
        out_type: Type,
    }
}

/// Statement
#[derive(Clone, Debug)]
pub enum Stmt
{
    Expr(Expr),

    ReturnExpr(Box<Expr>),
    ReturnVoid,

    Break,
    Continue,

    Block(Vec<Stmt>),

    If {
        test_expr: Expr,
        then_stmt: Box<Stmt>,
        else_stmt: Option<Box<Stmt>>,
    },

    While {
        test_expr: Expr,
        body_stmt: Box<Stmt>,
    },

    DoWhile {
        body_stmt: Box<Stmt>,
        test_expr: Expr,
    },

    For {
        init_stmt: Option<Box<Stmt>>,
        test_expr: Expr,
        incr_expr: Expr,
        body_stmt: Box<Stmt>,
    },

    /// Local variable declaration
    VarDecl {
        var_type: Type,
        var_name: String,
        init_expr: Option<Expr>,
    }
}

/// Function
#[derive(Clone, Debug)]
pub struct Function
{
    /// Name of the function
    pub name: String,

    // Return type
    pub ret_type: Type,

    /// Parameter list
    pub params: Vec<(Type, String)>,

    /// Variadic function, variable argument count
    pub var_arg: bool,

    /// Inline attribute
    pub inline: bool,

    /// Body of the function
    pub body: Stmt,

    /// Number of local variables
    pub num_locals: usize,
}

impl Function
{
    /// Get a type representing the function signature
    pub fn get_type(&self) -> Type
    {
        Type::Fun {
            ret_type: Box::new(self.ret_type.clone()),
            param_types: self.params.iter().map(|p| p.0.clone()).collect(),
            var_arg: self.var_arg,
        }
    }
}

/// Global variable declaration
#[derive(Clone, Debug)]
pub struct Global
{
    /// Name of the variable
    pub name: String,

    // Return type
    pub var_type: Type,

    // Initialization expression
    pub init_expr: Option<Expr>,
}

/// Top-level unit (e.g. source file)
#[derive(Default, Clone, Debug)]
pub struct Unit
{
    pub typedefs: Vec<(String, Rc<Box<RefCell<Type>>>)>,

    pub global_vars: Vec<Global>,

    pub fun_decls: Vec<Function>,
}
