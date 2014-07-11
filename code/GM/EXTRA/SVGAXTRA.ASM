        .MODEL LARGE
        IDEAL
        LARGESMALL EQU 6
        EXTRN  _zeroaddon:WORD
        EXTRN  _xor:WORD
        EXTRN  _curpage:BYTE
        EXTRN  _zeropage:BYTE
        EXTRN  _ScratchSeg:WORD
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



;        PUBLIC _FourPoint
;        PUBLIC _GetCol
;        PUBLIC _BufToScrn
;        PUBLIC _SVGAdrawblk
;        PUBLIC _SVGAgetblk

IFDEF THESE_FUNCTIONS_NOT_USED
PROC    _SVGAgetblk
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    dx
        push    di
        push    si
        push    es
        push    ds

        mov     dl,[_curpage]        ; Fetch current page num prior to DS remap
        mov     dh,[_zeropage]       ; Fetch last page num prior to DS remap
        mov     bx,[_zeroaddon]

        and     dl,0Fh
        and     dh,0Fh

        mov     ax,0A000h
        mov     ds,ax
        les     di,[bp+4]

        push    dx
        mov     ax,SY                ;   Find memory address of (x,y) screen
        mov     dx,320               ; location.
        mul     dx
        pop     dx
        add     ax,SX
        mov     si,ax
        add     si,bx                ; [_zeroaddon]
        jc      @@wrapdone

        cmp     si,59100             ; is si < 65535-(320*BLEN)
        jb      @@neverwrap

        jmp     @@wrapoccur

@@neverwrap:                         ; Si will never wrap
        cmp     dl,dh                ; is the current page the zero page?
        je      @@doall              ; Yes - so draw block
        mov     dl,dh
        push    dx
        call    FAR PTR _SetPage
        pop     dx
        jmp     @@doall

@@wrapdone:
        mov     al,dl
        dec     al
        and     al,0fh

        cmp     al,dh                ; See if already in 2nd page
        je      @@doall              ; Yes
        mov     dl,dh
        inc     dl                   ; No, so make it the next page.
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@doall:
        mov     bx,20

@@tplp:                         ; Since we're already in the 2nd scrn
        mov     cx,10           ; we need not worry about crossing a boundary
        rep     movsw           ; so we can just draw it without checking
        add     si,300
        dec     bx
        cmp     bx,0
        jg      @@tplp
        jmp     @@endfunc
;-------------------------

@@wrapoccur:                       ; Only need to set page once
        cmp     dl,dh           ; See if already in the page
        je      @@dpt
        mov     dl,dh           ; Mov  [_curpage], [_zeropage]
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@dpt:
        mov     bx,20
@@tplp1:
        cmp     si,65515   ; Will the new page need to be set in this line?
        jbe     @@dorep    ; No
        mov     cx,20      ; Yes
@@innerloop:
        movsb
        dec     cx

        cmp     si,0
        jne     @@nonextscrn            ; if equal add one to page

        mov     dl,dh
        inc     dl
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@nonextscrn:
        cmp     cx,0
        jg      @@innerloop
        add     si,300
        dec     bx
        cmp     bx,0            ; Done drawing box?
        jg      @@tplp1         ; No
        jmp     @@endfunc       ; Yes

@@dorep:                        ; Draw one line on screen
        mov     cx,10
        rep     movsw
        add     si,300
        jnc     @@cont          ; if wraparound (carry) add one to the page #

        mov     dl,dh
        inc     dl              ; Add one to current page
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@cont:
        dec     bx
        cmp     bx,0
        jg      @@tplp1

@@endfunc:

        pop     ds
        mov     [_curpage],dl           ; Restore the current page variable
        pop     es
        pop     si
        pop     di
        pop     dx
        pop     ax
        pop     bp
        RET
ENDP    _SVGAgetblk

PROC    _SVGAdrawblk
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    dx
        push    di
        push    si
        push    es
        push    ds

        mov     dl,[_curpage]        ; Fetch current page num prior to DS remap
        mov     dh,[_zeropage]       ; Fetch last page num prior to DS remap
        mov     bx,[_zeroaddon]

        and     dl,0Fh
        and     dh,0Fh


        mov     ax,0A000h
        mov     es,ax
        lds     si,[bp+4]       ; Load DS:SI with block picture address

        push    dx
        mov     ax,SY                ; Find memory address of x,y on scrn
        mov     dx,320
        mul     dx
        pop     dx

        add     ax,SX
        mov     di,ax                ; di (ax) can't have wrapped 'cause 320x200=64000

        add     di,bx                ; [_zeroaddon] Memory bias of scrolled map
                                     ; (screen is scrolled, so you need
                                     ; to add something because (0,0) is
                                     ; not 0 in memory)
        jc      @@wrapdone

        cmp     di,59100             ; is si < 65535-(320*BLEN)
        jb      @@neverwrap

        jmp     @@nowrap


@@neverwrap:                         ; Si will never wrap
        cmp     dl,dh                ; is the current page the zero page?
        je      @@doall              ; Yes - so draw block
        mov     dl,dh
        push    dx
        call    FAR PTR _SetPage
        pop     dx
        jmp     @@doall

@@wrapdone:                     ; Set curpage to zeropage+1 because si wrapped around
        mov     al,dl
        dec     al
        and     al,0fh

        cmp     al,dh                ; See if already in the page
        je      @@doall
        mov     dl,dh
        inc     dl
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@doall:

        mov     bx,20
@@tplp:                         ; Since we're already in the 2nd scrn
        mov     cx,10           ; we need not worry about crossing a boundary
        rep     movsw           ; so we can just draw it without checking
        add     di,300
        dec     bx
        cmp     bx,0
        jg      @@tplp
        jmp     @@endfunc
;-------------------------

@@nowrap:
        cmp     dh,dl           ; See if already in the page
        je      @@dpt
        mov     dl,dh
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@dpt:
        mov     bx,20
@@tplp1:

        cmp     di,65515        ; Will page change happen on this draw?
        jbe     @@dorep         ; No.
        mov     cx,20           ; Yes.
@@innerloop:
        dec     cx
        movsb

        cmp     di,0            ;
        jne     @@nonextscrn    ; if equal add one to page
        mov     dl,dh           ; Fetch the last page number (_zeropage)
        inc     dl
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@nonextscrn:
        cmp     cx,0
        jg      @@innerloop
        add     di,300
        dec     bx
        cmp     bx,0
        jg      @@tplp1
        jmp     @@endfunc
@@dorep:
        mov     cx,10
        rep     movsw

        add     di,300
        jnc     @@cont

        mov     dl,dh
        inc     dl
        push    dx
        call    FAR PTR _SetPage
        pop     dx
@@cont:
        dec     bx
        cmp     bx,0
        jg      @@tplp1

@@endfunc:

        pop     ds
        mov     [_curpage],dl           ; Restore the current page variable
        pop     es
        pop     si
        pop     di
        pop     dx
        pop     ax
        pop     bp
        RET
ENDP    _SVGAdrawblk

PROC    _GetCol

        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ds
        push    si
        push    dx

        mov     bh,[_zeropage]
        mov     bl,[_curpage]

        mov     ax,SY
        mov     dx,320
        mul     dx
        add     ax,SX
        mov     si,ax
        add     si,[_zeroaddon]
        jnc     @@nowrap

        mov     al,bh              ; Move to next page
        inc     al
        cmp     al,bl              ; See if already in the page
        je      @@gpt
        mov     bl,al
        push    bx
        call    FAR PTR _SetPage
        pop     bx
        jmp     @@gpt

@@nowrap:
        cmp     bh,bl              ; see if already in the page
        je      @@gpt              ; skip setting if already correct
        mov     bl,bh
        push    bx
        call    FAR PTR _SetPage
        pop     bx
@@gpt:
        mov     ax,0A000h
        mov     ds,ax

        mov     ax,0
        lodsb

        mov     [_curpage],bl     ; Move the curpage from bl to _curpage

        pop     dx
        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _GetCol

PROC    _FourPoint
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    bx
        push    dx
        push    di
        push    si
        push    es
        push    ds

        mov     bh,[_zeropage]
        mov     bl,[_curpage]
        mov     cx,[_zeroaddon]

        lds     si,[bp+4]       ; Load DS:SI with block picture address
        mov     ax,0A000h       ; Load ES with screen memory address
        mov     es,ax

        mov     ax,SY           ;  Find address of x,y on screen
        mov     dx,320          ; and put it into DI
        mul     dx
        add     ax,SX
        mov     di,ax
        add     di,cx
        jnc     @@nowrap        ; If DI is still in the zeropage then don't change

                                ; change the page to zeropage +1 if needed
        mov     al,bl           ; Is curpage 1 > than zeropage?
        dec     al
        cmp     bh,al           ; [_zero],[_cur] See if already in the page
        je      @@dpt
        mov     bl,bh
        inc     bl              ; make curpage 1 > zeropage
        push    bx
        call    FAR PTR _SetPage
        pop     bx
        jmp     @@dpt
@@nowrap:
        cmp     bh,bl          ; See if already in the page
        je      @@dpt
        mov     bl,bh
        push    bx
        call    FAR PTR _SetPage
        pop     bx

@@dpt:
        mov     cx,2
        rep     movsw

        pop     ds
        mov     [_curpage],bl
        pop     es
        pop     si
        pop     di
        pop     dx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _FourPoint
ENDIF

