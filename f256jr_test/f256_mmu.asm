;;;
;;; Definitions for the F256 memory management unit
;;;

MMU_MEM_CTRL = $0000
MMU_IO_CTRL = $0001
            
            .dpage DP_BASE
            .databank 0
            .as
            .xl

mmu         .namespace

            .section code
            .as
            .xl

;
; Go to I/O page 0
;
go_io_0:    .proc
            pha
            stz MMU_IO_CTRL
            pla
            rts
            .pend

;
; Go to I/O page 1
;
go_io_1:    .proc
            pha
            lda #$01
            sta MMU_IO_CTRL
            pla
            rts
            .pend

;
; Go to I/O page 2
;
go_io_2:    .proc
            pha
            lda #$02
            sta MMU_IO_CTRL
            pla
            rts
            .pend

;
; Go to I/O page 3
;
go_io_3:    .proc
            pha
            lda #$03
            sta MMU_IO_CTRL
            pla
            rts
            .pend

            .endsection
            .endnamespace
