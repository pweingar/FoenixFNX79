;;;
;;; Code to test reading raw PS/2 keyboard data
;;;

            .cpu "65816"

STATUS_PORT 	= $AF1064
KBD_STATUS      = $AF1064
KBD_OUT_BUF 	= $AF1060
KBD_INPT_BUF	= $AF1060
KBD_CMD_BUF		= $AF1064
KBD_DATA_BUF	= $AF1060
PORT_A		    = $AF1060
PORT_B			= $AF1061

; STATUS_PORT 	= $AF1807
; KBD_STATUS      = $AF1807
; KBD_CMD_BUF		= $AF1807
; KBD_OUT_BUF 	= $AF1803
; KBD_INPT_BUF	= $AF1803
; KBD_DATA_BUF	= $AF1803

OUT_BUF_FULL    = $01
INPT_BUF_FULL	= $02
SYS_FLAG		= $04
CMD_DATA		= $08
KEYBD_INH       = $10
TRANS_TMOUT	    = $20
RCV_TMOUT		= $40
PARITY_EVEN		= $80
INH_KEYBOARD	= $10
KBD_ENA			= $AE
KBD_DIS			= $AD

TXT_CHAR_MATRIX = $afa000   ; Text matrix
TXT_COLOR_MATRIX = $afc000  ; Color matrix

TXT_COLOR = $f0

;;
;; Variables
;;

* = $3000
txt_line    .dword ?
color_line  .dword ?
caps        .byte ?

;;
;; Code
;;

* = $fffc
vreset      .word <>start

* = $2000

;
; Main code
;
start       sei
            sec
            xce

            sep #$30
            .as
            .xs

            jsr clrscr

            lda #'>'
            jsr pr
            lda #' '
            jsr pr

            lda #0
            jsr pr_sc

loop        jsr get_sc

            ; cmp #$3a
            ; bne _send

            ; pha
            ; jsr toggle
            ; pla

_send       jsr pr_sc
            bra loop

kbd_send    .proc
            jsr kbd_wait_in
            sta KBD_OUT_BUF

            rts
            .pend

toggle      .proc
            lda caps
            beq _set

            stz caps
            bra _update

_set        lda #$04
            sta caps

_update     lda #$ed
            jsr kbd_send

            lda caps
            sta kbd_send

            rts
            .pend

;
; Wait for the keyboard to be ready to send data to the CPU
;
kbd_wait_out        .proc
                    pha
wait                lda @l KBD_STATUS       ; Get the keyboard status
                    bit #OUT_BUF_FULL       ; Check to see if the output buffer is full
                    beq wait                ; If it isn't, keep waiting
                    pla
                    rts
                    .pend

;
; Wait for the keyboard to be ready to receive data from the CPU
;
kbd_wait_in         .proc
                    pha
wait                lda @l KBD_STATUS       ; Get the keyboard status
                    bit #INPT_BUF_FULL      ; Check to see if the input buffer has data
                    bne wait                ; If not, wait for it to have something
                    pla
                    rts
                    .pend

;
; Poll the keyboard for a scan code
;
get_sc      .proc
            jsr kbd_wait_out
            lda KBD_DATA_BUF
            
_done       rts
            .pend

pr          .proc
            sta TXT_CHAR_MATRIX,x

            ; Switch to the color matrix
            lda #TXT_COLOR
            sta TXT_COLOR_MATRIX,x

            inx

            rts
            .pend

pr_hex      .proc
            pha

            and #$0f
            tay
            lda hex_digits,y

            jsr pr

            pla
            rts
            .pend

hex_digits  .null "0123456789ABCDEF"

pr_sc       .proc
            ldx #2
            
            pha
            lsr a
            lsr a
            lsr a
            lsr a
            jsr pr_hex
            pla

            jsr pr_hex
            
            rts
            .pend

clrscr      .proc
            lda #<TXT_CHAR_MATRIX
            sta txt_line
            lda #>TXT_CHAR_MATRIX
            sta txt_line+1
            lda #`TXT_CHAR_MATRIX
            sta txt_line+3

            lda #<TXT_COLOR_MATRIX
            sta color_line
            lda #>TXT_COLOR_MATRIX
            sta color_line+1
            lda #`TXT_COLOR_MATRIX
            sta color_line+3    

            ldy #60

loop2       ldx #79

_loop       lda #' '
            sta TXT_CHAR_MATRIX,x

            lda #TXT_COLOR
            sta TXT_COLOR_MATRIX,x

            dex
            bpl _loop

            clc
            lda txt_line
            adc #80
            sta txt_line
            lda txt_line+1
            adc #0
            sta txt_line+1
            
            clc
            lda color_line
            adc #80
            sta color_line
            lda color_line+1
            adc #0
            sta color_line+1

            dey
            bpl loop2

            ldx #0

            rts
            .pend
