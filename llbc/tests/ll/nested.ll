; Nested loops with phis carrying an accumulator across the inner loop.
; total = sum over i in [0,5), j in [0,i) of (i*10 + j)
define i32 @main() {
entry:
  br label %outer.head
outer.head:
  %i = phi i32 [ 0, %entry ], [ %i.n, %outer.latch ]
  %acc = phi i32 [ 0, %entry ], [ %acc.out, %outer.latch ]
  %i.lt = icmp slt i32 %i, 5
  br i1 %i.lt, label %inner.head, label %done
inner.head:
  %j = phi i32 [ 0, %outer.head ], [ %j.n, %inner.body ]
  %acc.in = phi i32 [ %acc, %outer.head ], [ %acc.in.n, %inner.body ]
  %j.lt = icmp slt i32 %j, %i
  br i1 %j.lt, label %inner.body, label %outer.latch
inner.body:
  %i10 = mul i32 %i, 10
  %term = add i32 %i10, %j
  %acc.in.n = add i32 %acc.in, %term
  %j.n = add i32 %j, 1
  br label %inner.head
outer.latch:
  %acc.out = phi i32 [ %acc.in, %inner.head ]
  %i.n = add i32 %i, 1
  br label %outer.head
done:
  ret i32 %acc
}
