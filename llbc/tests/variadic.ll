; ModuleID = 'tests/variadic.c'
source_filename = "tests/variadic.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag = type { i32, i32, ptr, ptr }

; Function Attrs: nofree norecurse nosync nounwind uwtable
define dso_local i32 @sum_args(i32 noundef %count, ...) local_unnamed_addr #0 {
entry:
  %ap = alloca [1 x %struct.__va_list_tag], align 16
  call void @llvm.lifetime.start.p0(i64 24, ptr nonnull %ap) #3
  call void @llvm.va_start.p0(ptr nonnull %ap)
  %cmp7 = icmp sgt i32 %count, 0
  br i1 %cmp7, label %for.body.lr.ph, label %for.cond.cleanup

for.body.lr.ph:                                   ; preds = %entry
  %ap.promoted = load i32, ptr %ap, align 16
  %overflow_arg_area_p = getelementptr inbounds nuw i8, ptr %ap, i64 8
  %0 = getelementptr inbounds nuw i8, ptr %ap, i64 16
  %reg_save_area = load ptr, ptr %0, align 16
  %overflow_arg_area_p.promoted = load ptr, ptr %overflow_arg_area_p, align 8
  %xtraiter = and i32 %count, 1
  %1 = icmp eq i32 %count, 1
  br i1 %1, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body.lr.ph.new

for.body.lr.ph.new:                               ; preds = %for.body.lr.ph
  %unroll_iter = and i32 %count, 2147483646
  br label %for.body

for.cond.cleanup.loopexit.unr-lcssa:              ; preds = %vaarg.end.1, %for.body.lr.ph
  %add.lcssa.ph = phi i32 [ poison, %for.body.lr.ph ], [ %add.1, %vaarg.end.1 ]
  %overflow_arg_area12.unr = phi ptr [ %overflow_arg_area_p.promoted, %for.body.lr.ph ], [ %overflow_arg_area11.1, %vaarg.end.1 ]
  %total.09.unr = phi i32 [ 0, %for.body.lr.ph ], [ %add.1, %vaarg.end.1 ]
  %gp_offset68.unr = phi i32 [ %ap.promoted, %for.body.lr.ph ], [ %gp_offset5.1, %vaarg.end.1 ]
  %lcmp.mod.not = icmp eq i32 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.cond.cleanup, label %for.body.epil

for.body.epil:                                    ; preds = %for.cond.cleanup.loopexit.unr-lcssa
  %fits_in_gp.epil = icmp ult i32 %gp_offset68.unr, 41
  br i1 %fits_in_gp.epil, label %vaarg.in_reg.epil, label %vaarg.in_mem.epil

vaarg.in_mem.epil:                                ; preds = %for.body.epil
  %overflow_arg_area.next.epil = getelementptr i8, ptr %overflow_arg_area12.unr, i64 8
  store ptr %overflow_arg_area.next.epil, ptr %overflow_arg_area_p, align 8
  br label %vaarg.end.epil

vaarg.in_reg.epil:                                ; preds = %for.body.epil
  %2 = zext nneg i32 %gp_offset68.unr to i64
  %3 = getelementptr i8, ptr %reg_save_area, i64 %2
  %4 = add nuw nsw i32 %gp_offset68.unr, 8
  store i32 %4, ptr %ap, align 16
  br label %vaarg.end.epil

vaarg.end.epil:                                   ; preds = %vaarg.in_reg.epil, %vaarg.in_mem.epil
  %vaarg.addr.epil = phi ptr [ %3, %vaarg.in_reg.epil ], [ %overflow_arg_area12.unr, %vaarg.in_mem.epil ]
  %5 = load i32, ptr %vaarg.addr.epil, align 4
  %add.epil = add nsw i32 %5, %total.09.unr
  br label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %vaarg.end.epil, %for.cond.cleanup.loopexit.unr-lcssa, %entry
  %total.0.lcssa = phi i32 [ 0, %entry ], [ %add.lcssa.ph, %for.cond.cleanup.loopexit.unr-lcssa ], [ %add.epil, %vaarg.end.epil ]
  call void @llvm.va_end.p0(ptr nonnull %ap)
  call void @llvm.lifetime.end.p0(i64 24, ptr nonnull %ap) #3
  ret i32 %total.0.lcssa

for.body:                                         ; preds = %vaarg.end.1, %for.body.lr.ph.new
  %overflow_arg_area12 = phi ptr [ %overflow_arg_area_p.promoted, %for.body.lr.ph.new ], [ %overflow_arg_area11.1, %vaarg.end.1 ]
  %total.09 = phi i32 [ 0, %for.body.lr.ph.new ], [ %add.1, %vaarg.end.1 ]
  %gp_offset68 = phi i32 [ %ap.promoted, %for.body.lr.ph.new ], [ %gp_offset5.1, %vaarg.end.1 ]
  %niter = phi i32 [ 0, %for.body.lr.ph.new ], [ %niter.next.1, %vaarg.end.1 ]
  %fits_in_gp = icmp ult i32 %gp_offset68, 41
  br i1 %fits_in_gp, label %vaarg.in_reg, label %vaarg.in_mem

vaarg.in_reg:                                     ; preds = %for.body
  %6 = zext nneg i32 %gp_offset68 to i64
  %7 = getelementptr i8, ptr %reg_save_area, i64 %6
  %8 = add nuw nsw i32 %gp_offset68, 8
  store i32 %8, ptr %ap, align 16
  br label %vaarg.end

vaarg.in_mem:                                     ; preds = %for.body
  %overflow_arg_area.next = getelementptr i8, ptr %overflow_arg_area12, i64 8
  store ptr %overflow_arg_area.next, ptr %overflow_arg_area_p, align 8
  br label %vaarg.end

vaarg.end:                                        ; preds = %vaarg.in_mem, %vaarg.in_reg
  %overflow_arg_area11 = phi ptr [ %overflow_arg_area12, %vaarg.in_reg ], [ %overflow_arg_area.next, %vaarg.in_mem ]
  %gp_offset5 = phi i32 [ %8, %vaarg.in_reg ], [ %gp_offset68, %vaarg.in_mem ]
  %vaarg.addr = phi ptr [ %7, %vaarg.in_reg ], [ %overflow_arg_area12, %vaarg.in_mem ]
  %9 = load i32, ptr %vaarg.addr, align 4
  %add = add nsw i32 %9, %total.09
  %fits_in_gp.1 = icmp ult i32 %gp_offset5, 41
  br i1 %fits_in_gp.1, label %vaarg.in_reg.1, label %vaarg.in_mem.1

vaarg.in_mem.1:                                   ; preds = %vaarg.end
  %overflow_arg_area.next.1 = getelementptr i8, ptr %overflow_arg_area11, i64 8
  store ptr %overflow_arg_area.next.1, ptr %overflow_arg_area_p, align 8
  br label %vaarg.end.1

vaarg.in_reg.1:                                   ; preds = %vaarg.end
  %10 = zext nneg i32 %gp_offset5 to i64
  %11 = getelementptr i8, ptr %reg_save_area, i64 %10
  %12 = add nuw nsw i32 %gp_offset5, 8
  store i32 %12, ptr %ap, align 16
  br label %vaarg.end.1

vaarg.end.1:                                      ; preds = %vaarg.in_reg.1, %vaarg.in_mem.1
  %overflow_arg_area11.1 = phi ptr [ %overflow_arg_area11, %vaarg.in_reg.1 ], [ %overflow_arg_area.next.1, %vaarg.in_mem.1 ]
  %gp_offset5.1 = phi i32 [ %12, %vaarg.in_reg.1 ], [ %gp_offset5, %vaarg.in_mem.1 ]
  %vaarg.addr.1 = phi ptr [ %11, %vaarg.in_reg.1 ], [ %overflow_arg_area11, %vaarg.in_mem.1 ]
  %13 = load i32, ptr %vaarg.addr.1, align 4
  %add.1 = add nsw i32 %13, %add
  %niter.next.1 = add i32 %niter, 2
  %niter.ncmp.1 = icmp eq i32 %niter.next.1, %unroll_iter
  br i1 %niter.ncmp.1, label %for.cond.cleanup.loopexit.unr-lcssa, label %for.body, !llvm.loop !5
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p0(ptr) #2

; Function Attrs: nofree norecurse nosync nounwind uwtable
define dso_local i32 @main() local_unnamed_addr #0 {
entry:
  %call = call i32 (i32, ...) @sum_args(i32 noundef 4, i32 noundef 10, i32 noundef 20, i32 noundef 30, i32 noundef 40)
  ret i32 %call
}

attributes #0 = { nofree norecurse nosync nounwind uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
