add %r5 %r5 #2
add %r5 %r5 #-1
br z $skip
add %r1 %r1 #3
br nzp $end
$skip:
add %r1 %r1 #4
$end:
trap #1
