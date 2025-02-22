    and %r3 %r3 #0
    jsr $dec_r0
    br n $skip
    add %r1 %r1 #5
    br nzp $end
$skip: 
    add %r1 %r1 #3
$end: 
    trap #x25

$inc_r0:
    add %r0 %r0 #1
    ret

$dec_r0:
    add %r0 %r0 #-1
    ret
