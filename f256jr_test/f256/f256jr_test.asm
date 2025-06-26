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
;; Local constants
;;

MAX_SEQUENCE = 10

HOME_X = 2
HOME_Y = 3

STATE_DEFAULT = 0
STATE_E0 = 1
STATE_F0 = 2
STATE_E0F0 = 3

;;
;; Variables
;;

            .section variables
state       .byte ?
sc_count    .byte ?
scancode    .byte ?
leds        .byte ?
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

            stz state
            stz sc_count
            stz leds
            stz red
            stz green
            stz blue

            jsr text.init

            ; Print the greeting
            ldx #0
            ldy #0
            jsr text.set_xy

            ldx #<>greeting
            jsr text.puts

            ; Print the prompt

pr_prompt   ldx #0
            ldy #HOME_Y
            jsr text.set_xy

            jsr text.clr_line

            stz sc_count

            ldx #<>prompt
            jsr text.puts

loop        jsr get_sc
            cmp #0
            beq loop

            sta scancode

            ; Dispatch on the state
            lda state
            beq st_default
            cmp #STATE_E0
            beq st_e0
            cmp #STATE_F0
            beq st_f0
            cmp #STATE_E0F0
            beq st_e0f0

            bra loop

;
; STATE_DEFAULT -- 
;
st_default  lda sc_count                ; clear the line if we have printed 10 sequences
            cmp #MAX_SEQUENCE
            blt dont_clear

            ldx #HOME_X
            ldy #HOME_Y
            jsr text.set_xy

            jsr text.clr_line

            stz sc_count

dont_clear  ; Dispatch based on the scan code
            lda scancode
            cmp #$e0
            bne not_e0

            ; Got E0... print it and move to STATE_E0

            ldx #<>prefix_e0
            jsr text.puts

            lda #STATE_E0
            sta state
            jmp loop

not_e0      cmp #$f0
            bne not_f0

            ; Got F0... print it and move to STATE_F0

            ldx #<>prefix_f0
            jsr text.puts

            lda #STATE_F0
            sta state
            jmp loop

not_f0      ; Any other character, print it, and repeat
            jsr sc_end          ; Print the code and end the sequence
            jmp loop

;
; STATE_E0 --- prefix for special characters
;

st_e0       lda scancode
            cmp #$f0
            bne not_e0f0

            ; Got E0 F0... move to STATE_E0F0

            ldx #<>prefix_f0
            jsr text.puts

            lda #STATE_E0F0
            sta state

            jmp loop

not_e0f0    ; Got any other code, print it and go back to DEFAULT
            jsr sc_end          ; Print the code and end the sequence

            stz state           ; Go back to the default state
            jmp loop

;
; STATE_F0 -- Release of standard characters
;

st_f0       lda scancode
            jsr sc_end          ; Print the code and end the sequence

            lda scancode
            cmp #$58
            bne go_default

            jsr caps_tog

go_default  stz state           ; Go back to the default state
            jmp loop

;
; STATE_E0F0 -- Release of special characters
;

st_e0f0     lda scancode
            jsr sc_end          ; Print the code and end the sequence

            stz state           ; Go back to the default state
            jmp loop

;
; Print the end of a scan code sequence
;
sc_end      .proc
            jsr pr_sc           ; Print the scan code
            lda #' '
            jsr text.put

            inc sc_count        ; Add to the sequence count

            lda sc_count
            cmp #MAX_SEQUENCE
            beq done

            lda #'/'            ; Print a sequence end marker
            jsr text.put
            lda #' '
            jsr text.put

done        rts
            .pend

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

            jsr pr_sc

            lda leds
            jsr kbd_send

            jsr pr_sc

            lda leds
            beq nocaps

            ; Print the caps lock prompt

            jsr text.get_xy

            phx
            phy

            ldx #0
            ldy #HOME_Y
            jsr text.set_xy
            
            lda #']'
            jsr text.put

            ply
            plx
            jsr text.set_xy

            bra done
            
nocaps      ; Print the no caps lock prompt
            jsr text.get_xy
            phx
            phy

            ldx #0
            ldy #HOME_Y
            jsr text.set_xy
            
            lda #'>'
            jsr text.put

            ply
            plx
            jsr text.set_xy

done        rts
            .pend

;
; Send a byte to the keyboard
;
; kbd_send    .proc
;             sta PS2_OUT

;             pha
;             ldx #<>send_pre
;             jsr text.puts

;             pla
;             jsr pr_sc
;             ldx #<>send_post
;             jsr text.puts

;             lda #PS2_CTRL_KBD_WR
;             sta PS2_CTRL
;             stz PS2_CTRL

; ; _wait       jsr get_sc
; ;             cmp #0
; ;             beq _wait

; _done       rts
;             .pend

;
; Send a byte to the keyboard
;
kbd_send    .proc
            stz MMU_IO_CTRL
            sta PS2_OUT

            lda #PS2_CTRL_KBD_WR
            sta PS2_CTRL
            stz PS2_CTRL

wait        jsr get_sc
            cmp #0
            beq wait

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
send_pre    .null "["
send_post   .null "] "

            .endsection