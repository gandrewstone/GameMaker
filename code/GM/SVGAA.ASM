; svgaa.asm - generic 320x200x256 color routines (SVGA replacment for gena.asm)
; Programmer:  Andy Stone    Created: 12/27/90
; Last Edited: 2/20/91

        .MODEL LARGE
        IDEAL
        LARGESMALL EQU 6

        CRTCINDEX EQU   03d4h
        CRTCDATA  EQU   (CRTCINDEX+1)

        EXTRN  _zeroaddon:WORD
        EXTRN  _xor:WORD
        EXTRN  _curpage:BYTE
        EXTRN  _zeropage:BYTE
        EXTRN  _ScratchSeg:WORD


        PUBLIC _SoftwareScroll
        PUBLIC _ATIExtReg
        PUBLIC _Vcard
        PUBLIC _Point
        PUBLIC _GetCol
        PUBLIC _BoxFill
        PUBLIC _Gwritestr
        PUBLIC _SvgaBufToScrn
        PUBLIC _SetPage
        PUBLIC _SetTrident
        PUBLIC _SetTridentStart
        PUBLIC _NewTimerAsm
        PUBLIC _TimerCounter
        PUBLIC _GetROMFont
        PUBLIC _MoveViewScreen


Unknown      EQU 0
ATI          EQU 1
ChipTech     EQU 2
Genoa        EQU 3
Paradise     EQU 4
Trident8800  EQU 5
Trident8900  EQU 6
Tseng3000    EQU 7
Tseng4000    EQU 8
Video7       EQU 9
VESA         EQU 10
ATIWonder    EQU 11
SoftwareSim  EQU 12


        DATASEG
        _TimerCounter DD  1
        OldAddr       DW  0
        OldHi         DB  0
        OldLit        DB  0
        _ScrollWait   DB  0
        CODESEG
        _Vcard        DW  0

PROC    _MoveViewScreen
NEWPOS     EQU [bp+LARGESMALL]
NEWPOSHIGH EQU [bp+LARGESMALL+2]
NEWLITTLE  EQU [bp+LARGESMALL+4]

;        mov        ax,[_Vcard]        ;; SoftwareSim will never call this fn.
;        cmp        ax,SoftwareSim
;        jne        @@FuncStart
;        ret

@@FuncStart:
        push bp
        mov  bp,sp

        mov        cx,NEWPOS
        mov        bx,[OldAddr]

@@WaitOutRetrace:
        mov        dx,03DAh
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace


        mov        dx,CRTCINDEX
        cmp        ch,bh
        je         @@SkipSetHigh
        mov        al,0Ch               ; Set Normal VGA high byte.
        mov        ah,ch
        out        dx,ax
@@SkipSetHigh:

        cmp        cl,bl
        je         @@SkipSetLow         ; Set Normal VGA low byte.
        mov        al,0Dh               ; will be 0Dh
        mov        ah,cl
        out        dx,ax

@@SkipSetLow:

        mov        [OldAddr],cx         ; Save the new address.

        mov        ax,[cs:_Vcard]
        cmp        ax,Unknown           ; Normal VGA - skip all high setting.
        je         @@SkipHigh

        mov        cx,NEWPOSHIGH        ; Load the new position.

        cmp        ax,Tseng4000
        jne        @@NotTseng

        and        cl,3            ; TSENG ET4000 set start address code.
        cmp        cl,[OldHi]
        je         @@SkipHigh
        mov        dx,03d4h
        mov        al,033h
        mov        ah,cl
        out        dx,ax
;        out        dx,al         ; The old byte by byte method.
;        inc        dx
;        mov        al,cl
;        out        dx,al

        jmp        @@SkipHigh
@@NotTseng:
        cmp        ax,Trident8900
        je         @@DoTrident
        cmp        ax,Trident8800
        jne        @@SkipHigh
        @@DoTrident:
        and        cl,1
        cmp        cl,[OldHi]
        je         @@SkipHigh
        push       cx
        call far   _SetTridentStart
        add        sp,2

@@SkipHigh:
        mov        bx,NEWLITTLE
        shl        bl,1
        cmp        bl,[OldLit]
        je         @@SkipLittle

        mov        dx,03DAh
@@WaitOutRetrace1:
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace1
@@WaitForRetrace:
        in         al,dx
        test       al,8
        jz         @@WaitForRetrace

        mov        dx,03dah     ; Move 0-3 pixels left (little x scroll)
        in         al,dx        ; Reset flip-flop
        mov        dx,03C0h
        mov        al,(13h+20h)
        out        dx,al        ; Output index w/out destroying bit 20h
        mov        al,bl
        out        dx,al
        mov        [OldLit],bl
@@SkipLittle:

        mov        [OldHi],cl
        pop  bp
        ret
ENDP    _MoveViewScreen

PROC    _GetROMFont
        push       es
        push       bp
        mov        ax,1130h
        mov        bh,3
        int        10h

        mov        dx,es
        mov        ax,bp
        pop        bp
        pop        es
        ret
ENDP    _GetROMFont

PROC    _SoftwareScroll
       push    di
       push    si
       push    es
       push    ds

       mov     ax,0A000h
       mov     es,ax
       xor     di,di
       mov     si,[_zeroaddon]
       mov     bx,[_ScratchSeg]
       mov     ds,bx

 @@WaitOutRetrace:
        mov        dx,03DAh
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace
@@WaitForRetrace:
        mov        dx,03DAh
        in         al,dx
        test       al,8
        jz         @@WaitForRetrace

        cmp     si,1536
        ja      @@WrapAround

        mov     cx,32000
        rep     movsw
        jmp     @@EndFunc


@@WrapAround:
       mov     ax,si
       mov     bx,0FFFFh
       sub     bx,ax
       inc     bx             ; bx is the length to the buffer end.

@@WriteStart:
       test    si,1
jz     @@EvenStart
       movsb
       dec     bx
@@EvenStart:
       mov     cx,bx
       shr     cx,1
       rep     movsw
       test    bx,1           ; We know that bx = 1 here because
jz     @@EvenMiddle
       movsb
@@EvenMiddle:
;       sub     ax,1536        ; Only copy 64000 not 64k (65536)
       mov     si,0
       mov     cx,ax
       shr     cx,1
       rep     movsw
       test    ax,1
       jz      @@EvenEnd
       movsb
@@EvenEnd:
@@EndFunc:
       pop     ds
       pop     es
       pop     si
       pop     di
       ret
ENDP    _SoftwareScroll


PROC    _SvgaBufToScrn
        LEN       EQU [BP+LARGESMALL]
        OFFS      EQU [BP+2+LARGESMALL]
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    es
        push    ds


        mov     dl,[_curpage]        ; Fetch current page num prior to DS remap
        mov     dh,[_zeropage]       ; Fetch last page num prior to DS remap

        mov     bx,LEN
        cmp     bx,0
        jne     @@Cont
        jmp     @@EndFunc
@@Cont:
        cld

        and     dl,0Fh
        and     dh,0Fh

        mov     di,OFFS
        mov     ax,0A000h
        mov     si,di
        mov     es,ax
        cmp     di,[_zeroaddon]
        mov     cx,[_ScratchSeg]
        mov     ds,cx
        jae     @@UseZeroPage

        ; Start with page after zeropage.
        cmp     dl,dh
        jg      @@WriteLastPageOnly
        mov     dl,dh           ; from above: invaraint dl=dh is NOT TRUE
        inc     dl              ; try taking out this line!  Why?
        push    dx
        call    FAR PTR _SetPage
        add     sp,2

@@WriteLastPageOnly:
        mov     cx,bx           ; cx = LEN
        test    si,01b          ; cx must be > 0, so ok. (look top)
        jz      @@SkipByte3     ; This is all done to write as many word
        movsb                   ; aligned words as possible (2x faster).
;        mov     al,15h
;        stosb
        dec     cx
@@SkipByte3:
        mov     ax,cx
        shr     cx,1
        cmp     cx,0
        je      @@SkipIntegers
;        push    ax                 ; Put 1 color down. - debugging
;        mov     ax,1515h
;        rep     stosw
;        pop     ax
        rep     movsw               ; Copy the buffer to the screen.
@@SkipIntegers:
        test    ax,01b
        jz      @@EndFunc
;        mov     al,15h             ; Put 1 color down. - debugging
;        stosb
        movsb
        jmp     @@EndFunc

@@UseZeroPage:
        cmp     dl,dh
        je      @@InitPageSet
        mov     dl,dh           ; curpage gets zeropage
        push    dx
        call    FAR PTR _SetPage
        add     sp,2

@@InitPageSet:

        mov     cx,0FFFFh
        sub     cx,di                   ; cx = length to end of page
        add     cx,1
        jc      @@NoBreak
        cmp     cx,bx                   ; cmp len to end with len to write
        jbe     @@PageBreak
@@NoBreak:
        mov     cx,bx                   ; len is small enough - no break
        mov     bx,0
        jmp @@DoFirstWrite

@@PageBreak:
        sub     bx,cx                   ; bx gets Amount to write after switch

@@DoFirstWrite:
        cmp     cx,0
        je      @@DoAfterWrap
        test    si,01b
        jz      @@SkipByte1
;        mov     al,15h
;        stosb
        movsb
        dec     cx
@@SkipByte1:
        mov     ax,cx
        shr     cx,1
        cmp     cx,0
        je      @@SkipIntegers2
        rep     movsw
@@SkipIntegers2:
        test    ax,01b
        jz      @@DoAfterWrap
        movsb
@@DoAfterWrap:
        cmp     bx,0
        je      @@EndFunc
        mov     dl,dh
        inc     dl
        push    dx
        call    FAR PTR _SetPage
        add     sp,2
        mov     cx,bx
        rep     movsb

@@EndFunc:
        pop     ds
        mov     [_curpage],dl           ; Restore the current page variable
        pop     es
        pop     si
        pop     di
        pop     bp
        ret
ENDP    _SvgaBufToScrn


PROC    _NewTimerAsm
        push    ax
        push    dx
        add     [WORD PTR _TimerCounter],1
        adc     [WORD PTR _TimerCounter+2],0
        mov     dx,20h
        mov     al,20h
        out     dx,al
        pop     dx
        pop     ax
        iret
ENDP    _NewTimerAsm

_ATIExtReg  DW 0

PROC    _SetPage
PAGENUM EQU [BP+LARGESMALL]
        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    dx
        push    ds

        mov     ax,[cs:_Vcard]
        mov     bx,PAGENUM
        cmp     ax,Unknown
        je      @@jUnknown
        cmp     ax,ATI
        je      @@jATI
        cmp     ax,ChipTech
        je      @@jChipTech
        cmp     ax,Genoa
        je      @@jGenoa
        cmp     ax,Paradise
        je      @@jParadise
        cmp     ax,Trident8800
        je      @@jTrident
        cmp     ax,Trident8900
        je      @@jTrident
        cmp     ax,Tseng3000
        je      @@jTseng3000
        cmp     ax,Video7
        je      @@jVideo7
        cmp     ax,ATIWonder
        je      @@jATIWonder
        cmp     ax,Tseng4000
        je      @@jTseng4000
        jmp @@endfunc

@@jUnknown:
@@jParadise:            ; Set to mimic VGA exactly
@@jATIWonder:           ; Set to mimic VGA exactly
@@jChipTech:
@@jGenoa:
@@jVideo7:
        jmp @@endfunc
;@@jATIWonder:                           ; From svgakt30.zip
;        mov    ax,PAGENUM
;        cli
;        and             al,0Fh                          ; Mask out bottom four bits
;        mov             ah,al
;        shl             ah,4                            ; Shift read bank into position
;        or              ah,al                           ; AH := combined read/write register numbers
;        rol             ah,1                            ; Shift banks numbers in position
;        mov             dx,1CEh                         ; Port of extended registers
;        mov             al,0B2h
;        out             dx,ax                           ; Set the ATI Bank Register
;        sti
;        jmp     @@endfunc

@@jATI:
        mov     dx,[cs:_ATIExtReg]
        mov     al,0B2h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,087h
        mov     ah,bl
        and     ah,007h
        shl     ah,3
        or      ah,al
        mov     al,0B2h
        dec     dx
        out     dx,ax
        jmp     @@endfunc

@@jTrident:
        mov     dx,3c4h                 ; Set sequence register bank address
        mov     al,0bh                  ; Set index of hardware version register
        out     dx,al                   ; Tell VGA return hardware register data
        inc     dx                      ; Point to the VGA sequence data port
        sub     al,al                   ; Set AL to null
        out     dx,al                   ; Force VGA into old definition mode
        in      al,dx                   ; Force VGA into new definition mode
        mov     al,bl                   ; Fetch current page number
	and	al,0Fh			; Force page number into range for VGA
	xor	al,02H			; Invert bit 1 for device protocol
	mov	ah,al			; Save/copy the formatted page number
        dec     dx                      ; Point back to sequence register bank
	mov	al,0eh         		; Set index of select page register
	out	dx,al			; Tell VGA Select the requested page
	inc	dx			; Point to sequence register data port
	in	al,dx			; Read previous page register data
	and	al,0f0H 		; Save the upper nibble of the register
	or	al,ah			; Combine saved bits with page number
	out	dx,al			; Tell VGA to select the requested page
        jmp     @@endfunc               ; Exit function after page set complete

@@jTseng3000:
                                        ; Map video page to screen
        and     bl,07
        mov     al,bl
        shl     al,3
        or      al,bl
        or      al,40h
        mov     dx,03CDh
        out     dx,al
        jmp     @@endfunc



@@jTseng4000:
                                        ; Map video page to screen
        and     bl,0Fh
        mov     al,bl
        shl     al,4
        or      al,bl
        mov     dx,03CDh
        out     dx,al
        jmp     @@endfunc


@@endfunc:

        pop     ds
        pop     dx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _SetPage


PROC _SetTridentStart
HIGHPAGE EQU [BYTE bp]                  ; Point high order VGA start address
        push    bp                      ; Save off register BP
        mov     bp,sp                   ; Set base pointer to point at stack
        add     bp,LARGESMALL           ; Point to the user specified data
        push    ax                      ; Save off register pair AX
        push    bx                      ; Save off register pair BX
        push    dx                      ; Save off register pair DX
        mov     dx,3c4h                 ; Set sequence register bank address
        mov     al,0bh                  ; Set index of hardware version register
        out     dx,al                   ; Tell VGA return hardware register data
        inc     dx                      ; Point to the VGA sequence data port
        sub     al,al                   ; Set AL to null
        out     dx,al                   ; Force VGA into old definition mode
        in      al,dx                   ; Force VGA into new definition mode
        mov     dx,03d4h                ; set address of CRTC register bank
        mov     al,1eh                  ; Set index of module testing register
        out     dx,al                   ; Tell VGA access module test register
        inc     dx                      ; Point to the data port
        in      al,dx                   ; Fetch off the old register data
        mov     bl,al                   ; Save off old register data
        and     bl,20h                  ; Strip off all but high start address
        mov     ah,HIGHPAGE             ; Fetch high order request start address
        shl     ah,5                    ; Align for data compare
        cmp     ah,bl                   ; Have we changed the 256k boundry
        je      @@endfunc               ; If not then no need to set a new one
        cmp     [cs:_Vcard],Trident8800 ; Skip the "or ah,80h" if 8800
        je      @@just8800              ;
        or      ah,80h                  ; Enable bit #16 of start address 8900
@@just8800:
        and     al,0dfh                 ; Strip off high order start address
        or      al,ah                   ; Add back high order start address bit
        out     dx,al                   ; Set the new high order start address
@@endfunc:
        pop     dx                      ; Restore register pair DX
        pop     bx                      ; Restore register pair BX
        pop     ax                      ; Restore register pair AX
        pop     bp
        ret                             ; Return from subroutine
ENDP    _SetTridentStart

PROC _SetTrident
        push    ax                      ; Save off register pair AX
        push    bx                      ; Save off register pair BX
        push    dx                      ; Save off register pair DX
        mov     dx,3c4h                 ; Set sequence register bank address
        mov     al,0bh                  ; Set index of hardware version register
        out     dx,al                   ; Tell VGA return hardware register data
        inc     dx                      ; Point to the VGA sequence data port
        sub     al,al                   ; Set AL to null
        out     dx,al                   ; Force VGA into old definition mode
        in      al,dx                   ; Force VGA into new definition mode
        mov     dx,03ceh                ; Set Graphics function request address
        mov     al,06h                  ; Set index of Misc. Graphics register
        out     dx,al                   ; Tell VGA the type of graphics request
        inc     dx                      ; Point to the graphics data port
        in      al,dx                   ; Fetch the old Misc. register settings
        and     al,0f0h                 ; Strip off host address selection bits
        or      al,04h                  ; Enable A0000h-AFFFFh host Addressing
        out     dx,al                   ; Tell VGA use A0000h-AFFFFh Addressing
        pop     dx                      ; Restore register pair DX
        pop     bx                      ; Restore register pair BX
        pop     ax                      ; Restore register pair AX
        ret                             ; Return from subroutine
ENDP    _SetTrident

PROC _Gwritestr
        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        pusha
        mov     ah,13h
        mov     al,0h
        mov     bh,0h
        mov     bl,[BYTE bp+4]
        mov     cx,[WORD bp+10]
        mov     dh,[BYTE bp+2]
        mov     dl,[BYTE bp]
        les     bp,[bp+6]
        int     10h
        popa
        pop     bp
        ret

ENDP _Gwritestr

PROC    _BoxFill

        SX      EQU [bp]
        SY      EQU [bp+2]
        SX1     EQU [bp+4]
        SY1     EQU [bp+6]
        COL     EQU [bp+8]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    es

        mov     ax,0A000h
        mov     es,ax

        mov     ax,SY   ;map starting point into di
        mov     dx,320
        mul     dx
        mov     di,ax
        add     di,SX

        mov     dx,SX1  ; Calculate length of box
        sub     dx,SX
        inc     dx

        mov     bx,320  ; Calculate amount to add to get to the next line
        sub     bx,dx

        mov     cx,SY1  ; Calculate width of box
        sub     cx,SY
        inc     cx

        mov     al,COL
@@drawhoriz:
        push    cx
        mov     cx,dx
        rep     stosb
        add     di,bx
        pop     cx
      loop @@drawhoriz
        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _BoxFill

PROC    _Point
        COL     EQU    [bp+4]
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    es
        push    di
        push    dx
        push    ax

        mov     ax,0A000h
        mov     es,ax

        mov     ax,SY                ; Find memory spot
        mov     dx,320
        mul     dx
        add     ax,SX
        mov     di,ax
        add     di,[_zeroaddon]
        jnc     @@nowrap                ; jump if no carry

        mov     al,[_zeropage]
        inc     al
        cmp     al,[_curpage]           ; See if already in the page
        je      @@dpt
        mov     [_curpage],al
        push    ax
        call    FAR PTR _SetPage
        pop     ax
        jmp     @@dpt
@@nowrap:
        mov     al,[_zeropage]
        cmp     al,[_curpage]           ; See if already in the page
        je      @@dpt
        mov     [_curpage],al
        push    ax
        call    FAR PTR _SetPage
        pop     ax
@@dpt:
        cmp     [_xor],0             ; xor or not?
        je      @@noxor              ; if xor is 0 overwrite color
        mov     al,[BYTE es:di]      ; xor col with current color
        xor     ax,COL
        jmp     @@dostore
@@noxor:
        mov     ax,COL

@@dostore:
        stosb

        pop     ax
        pop     dx
        pop     di
        pop     es
        pop     bp
        RET
ENDP    _Point

PROC    _GetCol
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    es
        push    di
        push    dx

        mov     ax,0A000h
        mov     es,ax

        mov     ax,SY                ; Find memory spot
        mov     dx,320
        mul     dx
        add     ax,SX
        mov     di,ax
        add     di,[_zeroaddon]
        jnc     @@nowrap                ; jump if no carry

        mov     al,[_zeropage]
        inc     al
        cmp     al,[_curpage]           ; See if already in the page
        je      @@dpt
        mov     [_curpage],al
        push    ax
        call    FAR PTR _SetPage
        pop     ax
        jmp     @@dpt
@@nowrap:
        mov     al,[_zeropage]
        cmp     al,[_curpage]           ; See if already in the page
        je      @@dpt
        mov     [_curpage],al
        push    ax
        call    FAR PTR _SetPage
        pop     ax
@@dpt:
        mov     ah,0
        mov     al,[BYTE es:di]      ; xor col with current color

        pop     dx
        pop     di
        pop     es
        pop     bp
        RET
ENDP    _GetCol

END
