.org #x20
    .fill $getc
    .fill $out
    .fill $puts
    .fill $in
    .fill $putsp
    .fill $halt

.org #x200
    ld %r3 $usrspc
    jmp %r3
    trap #x25

$usrspc: .fill #x3000

$getc:
    ldi %r0 $kbr_stat
    br z $getc
    ldi %r0 $kbr_data
    rti

$out:
    st %r0 $out_sv_r0
$out_loop:
    ldi %r0 $disp_stat
    br z $out_loop
    ldi %r0 $disp_data
    ld %r0 $out_sv_r0
    rti
    $out_sv_r0: .fill #0

$putsp:
$puts:
    st %r0 $puts_sv_r0
    st %r1 $puts_sv_r1
    ld %r1 $puts_sv_r0
    and %r0 %r0 #0
$puts_loop:
    ldr %r0 %r1 #0
    br z $puts_end
    trap #x21
    add %r1 %r1 #1
    br nzp $puts_loop
$puts_end:
    ld %r0 $puts_sv_r0
    ld %r1 $puts_sv_r1
    rti
    $puts_sv_r0: .fill #0
    $puts_sv_r1: .fill #0

$in:
    st %r0 $puts_sv_r0
    trap #x20
    trap #x21
    rti
    $in_sv_r0: .fill #0

$halt:
    br nzp #-1

$kbr_stat:  .fill #xfe00
$kbr_data:  .fill #xfe02

$disp_stat: .fill #xfe04
$disp_data: .fill #xfe06

