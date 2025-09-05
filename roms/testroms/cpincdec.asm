INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ld a, 5
    ld hl, $C000
    ld [hl], a

    cp [hl]
    cp 1
    ld b, 10
    cp b

    ld a, $ff
    inc a
    ld [hl], $0f
    inc [hl]

    ld a, 0
    dec a
    ld [hl], $10
    dec [hl]

    

Done:
    jp Done
    


