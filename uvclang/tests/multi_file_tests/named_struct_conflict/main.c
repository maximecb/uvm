// This whole build is expected to FAIL: point_a.c and point_b.c each define
// their own, incompatible `struct Point` under the same name, so
// Layout::new must reject the merge ("struct %struct.Point has conflicting
// definitions: module ... disagrees with module ..."), the same way a real
// ODR violation should be caught rather than silently letting one module's
// field layout win.
//
// Note: run_tests.sh reports any uvclang compile failure as SKIP regardless
// of cause (see TODO_multi_tu.md), so this fixture will show as SKIP, not a
// distinguishing FAIL/PASS — that's expected, not a bug in the harness.
#include <stdio.h>

int sum_a(void);
double sum_b(void);

int main(void)
{
    printf("%d %f\n", sum_a(), sum_b());
    return 0;
}
