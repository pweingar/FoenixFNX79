;;;
;;; Code to support text I/O
;;;

            .dpage DP_BASE
            .databank 0
            .as
            .xl

text        .namespace

            .virtual $d000
mstr_ctrl_0 .byte ?
mstr_ctrl_1 .byte ?

rsrvd1      .word ?

brdr_ctrl   .byte ?
brdr_blue   .byte ?
brdr_green  .byte ?
brdr_red    .byte ?
brdr_width  .byte ?
brdr_height .byte ?
            .endvirtual

            .virtual $d010
crsr_ctrl   .byte ?
crsr_char   .byte ?
crsr_x      .word ?
crsr_y      .word ?
            .endvirtual

            .virtual $de00
mulu_a      .word ?
mulu_b      .word ?
divu_den    .word ?
divu_num    .word ?
add_a       .dword ?
add_b       .dword ?
mulu_res    .dword ?
quou        .word ?
remu        .word ?
add_res     .dword ?
            .endvirtual

            .section variables
data_ptr    .word ?
line_ptr    .word ?
column      .word ?
row         .word ?
color       .byte ?
num_rows    .word ?
num_cols    .word ?
            .endsection

TXT_MATRIX = $c000
TXT_CLUT_FG = $d800
TXT_CLUT_BG = $d840

            .section code

;
; Get the position of the text cursor
;
; Result:
; X = column
; Y = row
;
get_xy      .proc
            ldx column
            ldy row
            rts
            .pend

;
; Set the position of the text cursor
;
; X = the target column
; Y = the target row
set_xy      .proc
            pha

            ; Save the cursor position
            sty row
            stx column

            lda #<TXT_MATRIX
            sta line_ptr
            lda #>TXT_MATRIX
            sta line_ptr+1

loop        cpy #0
            beq done

            clc
            lda line_ptr
            adc num_cols
            sta line_ptr
            lda line_ptr+1
            adc #0
            sta line_ptr+1

            dey
            bra loop

done        pla
            rts
            .pend

;
; Clear the screen and home the text cursor
;
clear       .proc
            pha
            phx
            phy

            ; Set the cursor to the home position
            ldx #0
            ldy #0
            jsr set_xy

            ; Clear the screen memory
            ldy #0
loop        jsr mmu.go_io_2
            lda #' '
            sta TXT_MATRIX,y

            jsr mmu.go_io_3
            lda color
            sta TXT_MATRIX,y

            iny
            cpy #$2000
            bne loop

            ply
            plx
            pla
            rts
            .pend

;
; Put the character in A to the screen at the cursor
;
; A = the character to print
;
put         .proc
            phx
            phy

            cmp #10
            bne not_cr

newline     ldx #0
            ldy row
            iny
            jsr set_xy
            bra done

not_cr      jsr mmu.go_io_2
            ldy column
            sta (line_ptr),y

            jsr mmu.go_io_3
            lda color
            sta (line_ptr),y

            iny
            cpy num_cols
            bge newline

            sty column
            ldx column
            ldy row
            jsr set_xy

done        ply
            plx
            rts
            .pend

;
; Print an ASCII-Z string to the screen
;
; X = pointer to the string to print (16-bit)
;
puts        .proc
            pha

loop        lda 0,x
            beq done
            jsr put

            inx
            bra loop

done        pla
            rts
            .pend

;
; Define our color lookup table
;
setclut     .proc
            jsr mmu.go_io_0

            ; Set black
            lda #0
            sta TXT_CLUT_FG
            sta TXT_CLUT_FG+1
            sta TXT_CLUT_FG+2

            sta TXT_CLUT_BG
            sta TXT_CLUT_BG+1
            sta TXT_CLUT_BG+2

            ; Set blue
            lda #128
            sta TXT_CLUT_FG+4*2
            sta TXT_CLUT_BG+4*2
            lda #0
            sta TXT_CLUT_FG+4*2+1
            sta TXT_CLUT_BG+4*2+1
            lda #0
            sta TXT_CLUT_FG+4*2+2
            sta TXT_CLUT_BG+4*2+2

            ; Set white
            lda #255
            sta TXT_CLUT_FG+4*15
            sta TXT_CLUT_BG+4*15
            sta TXT_CLUT_FG+4*15+1
            sta TXT_CLUT_BG+4*15+1
            sta TXT_CLUT_FG+4*15+2
            sta TXT_CLUT_BG+4*15+2

            rts
            .pend

;
; Initialize the text system and screen
;
init        .proc
            ; Make sure we're in text mode
            jsr mmu.go_io_0

            lda #1
            sta mstr_ctrl_0
            stz mstr_ctrl_1

            ; Turn off the text cursor
            stz crsr_ctrl

            ; Turn off the border
            stz brdr_ctrl

            ; Set up the CLUT
            jsr setclut

            ; Set the default color
            lda #$f2
            sta color

            ; Set the screen dimensions
            ldx #80
            stx num_cols
            ldy #60
            sty num_rows

            jsr clear

            rts
            .pend

            .endsection
            .endnamespace