; ModuleID = 'tests/recursion.c'
source_filename = "tests/recursion.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nofree nosync nounwind memory(none) uwtable
define dso_local range(i32 1, -2147483648) i32 @fact(i32 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp = icmp slt i32 %n, 2
  br i1 %cmp, label %cond.end, label %cond.false

cond.false:                                       ; preds = %entry
  %sub = add nsw i32 %n, -1
  %call = call i32 @fact(i32 noundef %sub)
  %mul = mul nuw nsw i32 %call, %n
  br label %cond.end

cond.end:                                         ; preds = %entry, %cond.false
  %cond = phi i32 [ %mul, %cond.false ], [ 1, %entry ]
  ret i32 %cond
}

; Function Attrs: nofree nosync nounwind memory(none) uwtable
define dso_local range(i32 0, 2) i32 @is_odd(i32 noundef %n) local_unnamed_addr #0 {
entry:
  %switch = icmp ult i32 %n, 2
  br i1 %switch, label %cond.end, label %cond.false.i

cond.false.i:                                     ; preds = %entry
  %sub.i = add nsw i32 %n, -2
  %call.i = call i32 @is_odd(i32 noundef %sub.i)
  br label %cond.end

cond.end:                                         ; preds = %entry, %cond.false.i
  %cond = phi i32 [ %call.i, %cond.false.i ], [ %n, %entry ]
  ret i32 %cond
}

; Function Attrs: nofree nosync nounwind memory(none) uwtable
define dso_local range(i32 0, 2) i32 @is_even(i32 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp = icmp eq i32 %n, 0
  br i1 %cmp, label %cond.end, label %cond.false

cond.false:                                       ; preds = %entry
  %sub = add nsw i32 %n, -1
  %call = call i32 @is_odd(i32 noundef %sub)
  br label %cond.end

cond.end:                                         ; preds = %entry, %cond.false
  %cond = phi i32 [ %call, %cond.false ], [ 1, %entry ]
  ret i32 %cond
}

; Function Attrs: nofree nosync nounwind memory(none) uwtable
define dso_local range(i32 -2147483647, -2147483648) i32 @ack(i32 noundef %m, i32 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp = icmp eq i32 %m, 0
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %add = add nsw i32 %n, 1
  br label %return

if.end:                                           ; preds = %entry
  %cmp1 = icmp eq i32 %n, 0
  %sub = add nsw i32 %m, -1
  br i1 %cmp1, label %if.then2, label %if.end3

if.then2:                                         ; preds = %if.end
  %call = call i32 @ack(i32 noundef %sub, i32 noundef 1)
  br label %return

if.end3:                                          ; preds = %if.end
  %sub5 = add nsw i32 %n, -1
  %call6 = call i32 @ack(i32 noundef %m, i32 noundef %sub5)
  %call7 = call i32 @ack(i32 noundef %sub, i32 noundef %call6)
  br label %return

return:                                           ; preds = %if.end3, %if.then2, %if.then
  %retval.0 = phi i32 [ %add, %if.then ], [ %call, %if.then2 ], [ %call7, %if.end3 ]
  ret i32 %retval.0
}

; Function Attrs: nofree nosync nounwind memory(none) uwtable
define dso_local range(i32 -2147483647, -2147483648) i32 @main() local_unnamed_addr #0 {
entry:
  %call = call i32 @fact(i32 noundef 5)
  %rem = urem i32 %call, 100
  %call.i = call i32 @is_odd(i32 noundef 9)
  %add = add nuw nsw i32 %rem, %call.i
  %call2 = call i32 @is_odd(i32 noundef 7)
  %add3 = add nuw nsw i32 %add, %call2
  %call4 = call i32 @ack(i32 noundef 2, i32 noundef 3)
  %add5 = add nsw i32 %add3, %call4
  ret i32 %add5
}

attributes #0 = { nofree nosync nounwind memory(none) uwtable "disable-tail-calls"="true" "min-legal-vector-width"="0" "no-jump-tables"="true" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Homebrew clang version 20.1.7"}
