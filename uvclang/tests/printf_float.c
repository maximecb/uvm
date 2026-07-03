// Differential test for uvclang's single-precision printf %f / %F support
// (against the native libc printf).
//
// UVM has no f64: a `float` vararg (promoted to `double` by the ABI) is
// de-promoted by uvclang and formatted by <stdio.h> in f32 arithmetic. So this
// only matches native libc byte-for-byte for values whose f32 %f output equals
// the double %f output — i.e. exactly representable (dyadic) values with short
// decimal expansions, and precisions that don't land on an exact rounding tie
// (native rounds half-to-even; our peel-and-round is half-away-from-zero, so
// e.g. "%.0f" of 2.5 would disagree — deliberately avoided here).
//
// All values flow through opaque() (a volatile round-trip) so that even at
// -O1/-O2 clang keeps the `float`->`double` fpext at the call site rather than
// const-folding it into a double literal (which uvclang does not yet de-promote).

#include <stdio.h>

// Defeat constant folding: the returned float is not a compile-time constant,
// so `printf("%f", opaque(x))` always emits a real fpext of a float.
static float opaque(float x) { volatile float v = x; return v; }

int main()
{
    // Basic values, default precision (6). All dyadic and exactly representable.
    printf("basic: %f %f %f %f\n", opaque(3.5f), opaque(0.5f), opaque(0.0f), opaque(100.0f));
    printf("neg:   %f %f\n", opaque(-2.25f), opaque(-1.75f));
    printf("frac:  %f %f\n", opaque(12.625f), opaque(255.5f));

    // Explicit precision (non-tie cases).
    printf("prec:  [%.2f][%.0f][%.4f][%.1f]\n",
           opaque(3.5f), opaque(2.0f), opaque(0.5f), opaque(12.625f));
    printf("round: [%.0f][%.1f]\n", opaque(2.75f), opaque(0.125f));  // 3, 0.1

    // Field width, left-justify, zero-pad.
    printf("width: [%8.2f][%-8.2f][%08.2f][%8.2f]\n",
           opaque(3.5f), opaque(3.5f), opaque(3.5f), opaque(-3.5f));

    // Sign flags and alternate form.
    printf("sign:  [%+.2f][%+.2f][% .2f][%#.0f]\n",
           opaque(3.5f), opaque(-3.5f), opaque(3.5f), opaque(4.0f));  // "4."

    // '*' width and precision from the arg list.
    printf("star:  [%*.*f]\n", 10, 3, opaque(2.5f));

    // Mixed conversions: proves the FP arg stays in step with the plain GP
    // va_list walk — the trailing %d must still read the right slot.
    printf("mixed: %d %f %d %s %f\n", 1, opaque(2.5f), 3, "ok", opaque(0.125f));

    // inf / nan (sign preserved for inf; nan is unsigned per C).
    printf("special: %f %f %f %F\n",
           opaque(1.0f) / opaque(0.0f),    // +inf
           -opaque(1.0f) / opaque(0.0f),   // -inf
           opaque(0.0f) / opaque(0.0f),    // nan
           opaque(1.0f) / opaque(0.0f));   // INF

    // Return value (characters written) must match native too.
    int n = printf("count: %.2f\n", opaque(3.5f));
    printf("wrote %d\n", n);

    return 0;
}
