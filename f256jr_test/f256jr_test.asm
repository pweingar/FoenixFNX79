;;;
;;; Code to test reading raw PS/2 keyboard data
;;;

            .cpu "65816"

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

;;
;; Imports
;;

            .include "f256_sections.asm"
            .include "f256_mmu.asm"
            .include "f256_text.asm"

;;
;; Variables
;;

            .section variables
scancode    .byte ?
leds        .byte ?
is_release  .byte ?
red         .byte ?
green       .byte ?
blue        .byte ? 
            .endsection

;;
;; Vectors
;;

            .section hreset
vreset      .word <>start
            .endsection

;;
;; Code
;;

            .section code
;
; Main code
;
start       sei
            clc
            xce

            ; Set the direct page
            rep #$20
            .al
            lda #<>DP_BASE
            tcd
            .dpage DP_BASE

            ; 8-bit accumulator and 16-bit index registers will be standard
            sep #$20
            rep #$10
            .as
            .xl

            ; Set the data bank register to 0
            lda #0
            pha
            plb
            .databank 0

            stz leds
            stz is_release

            jsr text.init

            ; Print the greeting
            ldx #0
            ldy #0
            jsr text.set_xy

            ldx #<>greeting
            jsr text.puts

            ; Print the prompt

            ldx #0
            ldy #3
            jsr text.set_xy

            ldx #<>prompt
            jsr text.puts

loop        jsr get_sc
            cmp #0
            beq loop

            ; Check to see if it is the release prefix
            cmp #$f0
            bne _not_f0
            lda #1
            sta is_release

_wait2      jsr get_sc
            cmp #0
            beq _wait2
            sta scancode

_not_f0     cmp #$e0
            bne _not_e0

            ldx #<>prefix_e0
            jsr text.puts
            bra loop

_not_e0     nop

            ; Check to see if it's the CAPS lock press
            cmp #$58
            bne _not_caps

            ; It is CAPS... make sure it is the release
            lda is_release
            beq _not_caps

            ; It is... toggle the CAPS LED
            jsr caps_tog

_not_caps   cmp #$16            ; '1'
            bne _not_1

            lda #1
            jsr led_on
            bra _process

_not_1      cmp #$1e            ; '2'
            bne _not_2

            lda #1
            jsr led_off
            bra _process

_not_2      cmp #$05            ; 'F1'
            bne _not_f1

            stz blue            ; Set LED to red
            stz green
            lda #255
            sta red
            lda #3
            jsr led_rgb
            bra _process

_not_f1     cmp #$06            ; 'F2'
            bne _not_f2

            stz blue            ; Set LED to green
            lda #255
            sta green
            stz red
            lda #3
            jsr led_rgb
            bra _process

_not_f2     cmp #$04            ; 'F3'
            bne _not_f3

            lda #255            ; Set LED to blue
            sta blue
            stz green
            stz red
            lda #3
            jsr led_rgb
            bra _process

_not_f3     nop

_process    lda is_release
            beq _skip_f0

            ldx #<>prefix_f0
            jsr text.puts

_skip_f0    lda scancode
            jsr pr_sc
            stz is_release
            jmp loop

;
; Toggle the CAPS lock key LED
;
caps_tog    .proc
            lda leds
            beq set_caps

            stz leds
            bra send_caps

set_caps    lda #$04
            sta leds

send_caps   lda #$ed
            jsr kbd_send

            lda leds
            jsr kbd_send

            lda leds
            beq nocaps

            ; Print the caps lock prompt

            ldx #0
            ldy #3
            jsr text.set_xy
            
            ldx #<>prompt2
            jsr text.puts
            
nocaps      ; Print the no caps lock prompt
            ldx #0
            ldy #3
            jsr text.set_xy
            
            ldx #<>prompt
            jsr text.puts

_done       rts
            .pend

;
; Send a byte to the keyboard
;
kbd_send    .proc
            sta PS2_OUT

            lda #PS2_CTRL_KBD_WR
            sta PS2_CTRL
            stz PS2_CTRL

_wait       jsr get_sc
            cmp #0
            beq _wait

_done       rts
            .pend

;
; Poll the keyboard for a scan code
;
get_sc      .proc
            phy
            ldy #100
            stz MMU_IO_CTRL

_wait       lda PS2_STAT
            and #PS2_STAT_KB_EMPTY
            cmp #PS2_STAT_KB_EMPTY
            bne _ready

            dey
            bne _wait

            lda #0
            bra _done

_ready      lda PS2_KBD_IN
            sta scancode
            
_done       ply
            rts
            .pend

pr_hex      .proc
            pha

            rep #$20
            .al
            and #$000f
            tay
            sep #$20
            .as

            lda hex_digits,y

            jsr text.put

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

            ldx #<>blanks
            jsr text.puts

            jsr text.get_xy
            ldx #2
            jsr text.set_xy
            
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

prompt      .null "> "
prompt2     .null "] "
prefix_f0   .null "F0 "
prefix_e0   .null "E0 "
greeting    .null "F256 PS/2 Keyboard Test Harness",10,"Press keys to see scan codes:",10,10
blanks      .null "      "

            .endsection