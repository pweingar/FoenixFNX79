;;;
;;; Code to test reading raw PS/2 keyboard data
;;;

            .cpu "65816"

MMU_MEM_CTRL = $0000
MMU_IO_CTRL = $0001

PS2_CTRL = $d640
PS2_CTRL_KBD_WR = $02
PS2_CTRL_MS_WR = $04
PS2_CTRL_KBD_CLR = $10
PS2_CTRL_MS_CLR = $20

PS2_OUT = $d641
PS2_KBD_IN = $d642
PS2_MS_IN = $d643

PS2_STAT = $d644
PS2_STAT_KBD_ACK = $80
PS2_STAT_KBD_NAK = $40
PS2_STAT_MS_ACK = $20
PS2_STAT_MS_NAK = $10
PS2_STAT_MS_EMPTY = $02
PS2_STAT_KB_EMPTY = $01

TXT_CHAR_MATRIX = $c000     ; Text matrix
TXT_COLOR_MATRIX = $c000    ; Color matrix

; STATUS_PORT 	= $AF1807
; KBD_STATUS      = $AF1807
; KBD_CMD_BUF		= $AF1807
; KBD_OUT_BUF 	= $AF1803
; KBD_INPT_BUF	= $AF1803
; KBD_DATA_BUF	= $AF1803

; OUT_BUF_FULL    = $01
; INPT_BUF_FULL	= $02
; SYS_FLAG		= $04
; CMD_DATA		= $08
; KEYBD_INH       = $10
; TRANS_TMOUT	    = $20
; RCV_TMOUT		= $40
; PARITY_EVEN		= $80
; INH_KEYBOARD	= $10
; KBD_ENA			= $AE
; KBD_DIS			= $AD

; TXT_CHAR_MATRIX = $afa000   ; Text matrix
; TXT_COLOR_MATRIX = $afc000  ; Color matrix

TXT_COLOR = $f0

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

            jsr clrscr

            lda #'>'
            jsr pr
            lda #' '
            jsr pr

            lda #0
            jsr pr_sc

loop        jsr get_sc
            jsr pr_sc
            bra loop

;
; Poll the keyboard for a scan code
;
get_sc      .proc
            stz MMU_IO_CTRL

; _wait       lda PS2_STAT
;             and #PS2_STAT_KB_EMPTY
;             cmp #PS2_STAT_KB_EMPTY
;             beq _wait

            lda PS2_KBD_IN
            
_done       rts
            .pend

pr          .proc
                        ; Switch to the text matrix
            pha
            lda #$02
            sta MMU_IO_CTRL
            pla
            sta TXT_CHAR_MATRIX,x

            ; Switch to the color matrix
            lda #$03
            sta MMU_IO_CTRL
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
            ldx #79

_loop       lda #$02
            sta MMU_IO_CTRL
            lda #' '
            sta TXT_CHAR_MATRIX,x

            lda #$03
            sta MMU_IO_CTRL
            lda #TXT_COLOR
            sta TXT_COLOR_MATRIX,x

            dex
            bpl _loop

            ldx #0

            rts
            .pend
