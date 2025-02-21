    add %r1 %r1 #-1
    lea %r0 $str
    trap #x22
    trap #x25
$str: .stringz "hello"
      .fill #10
      .fill #0
    

