INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ld a, $ff
    add 1
    ccf 
    ccf 
    ld a, $0f
    inc a
    scf
    inc a
    daa 
    dec a 
    daa 
    cpl

    ld hl, $56
    inc hl
    ld bc, $5089
    dec bc

    add hl, bc
    
    ld sp, $60
    add sp, -6

Done:
    jp Done
    


