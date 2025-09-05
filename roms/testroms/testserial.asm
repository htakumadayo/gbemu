INCLUDE "hardware.inc"

SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0


EntryPoint:
    ld a, $41
    call out
    ld a, $42
    call out
    ld a, $43
    call out

Done:
    jp Done

; Send a byte to serial output
; @param a: Data
out:
    ld c, 1
    ld [$ff00+c], a
    ld c, 2
    ld a, $81
    ld [$ff00+c], a
    ret
