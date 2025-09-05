INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ld hl, BB
    ld a, 0
    jp hl
AA:
    ld a, $FF
BB: 
    inc a
    ld a, $ff
    add a, 1
    jp c, CC
    jp DD
CC:
    ld c, $0F
DD:
    ld c, $F0

    jr EE
EE:
    ld a, 1
    ld a, 2
    ld a, 3
    ld a, 4
FF:
    ld a, 5
    ld a, 6
    ld a, 7
    jr nz, FF

    jr nz, EE
    jr z, FF 

Done:
    inc a
    jp hl
    


