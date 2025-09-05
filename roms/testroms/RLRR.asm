INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ld a, $5B
    rlca
    rlca
    rrca
    rla
    rra
    rrca
    rla
    rra

    ld b, $48
    rlc b
    rr b
    rr b
    rlc b
    rrc b

    ld hl, $C000
    ld [hl], a
    rlc [hl]
    rrc [hl]
    rlc [hl]
    rrc [hl]

    rl b
    rl [hl]
    rr b
    rr [hl]
    rr b
    rr [hl]
    rl b
    rl [hl]


Done:
    jp Done
    


