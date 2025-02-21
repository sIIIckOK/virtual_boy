    and %r3 %r3 #0
    add %r0 %r0 #-1
    br n $skip
    add %r1 %r1 #-3
    br nzp $end
$skip: 
    add %r1 %r1 #3
$end: 
    jsr $inc_r0
    trap #x25

$inc_r0:
    add %r0 %r0 #1
    ret
