; ModuleID = 'tests/loops.c'
source_filename = "tests/loops.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i32 @sum_while(i32 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp.not4 = icmp slt i32 %n, 1
  br i1 %cmp.not4, label %while.end, label %while.body.preheader

while.body.preheader:                             ; preds = %entry
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
  br label %while.end

while.end:                                        ; preds = %while.body.preheader, %entry
  %s.0.lcssa = phi i32 [ 0, %entry ], [ %9, %while.body.preheader ]
  ret i32 %s.0.lcssa
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local range(i32 1, 0) i32 @sum_dowhile(i32 noundef %n) local_unnamed_addr #0 {
entry:
  %smax = call i32 @llvm.smax.i32(i32 %n, i32 1)
  %0 = shl nuw i32 %smax, 1
  %1 = add nsw i32 %smax, -1
  %2 = zext nneg i32 %1 to i33
  %3 = add nsw i32 %smax, -2
  %4 = zext i32 %3 to i33
  %5 = mul i33 %2, %4
  %6 = lshr i33 %5, 1
  %7 = trunc nuw i33 %6 to i32
  %8 = add i32 %0, %7
  %9 = add i32 %8, -1
  ret i32 %9
}

; Function Attrs: nofree norecurse nosync nounwind memory(none) uwtable
define dso_local i32 @count_pairs(i32 noundef %n) local_unnamed_addr #1 {
entry:
  %cmp28 = icmp sgt i32 %n, 0
  br i1 %cmp28, label %for.body4.preheader.preheader, label %for.cond.cleanup

for.body4.preheader.preheader:                    ; preds = %entry
  %xtraiter = and i32 %n, 1
  %0 = icmp eq i32 %n, 1
  br i1 %0, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body4.preheader.preheader.new

for.body4.preheader.preheader.new:                ; preds = %for.body4.preheader.preheader
  %unroll_iter = and i32 %n, 2147483646
  br label %for.body4.preheader

for.body4.preheader:                              ; preds = %cleanup.1, %for.body4.preheader.preheader.new
  %i.030 = phi i32 [ 0, %for.body4.preheader.preheader.new ], [ %inc11.1, %cleanup.1 ]
  %c.029 = phi i32 [ 0, %for.body4.preheader.preheader.new ], [ %c.1.lcssa.ph.1, %cleanup.1 ]
  %niter = phi i32 [ 0, %for.body4.preheader.preheader.new ], [ %niter.next.1, %cleanup.1 ]
  br label %for.body4

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %cleanup.1, %for.body4.preheader.preheader
  %c.1.lcssa.ph.lcssa.ph = phi i32 [ poison, %for.body4.preheader.preheader ], [ %c.1.lcssa.ph.1, %cleanup.1 ]
  %i.030.unr = phi i32 [ 0, %for.body4.preheader.preheader ], [ %inc11.1, %cleanup.1 ]
  %c.029.unr = phi i32 [ 0, %for.body4.preheader.preheader ], [ %c.1.lcssa.ph.1, %cleanup.1 ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body4.epil

for.body4.epil:                                   ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.inc.epil
  %j.026.epil = phi i32 [ %inc9.epil, %for.inc.epil ], [ 0, %for.cond.cleanup.loopexit.unr-lcssa ]
  %c.125.epil = phi i32 [ %c.2.epil, %for.inc.epil ], [ %c.029.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %cmp5.epil = icmp eq i32 %i.030.unr, %j.026.epil
  br i1 %cmp5.epil, label %for.inc.epil, label %if.end.epil

if.end.epil:                                      ; preds = %for.body4.epil
  %add.epil = add nuw nsw i32 %j.026.epil, %i.030.unr
  %cmp6.epil = icmp sgt i32 %add.epil, %n
  br i1 %cmp6.epil, label %for.cond.cleanup, label %if.end8.epil

if.end8.epil:                                     ; preds = %if.end.epil
  %inc.epil = add nsw i32 %c.125.epil, 1
  br label %for.inc.epil

for.inc.epil:                                     ; preds = %if.end8.epil, %for.body4.epil
  %c.2.epil = phi i32 [ %c.125.epil, %for.body4.epil ], [ %inc.epil, %if.end8.epil ]
  %inc9.epil = add nuw nsw i32 %j.026.epil, 1
  %exitcond.not.epil = icmp eq i32 %inc9.epil, %n
  br i1 %exitcond.not.epil, label %for.cond.cleanup, label %for.body4.epil, !llvm.loop !5

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.inc.epil, %if.end.epil, %entry
  %c.0.lcssa = phi i32 [ 0, %entry ], [ %c.1.lcssa.ph.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %c.2.epil, %for.inc.epil ], [ %c.125.epil, %if.end.epil ]
  ret i32 %c.0.lcssa

for.body4:                                        ; preds = %for.body4.preheader, %for.inc
  %j.026 = phi i32 [ %inc9, %for.inc ], [ 0, %for.body4.preheader ]
  %c.125 = phi i32 [ %c.2, %for.inc ], [ %c.029, %for.body4.preheader ]
  %cmp5 = icmp eq i32 %i.030, %j.026
  br i1 %cmp5, label %for.inc, label %if.end

if.end:                                           ; preds = %for.body4
  %add = add nuw nsw i32 %j.026, %i.030
  %cmp6 = icmp sgt i32 %add, %n
  br i1 %cmp6, label %cleanup, label %if.end8

if.end8:                                          ; preds = %if.end
  %inc = add nsw i32 %c.125, 1
  br label %for.inc

for.inc:                                          ; preds = %for.body4, %if.end8
  %c.2 = phi i32 [ %c.125, %for.body4 ], [ %inc, %if.end8 ]
  %inc9 = add nuw nsw i32 %j.026, 1
  %exitcond.not = icmp eq i32 %inc9, %n
  br i1 %exitcond.not, label %cleanup, label %for.body4, !llvm.loop !5

cleanup:                                          ; preds = %for.inc, %if.end
  %c.1.lcssa.ph = phi i32 [ %c.2, %for.inc ], [ %c.125, %if.end ]
  %inc11 = or disjoint i32 %i.030, 1
  br label %for.body4.1

for.body4.1:                                      ; preds = %for.inc.1, %cleanup
  %j.026.1 = phi i32 [ %inc9.1, %for.inc.1 ], [ 0, %cleanup ]
  %c.125.1 = phi i32 [ %c.2.1, %for.inc.1 ], [ %c.1.lcssa.ph, %cleanup ]
  %cmp5.1 = icmp eq i32 %inc11, %j.026.1
  br i1 %cmp5.1, label %for.inc.1, label %if.end.1

if.end.1:                                         ; preds = %for.body4.1
  %add.1 = add nuw nsw i32 %j.026.1, %inc11
  %cmp6.1 = icmp sgt i32 %add.1, %n
  br i1 %cmp6.1, label %cleanup.1, label %if.end8.1

if.end8.1:                                        ; preds = %if.end.1
  %inc.1 = add nsw i32 %c.125.1, 1
  br label %for.inc.1

for.inc.1:                                        ; preds = %if.end8.1, %for.body4.1
  %c.2.1 = phi i32 [ %c.125.1, %for.body4.1 ], [ %inc.1, %if.end8.1 ]
  %inc9.1 = add nuw nsw i32 %j.026.1, 1
  %exitcond.not.1 = icmp eq i32 %inc9.1, %n
  br i1 %exitcond.not.1, label %cleanup.1, label %for.body4.1, !llvm.loop !5

cleanup.1:                                        ; preds = %for.inc.1, %if.end.1
  %c.1.lcssa.ph.1 = phi i32 [ %c.2.1, %for.inc.1 ], [ %c.125.1, %if.end.1 ]
  %inc11.1 = add nuw nsw i32 %i.030, 2
  %niter.next.1 = add i32 %niter, 2
  %niter.ncmp.1 = icmp eq i32 %niter.next.1, %unroll_iter
  br i1 %niter.ncmp.1, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body4.preheader, !llvm.loop !7
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef range(i32 -2147483578, -2147483648) i32 @main() local_unnamed_addr #0 {
cleanup.i.5:
  ret i32 92
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #2

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree norecurse nosync nounwind memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = distinct !{!7, !6}
