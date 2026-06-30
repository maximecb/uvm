; ModuleID = 'tests/strings.c'
source_filename = "tests/strings.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [13 x i8] c"hello, world\00", align 1

; Function Attrs: nofree nounwind memory(argmem: read) uwtable
define dso_local i32 @count_char(ptr nocapture noundef readonly %s, i8 noundef signext %c) local_unnamed_addr #0 {
entry:
  %call = call i64 @strlen(ptr noundef nonnull dereferenceable(1) %s) #5
  %cmp9.not = icmp eq i64 %call, 0
  br i1 %cmp9.not, label %for.cond.cleanup, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %xtraiter = and i64 %call, 3
  %0 = icmp ult i64 %call, 4
  br i1 %0, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i64 %call, -4
  br label %for.body

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %for.body, %for.body.preheader
  %spec.select.lcssa.ph = phi i32 [ poison, %for.body.preheader ], [ %spec.select.3, %for.body ]
  %i.011.unr = phi i64 [ 0, %for.body.preheader ], [ %inc4.3, %for.body ]
  %n.010.unr = phi i32 [ 0, %for.body.preheader ], [ %spec.select.3, %for.body ]
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body.epil

for.body.epil:                                    ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil
  %i.011.epil = phi i64 [ %inc4.epil, %for.body.epil ], [ %i.011.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %n.010.epil = phi i32 [ %spec.select.epil, %for.body.epil ], [ %n.010.unr, %for.cond.cleanup.loopexit.unr-lcssa ]
  %epil.iter = phi i64 [ %epil.iter.next, %for.body.epil ], [ 0, %for.cond.cleanup.loopexit.unr-lcssa ]
  %arrayidx.epil = getelementptr inbounds nuw i8, ptr %s, i64 %i.011.epil
  %1 = load i8, ptr %arrayidx.epil, align 1
  %cmp2.epil = icmp eq i8 %1, %c
  %inc.epil = zext i1 %cmp2.epil to i32
  %spec.select.epil = add nuw nsw i32 %n.010.epil, %inc.epil
  %inc4.epil = add nuw i64 %i.011.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.cond.cleanup, label %for.body.epil, !llvm.loop !5

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit.unr-lcssa, %for.body.epil, %entry
  %n.0.lcssa = phi i32 [ 0, %entry ], [ %spec.select.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %spec.select.epil, %for.body.epil ]
  ret i32 %n.0.lcssa

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %i.011 = phi i64 [ 0, %for.body.preheader.new ], [ %inc4.3, %for.body ]
  %n.010 = phi i32 [ 0, %for.body.preheader.new ], [ %spec.select.3, %for.body ]
  %niter = phi i64 [ 0, %for.body.preheader.new ], [ %niter.next.3, %for.body ]
  %arrayidx = getelementptr inbounds nuw i8, ptr %s, i64 %i.011
  %2 = load i8, ptr %arrayidx, align 1
  %cmp2 = icmp eq i8 %2, %c
  %inc = zext i1 %cmp2 to i32
  %spec.select = add nuw nsw i32 %n.010, %inc
  %inc4 = or disjoint i64 %i.011, 1
  %arrayidx.1 = getelementptr inbounds nuw i8, ptr %s, i64 %inc4
  %3 = load i8, ptr %arrayidx.1, align 1
  %cmp2.1 = icmp eq i8 %3, %c
  %inc.1 = zext i1 %cmp2.1 to i32
  %spec.select.1 = add nuw nsw i32 %spec.select, %inc.1
  %inc4.1 = or disjoint i64 %i.011, 2
  %arrayidx.2 = getelementptr inbounds nuw i8, ptr %s, i64 %inc4.1
  %4 = load i8, ptr %arrayidx.2, align 1
  %cmp2.2 = icmp eq i8 %4, %c
  %inc.2 = zext i1 %cmp2.2 to i32
  %spec.select.2 = add nuw nsw i32 %spec.select.1, %inc.2
  %inc4.2 = or disjoint i64 %i.011, 3
  %arrayidx.3 = getelementptr inbounds nuw i8, ptr %s, i64 %inc4.2
  %5 = load i8, ptr %arrayidx.3, align 1
  %cmp2.3 = icmp eq i8 %5, %c
  %inc.3 = zext i1 %cmp2.3 to i32
  %spec.select.3 = add nuw nsw i32 %spec.select.2, %inc.3
  %inc4.3 = add nuw i64 %i.011, 4
  %niter.next.3 = add i64 %niter, 4
  %niter.ncmp.3 = icmp eq i64 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body, !llvm.loop !7
}

; Function Attrs: mustprogress nofree nounwind willreturn memory(argmem: read)
declare i64 @strlen(ptr nocapture noundef) local_unnamed_addr #1

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: read) uwtable
define dso_local noundef i64 @my_strlen(ptr noundef %s) local_unnamed_addr #2 {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.cond, %entry
  %p.0 = phi ptr [ %s, %entry ], [ %incdec.ptr, %while.cond ]
  %0 = load i8, ptr %p.0, align 1
  %tobool.not = icmp eq i8 %0, 0
  %incdec.ptr = getelementptr inbounds nuw i8, ptr %p.0, i64 1
  br i1 %tobool.not, label %while.end, label %while.cond, !llvm.loop !9

while.end:                                        ; preds = %while.cond
  %sub.ptr.lhs.cast = ptrtoint ptr %p.0 to i64
  %sub.ptr.rhs.cast = ptrtoint ptr %s to i64
  %sub.ptr.sub = sub i64 %sub.ptr.lhs.cast, %sub.ptr.rhs.cast
  ret i64 %sub.ptr.sub
}

; Function Attrs: mustprogress nofree nounwind willreturn memory(argmem: read) uwtable
define dso_local range(i32 0, 2) i32 @longer(ptr nocapture noundef readonly %a, ptr nocapture noundef readonly %b) local_unnamed_addr #3 {
entry:
  %call = call i64 @strlen(ptr noundef nonnull dereferenceable(1) %a) #5
  %call1 = call i64 @strlen(ptr noundef nonnull dereferenceable(1) %b) #5
  %cmp = icmp ugt i64 %call, %call1
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i32 @main() local_unnamed_addr #4 {
entry:
  ret i32 add (i32 add (i32 ptrtoint (ptr getelementptr inbounds nuw (i8, ptr @.str, i64 12) to i32), i32 add (i32 sub (i32 0, i32 ptrtoint (ptr @.str to i32)), i32 1)), i32 3)
}

attributes #0 = { nofree nounwind memory(argmem: read) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nofree nounwind willreturn memory(argmem: read) "disable-tail-calls"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nofree norecurse nosync nounwind memory(argmem: read) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nofree nounwind willreturn memory(argmem: read) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nounwind }

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
!9 = distinct !{!9, !8}
