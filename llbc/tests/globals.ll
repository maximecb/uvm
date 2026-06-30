; ModuleID = 'tests/globals.c'
source_filename = "tests/globals.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.Point = type { i32, i32 }

@global_int = dso_local local_unnamed_addr global i32 42, align 4
@global_arr = dso_local global [5 x i32] [i32 1, i32 2, i32 3, i32 4, i32 5], align 16
@small = dso_local local_unnamed_addr global [4 x i8] zeroinitializer, align 1
@str = dso_local local_unnamed_addr constant [6 x i8] c"world\00", align 1
@.str = private unnamed_addr constant [6 x i8] c"hello\00", align 1
@msg = dso_local local_unnamed_addr global ptr @.str, align 8
@origin = dso_local local_unnamed_addr global %struct.Point zeroinitializer, align 4
@line = dso_local local_unnamed_addr global [3 x %struct.Point] [%struct.Point { i32 -1, i32 0 }, %struct.Point { i32 0, i32 1 }, %struct.Point { i32 2, i32 3 }], align 16
@matrix = dso_local local_unnamed_addr global [2 x [3 x i32]] [[3 x i32] [i32 1, i32 2, i32 3], [3 x i32] [i32 4, i32 5, i32 6]], align 16
@tail = dso_local local_unnamed_addr global ptr getelementptr inbounds nuw (i8, ptr @global_arr, i64 8), align 8

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
entry:
  ret i32 0
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
