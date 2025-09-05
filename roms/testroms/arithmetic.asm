INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ; LD HL, SP+e8
    ld sp, 5
    ld hl, sp+1
    ld a, $1

    ld sp, 5
    ld hl, sp-10
    ld a, $2

    ld sp, $fffe
    ld hl, sp+$ff
    ld a, $3

    ld sp, $0007
    ld hl, sp+1
    ld a, $4

TestAdd:
    ld a, 25
    add a, 1
    ld b, 2
    add a, b
    ld a, 1
    ld hl, 257
    add a, [hl]

TestAdc:
    xor a
    adc $ff
    
    ld hl, 257
    xor a
    adc [hl]

    ld b, 5
    xor a
    adc b

TestSub:
    ld a, 25
    sub a, 1
    ld a, 1
    ld b, 2
    sub a, b
    ld a, 1
    ld hl, 257
    sub a, [hl]

TestSbc:
    xor a
    sbc $ff
    
    ld hl, 257
    xor a
    sbc [hl]

    ld b, 5
    xor a
    sbc b

Done:
    jp Done
    


