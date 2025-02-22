    and %r3 %r3 #0
    lea %r0 $str
    trap #x22
    trap #x25
$str: .stringz "hello"
      .fill #10
      .fill #0
    

