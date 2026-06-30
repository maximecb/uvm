; ModuleID = 'tests/arithmetic.c'
source_filename = "tests/arithmetic.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef i32 @arith(i32 noundef %a, i32 noundef %b) local_unnamed_addr #0 {
entry:
  %div = sdiv i32 %a, %b
  %rem1 = srem i32 %a, %b
  %add418 = add i32 %b, 1
  %add5 = mul i32 %add418, %a
  %add6 = add i32 %add5, %div
  %add7 = add i32 %add6, %rem1
  ret i32 %add7
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i32 0, 1073741824) i32 @bitops(i32 noundef %a, i32 noundef %b) local_unnamed_addr #0 {
entry:
  %or = or i32 %b, %a
  %0 = lshr i32 %or, 1
  %shr = and i32 %0, 1073741823
  %shr1 = lshr i32 %shr, 3
  %and2 = and i32 %a, 255
  %1 = or i32 %and2, %shr1
  %or4 = or i32 %1, %shr
  ret i32 %or4
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i64 -2147516416, 6442483710) i64 @widen(i32 noundef %x) local_unnamed_addr #0 {
entry:
  %conv = sext i32 %x to i64
  %conv1 = zext i32 %x to i64
  %sext = shl i64 %conv1, 48
  %conv3 = ashr exact i64 %sext, 48
  %add = add nsw i64 %conv1, %conv
  %add4 = add nsw i64 %add, %conv3
  ret i64 %add4
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
entry:
  ret i32 391
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
