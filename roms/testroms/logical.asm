INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
TestAnd:
    ld a, $F0
    ld b, $0F
    and b

    ld hl, _RAM  ; 49152
    ld a, $86
    ld [hl], $00
    and [hl]

    ld a, $A9
    and $00

TestOr:
    ld a, $00
    ld b, $00
    or b

    ld a, $00
    or [hl]

    ld a, $00
    or $00

TestXor:
    ld a, $34
    ld b, a
    xor b

    ld a, $00
    xor [hl]

    ld a, $A9
    xor $A9


Done:
    jp Done