; ModuleID = 'tests/pointers.c'
source_filename = "tests/pointers.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) uwtable
define dso_local i32 @dot(ptr nocapture noundef readonly %a, ptr nocapture noundef readonly %b) local_unnamed_addr #0 {
entry:
  %0 = load i32, ptr %a, align 4
  %1 = load i32, ptr %b, align 4
  %mul = mul nsw i32 %1, %0
  %y = getelementptr inbounds nuw i8, ptr %a, i64 4
  %2 = load i32, ptr %y, align 4
  %y2 = getelementptr inbounds nuw i8, ptr %b, i64 4
  %3 = load i32, ptr %y2, align 4
  %mul3 = mul nsw i32 %3, %2
  %add = add nsw i32 %mul3, %mul
  %z = getelementptr inbounds nuw i8, ptr %a, i64 8
  %4 = load i32, ptr %z, align 4
  %z4 = getelementptr inbounds nuw i8, ptr %b, i64 8
  %5 = load i32, ptr %z4, align 4
  %mul5 = mul nsw i32 %5, %4
  %add6 = add nsw i32 %add, %mul5
  ret i32 %add6
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) uwtable
define dso_local void @scale(ptr nocapture noundef %v, i32 noundef %s) local_unnamed_addr #1 {
entry:
  %0 = load i32, ptr %v, align 4
  %mul = mul nsw i32 %0, %s
  store i32 %mul, ptr %v, align 4
  %y = getelementptr inbounds nuw i8, ptr %v, i64 4
  %1 = load i32, ptr %y, align 4
  %mul1 = mul nsw i32 %1, %s
  store i32 %mul1, ptr %y, align 4
  %z = getelementptr inbounds nuw i8, ptr %v, i64 8
  %2 = load i32, ptr %z, align 4
  %mul2 = mul nsw i32 %2, %s
  store i32 %mul2, ptr %z, align 4
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read) uwtable
define dso_local i32 @sum_array(ptr nocapture noundef readonly %arr, i32 noundef %n) local_unnamed_addr #2 {
entry:
  %cmp4 = icmp sgt i32 %n, 0
  br i1 %cmp4, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext nneg i32 %n to i64
  %xtraiter = and i64 %wide.trip.count, 7
  %0 = icmp ult i32 %n, 8
  br i1 %0, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i64 %wide.trip.count, 2147483640
  br label %for.body

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %for.body, %for.body.preheader
  %add.lcssa.ph = phi i32 [ poison, %for.body.preheader ], [ %add.7, %for.body ]
  %indvars.iv.unr = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next.7, %for.body ]
  %s.05.unr = phi i32 [ 0, %for.body.preheader ], [ %add.7, %for.body ]
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body.epil

for.body.epil:                                    ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil
  %indvars.iv.epil = phi i64 [ %indvars.iv.next.epil, %for.body.epil ], [ %indvars.iv.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %s.05.epil = phi i32 [ %add.epil, %for.body.epil ], [ %s.05.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %epil.iter = phi i64 [ %epil.iter.next, %for.body.epil ], [ 0, %for.cond.cleanup.loopexit.unr-lcssa ]
  %arrayidx.epil = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.epil
  %1 = load i32, ptr %arrayidx.epil, align 4
  %add.epil = add nsw i32 %1, %s.05.epil
  %indvars.iv.next.epil = add nuw nsw i64 %indvars.iv.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.cond.cleanup, label %for.body.epil, !llvm.loop !5

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil, %entry
  %s.0.lcssa = phi i32 [ 0, %entry ], [ %add.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %add.epil, %for.body.epil ]
  ret i32 %s.0.lcssa

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %indvars.iv = phi i64 [ 0, %for.body.preheader.new ], [ %indvars.iv.next.7, %for.body ]
  %s.05 = phi i32 [ 0, %for.body.preheader.new ], [ %add.7, %for.body ]
  %niter = phi i64 [ 0, %for.body.preheader.new ], [ %niter.next.7, %for.body ]
  %arrayidx = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv
  %2 = load i32, ptr %arrayidx, align 4
  %add = add nsw i32 %2, %s.05
  %indvars.iv.next = or disjoint i64 %indvars.iv, 1
  %arrayidx.1 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next
  %3 = load i32, ptr %arrayidx.1, align 4
  %add.1 = add nsw i32 %3, %add
  %indvars.iv.next.1 = or disjoint i64 %indvars.iv, 2
  %arrayidx.2 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next.1
  %4 = load i32, ptr %arrayidx.2, align 4
  %add.2 = add nsw i32 %4, %add.1
  %indvars.iv.next.2 = or disjoint i64 %indvars.iv, 3
  %arrayidx.3 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next.2
  %5 = load i32, ptr %arrayidx.3, align 4
  %add.3 = add nsw i32 %5, %add.2
  %indvars.iv.next.3 = or disjoint i64 %indvars.iv, 4
  %arrayidx.4 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next.3
  %6 = load i32, ptr %arrayidx.4, align 4
  %add.4 = add nsw i32 %6, %add.3
  %indvars.iv.next.4 = or disjoint i64 %indvars.iv, 5
  %arrayidx.5 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next.4
  %7 = load i32, ptr %arrayidx.5, align 4
  %add.5 = add nsw i32 %7, %add.4
  %indvars.iv.next.5 = or disjoint i64 %indvars.iv, 6
  %arrayidx.6 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next.5
  %8 = load i32, ptr %arrayidx.6, align 4
  %add.6 = add nsw i32 %8, %add.5
  %indvars.iv.next.6 = or disjoint i64 %indvars.iv, 7
  %arrayidx.7 = getelementptr inbounds nuw i32, ptr %arr, i64 %indvars.iv.next.6
  %9 = load i32, ptr %arrayidx.7, align 4
  %add.7 = add nsw i32 %9, %add.6
  %indvars.iv.next.7 = add nuw nsw i64 %indvars.iv, 8
  %niter.next.7 = add i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body, !llvm.loop !7
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) uwtable
define dso_local i32 @nested(ptr nocapture noundef readonly %m) local_unnamed_addr #0 {
entry:
  %arrayidx1 = getelementptr inbounds nuw i8, ptr %m, i64 28
  %0 = load i32, ptr %arrayidx1, align 4
  %1 = load i32, ptr %m, align 4
  %add = add nsw i32 %1, %0
  ret i32 %add
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef range(i32 -2147483575, -2147483648) i32 @main() local_unnamed_addr #3 {
entry:
  ret i32 223
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nofree norecurse nosync nounwind memory(argmem: read) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

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
