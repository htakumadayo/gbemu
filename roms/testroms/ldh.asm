SECTION "Header", ROM0[$100]
    jp EntryPoint
    ds $150 - @, 0

EntryPoint:
    ld hl, $ff00  ; 65280
    ld c, 0

Test1: ; Test ldh (c), a　　write 0, 1, 2, 3, 4, 5
    inc c
    ld a, c
    ld [$ff00+c], a
    
    cp 5
    jp nz, Test1

Test2: ;　Remaining LDHs
    ld a, 5
    ld c, 1
    ld [$ff00+c], a
    ld a, 6
    ld [$ff00+$5], a
    ld a, [$ff00+$1]


Done:
    ld hl, $ffff
    jp Done