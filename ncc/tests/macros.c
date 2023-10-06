#include <assert.h>

#define FOO()
#define BAR(x) x

#define BIF 1
#undef BIF
#define BIF 2

#define EOF (-1)

#define MACRO2(a, b) (a+b)

// Recursive macro
#define REC_MACRO2 3
#define REC_MACRO REC_MACRO2

// Definition including counter
#define CTR_MACRO __COUNTER__

// Regression: definition containing a \ character in a string
#define newline_str "\n"

// Regression: quote in comment inside definition
#define K 1 // '

// Regression: macro replacement should not happen within a string
#define SUB_STR_MACRO "foo"
#define STR_MACRO "macro SUB_STR_MACRO"

// Regression: argument names contain parameter name
#define rgba32(b, a) (b | a)
int regress_rgba(int chroma, int alpha)
{
    return rgba32(alpha, chroma);
}

void main()
{
    int l = __LINE__;
    char* f = __FILE__;

    // Macro with zero arguments
    FOO();
    FOO ();
    FOO( );

    // Macro with one argument
    BAR(1);
    BAR( 1 );
    BAR((1));
    BAR((1, 2));
    BAR(3 + (1, 2));

    // Definition with parens, but not a macro
    assert(EOF == -1);

    // Macro applied over multiple lines
    MACRO2(
        1,
        2
    );

    MACRO2(
        1, // comment in macro args
        2  /* multi-line comment */
    );

    // This macro isn't expanded because we supply no arguments
    int MACRO2 = 3;
    assert(MACRO2 == 3);

    // Definition including the counter macro
    int c0 = CTR_MACRO;
    int c1 = CTR_MACRO;
    int c2 = CTR_MACRO;
    assert(c1 > c0);
    assert(c2 > c1);

    // Regression: closing parens inside a string
    BAR(")");
    BAR("\")\"");
    BAR(",");
    BAR(',');
    BAR('"');

    // Recursive macro
    REC_MACRO;

    assert(newline_str[0] == '\n');

    #ifdef FOO
    assert(true);
    #else
    assert(false);
    #endif

    #ifndef NOT_DEFINED_LOL
    assert(true);
    #else
    assert(false);
    #endif
}
