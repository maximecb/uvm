define i32 @main() {
entry:
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %loop ]
  %sum.next = add i32 %sum, %i
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, 10
  br i1 %cond, label %loop, label %after
after:
  switch i32 %sum, label %def [ i32 10, label %c10
                                i32 45, label %c45 ]
c10:
  br label %merge
c45:
  br label %merge
def:
  br label %merge
merge:
  %r = phi i32 [ 100, %c10 ], [ 200, %c45 ], [ 300, %def ]
  %lt = icmp slt i32 %sum, 50
  %sel = select i1 %lt, i32 %r, i32 999
  ret i32 %sel
}
