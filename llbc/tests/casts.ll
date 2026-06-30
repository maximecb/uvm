; ModuleID = 'tests/casts.c'
source_filename = "tests/casts.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i32 -32896, 32895) i32 @sext_trunc(i32 noundef %x) local_unnamed_addr #0 {
entry:
  %conv = zext i32 %x to i64
  %sext = shl i64 %conv, 56
  %conv2 = ashr exact i64 %sext, 56
  %sext6 = shl i64 %conv, 48
  %conv3 = ashr exact i64 %sext6, 48
  %add = add nsw i64 %conv2, %conv3
  %conv4 = trunc nsw i64 %add to i32
  ret i32 %conv4
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i32 0, 65791) i32 @zext(i8 noundef zeroext %b, i16 noundef zeroext %h) local_unnamed_addr #0 {
entry:
  %conv = zext i8 %b to i32
  %conv1 = zext i16 %h to i32
  %add = add nuw nsw i32 %conv1, %conv
  ret i32 %add
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i64 -4611686016279904256, 4611686018427387905) i64 @mul64(i32 noundef %a, i32 noundef %b) local_unnamed_addr #0 {
entry:
  %conv = sext i32 %a to i64
  %conv1 = sext i32 %b to i64
  %mul = mul nsw i64 %conv1, %conv
  ret i64 %mul
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef range(i32 -96, 66142) i32 @main() local_unnamed_addr #0 {
entry:
  ret i32 781
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
