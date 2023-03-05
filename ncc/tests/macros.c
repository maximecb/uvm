#include <assert.h>

#define FOO()
#define BAR(x) x

#define BIF 1
#undef BIF
#define BIF 2

#define MACRO2(a, b) (a+b)

// Recursive macro
#define REC_MACRO2 3
#define REC_MACRO REC_MACRO2

// Regression: definition containing a \ character in a string
#define newline_str "\n"

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
