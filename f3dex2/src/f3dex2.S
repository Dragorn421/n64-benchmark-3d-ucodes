.macro glabel label
    .global \label
    \label:
.endm

.balign 16

glabel gspF3DZEX2_NoN_PosLight_fifoTextStart
    .incbin "f3dex2_ucode/f3dex2/build/F3DZEX_NoN_2.08J/F3DZEX_NoN_2.08J.code"
glabel gspF3DZEX2_NoN_PosLight_fifoTextEnd

.balign 16

glabel gspF3DZEX2_NoN_PosLight_fifoDataStart
    .incbin "f3dex2_ucode/f3dex2/build/F3DZEX_NoN_2.08J/F3DZEX_NoN_2.08J.data"
glabel gspF3DZEX2_NoN_PosLight_fifoDataEnd
