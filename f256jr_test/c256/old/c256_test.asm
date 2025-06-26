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

MASTER_CTRL_REG_L = $af0000
VKY_TXT_CURSOR_CTRL_REG = $af0010
BORDER_CTRL_REG = $af0004
MOUSE_PTR_CTRL_REG_L = $af0700

TXT_COLOR = $f4

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

            rep #$30
            .as
            .xs

            ; Initialize the variables to something sane
            stz scancode
            stz leds
            stz is_release
            stz red
            stz green
            stz blue
           
            ; Switch to text-only mode
            lda #1
            sta @l MASTER_CTRL_REG_L

            ; Disable border, text cursor, and mouse cursor
            lda #0
            sta @l BORDER_CTRL_REG
            sta @l MOUSE_PTR_CTRL_REG_L
            sta @l VKY_TXT_CURSOR_CTRL_REG

            ; Clear the screen
            jsr clrscr

            ; Set LED #1 to cyan

            ; lda #128        ; Blue
            ; pha
            ; lda #128        ; Green
            ; pha
            ; lda #0          ; Red
            ; pha
            ; lda #1          ; LED #1
            ; jsr led_rgb
            ; pla             ; Clean the stack
            ; pla
            ; pla

            ; Print the prompt
            lda #'>'
            jsr pr
            lda #' '
            jsr pr

loop        jsr get_sc
            cmp #0
            beq loop

            ; Check to see if it is the release prefix
            cmp #$f0
            bne _not_prefix
            lda #1
            sta is_release

_wait2      jsr get_sc
            cmp #0
            beq _wait2
            sta scancode

_not_prefix ; Check to see if it's the CAPS lock press
            cmp #$3a
            bne _not_caps

            ; It is CAPS... make sure it's not the release
            lda is_release
            bne _not_caps

            ; It is... toggle the CAPS LED
            jsr caps_tog

_not_caps   nop
;             cmp #$16            ; '1'
;             bne _not_1

;             lda #1
;             jsr led_on
;             bra _process

; _not_1      cmp #$1e            ; '2'
;             bne _not_2

;             lda #1
;             jsr led_off
;             bra _process

; _not_2      cmp #$05            ; 'F1'
;             bne _not_f1

;             stz blue            ; Set LED to red
;             stz green
;             lda #255
;             sta red
;             lda #3
;             jsr led_rgb
;             bra _process

; _not_f1     cmp #$06            ; 'F2'
;             bne _not_f2

;             stz blue            ; Set LED to green
;             lda #255
;             sta green
;             stz red
;             lda #3
;             jsr led_rgb
;             bra _process

; _not_f2     cmp #$04            ; 'F3'
;             bne _not_f3

;             lda #255            ; Set LED to blue
;             sta blue
;             stz green
;             stz red
;             lda #3
;             jsr led_rgb
;             bra _process

; _not_f3     nop

_process    lda is_release
            beq _skip_f0

            lda #'F'
            jsr pr
            lda #'0'
            jsr pr
            lda #' '
            jsr pr

_skip_f0    lda scancode
            jsr pr_sc
            stz is_release
            jmp loop

;
; Toggle the CAPS lock key LED
;
caps_tog    .proc
            lda leds
            eor #$04
            sta leds

            lda #$ed
            jsr kbd_send

            lda leds
            jsr kbd_send

            lda leds
            beq _nocaps

            ldx #0
            lda #']'
            jsr pr
            lda #' '
            jsr pr
            bra _done
            
_nocaps     ldx #0
            lda #'>'
            jsr pr
            lda #' '
            jsr pr

_done       rts
            .pend

;
; Send a byte to the keyboard
;
kbd_send    .proc
            pha

_wait1      lda @l STATUS_PORT
            and #INPT_BUF_FULL
            cmp #INPT_BUF_FULL
            beq _wait1

            pla
            sta @l KBD_DATA_BUF

_wait2      jsr get_sc
            cmp #0
            beq _wait2


_done       rts
            .pend

;
; Poll the keyboard for a scan code
;
get_sc      .proc
            phy
            ldy #100

_wait       lda @l KBD_STATUS
            and #OUT_BUF_FULL
            cmp #OUT_BUF_FULL
            beq _ready

            dey
            bne _wait

            lda #0
            bra _done

_ready      lda @l KBD_DATA_BUF
            sta scancode
            
_done       ply
            rts
            .pend

pr          .proc
            sta @l TXT_CHAR_MATRIX,x

            lda #TXT_COLOR
            sta @l TXT_COLOR_MATRIX,x

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
            pha
            lsr a
            lsr a
            lsr a
            lsr a
            jsr pr_hex
            pla

            jsr pr_hex

            .rept 10
            lda #' '
            jsr pr
            .endrept

            ldx #2
            
            rts
            .pend

clrscr      .proc
            ldx #0

_loop       lda #' '
            sta @l TXT_CHAR_MATRIX,x
            sta @l TXT_CHAR_MATRIX+$100,x
            sta @l TXT_CHAR_MATRIX+$200,x
            sta @l TXT_CHAR_MATRIX+$300,x
            sta @l TXT_CHAR_MATRIX+$400,x
            sta @l TXT_CHAR_MATRIX+$500,x
            sta @l TXT_CHAR_MATRIX+$600,x
            sta @l TXT_CHAR_MATRIX+$700,x
            sta @l TXT_CHAR_MATRIX+$800,x
            sta @l TXT_CHAR_MATRIX+$900,x
            sta @l TXT_CHAR_MATRIX+$a00,x
            sta @l TXT_CHAR_MATRIX+$b00,x
            sta @l TXT_CHAR_MATRIX+$c00,x
            sta @l TXT_CHAR_MATRIX+$d00,x
            sta @l TXT_CHAR_MATRIX+$e00,x
            sta @l TXT_CHAR_MATRIX+$f00,x
            sta @l TXT_CHAR_MATRIX+$1000,x
            sta @l TXT_CHAR_MATRIX+$1100,x
            sta @l TXT_CHAR_MATRIX+$1200,x
            sta @l TXT_CHAR_MATRIX+$1300,x

            lda #TXT_COLOR
            sta @l TXT_COLOR_MATRIX,x
            sta @l TXT_COLOR_MATRIX+$100,x
            sta @l TXT_COLOR_MATRIX+$200,x
            sta @l TXT_COLOR_MATRIX+$300,x
            sta @l TXT_COLOR_MATRIX+$400,x
            sta @l TXT_COLOR_MATRIX+$500,x
            sta @l TXT_COLOR_MATRIX+$600,x
            sta @l TXT_COLOR_MATRIX+$700,x
            sta @l TXT_COLOR_MATRIX+$800,x
            sta @l TXT_COLOR_MATRIX+$900,x
            sta @l TXT_COLOR_MATRIX+$a00,x
            sta @l TXT_COLOR_MATRIX+$b00,x
            sta @l TXT_COLOR_MATRIX+$c00,x
            sta @l TXT_COLOR_MATRIX+$d00,x
            sta @l TXT_COLOR_MATRIX+$e00,x
            sta @l TXT_COLOR_MATRIX+$f00,x
            sta @l TXT_COLOR_MATRIX+$1000,x
            sta @l TXT_COLOR_MATRIX+$1100,x
            sta @l TXT_COLOR_MATRIX+$1200,x
            sta @l TXT_COLOR_MATRIX+$1300,x

            inx
            beq _done
            jmp _loop

_done       ldx #0

            rts
            .pend

;
; Turn on an RGB LED
;
; A = index of the LED
;
led_on      .proc
            pha
            lda #$e1
            jsr kbd_send

            pla
            jsr kbd_send
            rts
            .pend

;
; Turn off an RGB LED
;
; A = index of the LED
;
led_off      .proc
            pha
            lda #$e2
            jsr kbd_send

            pla
            jsr kbd_send
            rts
            .pend

;
; Set the color of an LED
;
; A = index of the LED
; 5,s = blue
; 4,s = green
; 3,s = red
;
led_rgb     .proc
            pha
            lda #$e0
            jsr kbd_send

            pla
            jsr kbd_send

            lda red
            jsr kbd_send

            lda green
            jsr kbd_send

            lda blue
            jsr kbd_send

            rts
            .pend

scancode    .byte 0
leds        .byte 0
is_release  .byte 0
red         .byte 0
green       .byte 0
blue        .byte 0
