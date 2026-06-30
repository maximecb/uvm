define i32 @main() {
entry:
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.n, %loop ]
  %a = phi i32 [ 0, %entry ], [ %b, %loop ]
  %b = phi i32 [ 1, %entry ], [ %ab, %loop ]
  %ab = add i32 %a, %b
  %i.n = add i32 %i, 1
  %c = icmp ult i32 %i.n, 10
  br i1 %c, label %loop, label %done
done:
  ret i32 %a
}
