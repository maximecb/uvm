; ModuleID = 'tests/control_flow.c'
source_filename = "tests/control_flow.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(none) uwtable
define dso_local i32 @fib(i32 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp4 = icmp sgt i32 %n, 0
  br i1 %cmp4, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %xtraiter = and i32 %n, 7
  %0 = icmp ult i32 %n, 8
  br i1 %0, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i32 %n, 2147483640
  br label %for.body

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %for.body, %for.body.preheader
  %b.05.lcssa.ph = phi i32 [ poison, %for.body.preheader ], [ %add.6, %for.body ]
  %a.07.unr = phi i32 [ 0, %for.body.preheader ], [ %add.6, %for.body ]
  %b.05.unr = phi i32 [ 1, %for.body.preheader ], [ %add.7, %for.body ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body.epil

for.body.epil:                                    ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil
  %a.07.epil = phi i32 [ %b.05.epil, %for.body.epil ], [ %a.07.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %b.05.epil = phi i32 [ %add.epil, %for.body.epil ], [ %b.05.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %epil.iter = phi i32 [ %epil.iter.next, %for.body.epil ], [ 0, %for.cond.cleanup.loopexit.unr-lcssa ]
  %add.epil = add nsw i32 %a.07.epil, %b.05.epil
  %epil.iter.next = add i32 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i32 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.cond.cleanup, label %for.body.epil, !llvm.loop !5

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil, %entry
  %a.0.lcssa = phi i32 [ 0, %entry ], [ %b.05.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %b.05.epil, %for.body.epil ]
  ret i32 %a.0.lcssa

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %a.07 = phi i32 [ 0, %for.body.preheader.new ], [ %add.6, %for.body ]
  %b.05 = phi i32 [ 1, %for.body.preheader.new ], [ %add.7, %for.body ]
  %niter = phi i32 [ 0, %for.body.preheader.new ], [ %niter.next.7, %for.body ]
  %add = add nsw i32 %a.07, %b.05
  %add.1 = add nsw i32 %b.05, %add
  %add.2 = add nsw i32 %add, %add.1
  %add.3 = add nsw i32 %add.1, %add.2
  %add.4 = add nsw i32 %add.2, %add.3
  %add.5 = add nsw i32 %add.3, %add.4
  %add.6 = add nsw i32 %add.4, %add.5
  %add.7 = add nsw i32 %add.5, %add.6
  %niter.next.7 = add i32 %niter, 8
  %niter.ncmp.7 = icmp eq i32 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body, !llvm.loop !7
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i32 -1, 301) i32 @classify(i32 noundef %x) local_unnamed_addr #1 {
entry:
  switch i32 %x, label %sw.default [
    i32 0, label %return
    i32 1, label %sw.bb1
    i32 2, label %sw.bb2
  ]

sw.bb1:                                           ; preds = %entry
  br label %return

sw.bb2:                                           ; preds = %entry
  br label %return

sw.default:                                       ; preds = %entry
  br label %return

return:                                           ; preds = %entry, %sw.default, %sw.bb2, %sw.bb1
  %retval.0 = phi i32 [ -1, %sw.default ], [ 300, %sw.bb2 ], [ 200, %sw.bb1 ], [ 100, %entry ]
  ret i32 %retval.0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i32 @sum_to(i32 noundef %n) local_unnamed_addr #1 {
entry:
  %cmp.not4 = icmp slt i32 %n, 1
  br i1 %cmp.not4, label %for.cond.cleanup, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = shl nuw i32 %n, 1
  %1 = add nsw i32 %n, -1
  %2 = zext nneg i32 %1 to i33
  %3 = add nsw i32 %n, -2
  %4 = zext i32 %3 to i33
  %5 = mul i33 %2, %4
  %6 = lshr i33 %5, 1
  %7 = trunc nuw i33 %6 to i32
  %8 = add i32 %0, %7
  %9 = add i32 %8, -1
  br label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i32 [ 0, %entry ], [ %9, %for.body.preheader ]
  ret i32 %s.0.lcssa
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef range(i32 -2147483333, -2147483648) i32 @main() local_unnamed_addr #1 {
entry:
  ret i32 370
}

attributes #0 = { nofree norecurse nosync nounwind memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.unroll.disable"}
!7 = distinct !{!7, !8}
!8 = !{!"llvm.loop.mustprogress"}
