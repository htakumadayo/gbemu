INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ld b, $5e
    sla b
    sla b
    sra b
    sla b
    sra b
    swap b
    srl b
    bit 0, b
    bit 5, b
    bit 7, b
    res 1, b
    set 2, b
    res 3, b

    ld hl, $C000
    ld [hl], $2a
    sla [hl]
    sla [hl]
    sra [hl]
    sla [hl]
    sra [hl]
    swap [hl]
    srl [hl]
    bit 0, [hl]
    bit 5, [hl]
    bit 7, [hl]
    res 1, [hl]
    set 2, [hl]
    res 3, [hl]


Done:
    jp Done
    


