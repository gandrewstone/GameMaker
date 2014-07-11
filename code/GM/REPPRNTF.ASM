; gena.asm   - generic 320x200x256 color routines
; Programmer:  Andy Stone    Created: 12/27/90
; Last Edited: 2/20/91

        .MODEL LARGE
        LARGESMALL EQU 6
        IDEAL
        PUBLIC _OldDOS
        PUBLIC _NewDOS
        CODESEG

LEASTX  EQU     5
LEASTY  EQU     12
MOSTX   EQU     72
MOSTY   EQU     22

        curx    db      LEASTX+1
        cury    db      MOSTY-2
        _OldDOS  dd      0

MACRO   ScrollUP
        push    ax
        push    cx
        push    di
        push    si
        push    es
        push    ds

        mov     ax,0b800h
        mov     es,ax                   
        mov     ds,ax
                
        mov     di,(LEASTY*160)+(LEASTX*2)
        mov     si,((LEASTY+1)*160)+(LEASTX*2)
        mov     cx,(MOSTY-LEASTY)
        cld
@@ScrollUPtopLoop:
        push    cx
        mov     cx,(MOSTX-LEASTX+1)
        rep     movsw
        mov     cx,160-((MOSTX-LEASTX+1)*2)     ; calc diff betw lines
        add     si,cx                           ; goto next line
        add     di,cx                           ; goto next line
        pop     cx
        loop    @@ScrollUPtopLoop
        mov     al,' '                          ; Erase the bottom line
        mov     cx,(MOSTX-LEASTX+1)
@@ScrollUpClearLast:
        stosb
        inc     di
        loop @@ScrollUpClearLast

        pop     ds
        pop     es
        pop     si
        pop     di
        pop     cx
        pop     ax
ENDM    ScrollUP



PROC    _NewDOS
        pushf
        cmp     ah,40h
        je      @@NoCall

@@CallDOS:
	popf	
	jmp	[dword ptr cs:_OldDOS]

@@NoCall:
        cmp     bx,1                    ; Is the handle stdout?
        je      @@DoWrite               ; yes
        cmp     bx,2                    ; handle stderr?
        je      @@DoWrite
        jmp     @@CallDOS

@@DoWrite:
        popf      
        mov     ax,cx                   ; returns bytes xfered in ax
        
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    si
        push    es
        mov     si,dx                   ; put initial address into si
@@TopLoop:
        mov     ax,0b800h
        mov     es,ax                   ; Calc the right location:
        mov     ah,[cs:cury]
        mov     al,160
        mul     ah
        mov     di,ax
        mov     ah,0
        mov     al,[cs:curx]
        shl     al,1
        add     di,ax

        mov     bx,0
        lodsb                           ; Need to get it into al
        cmp     [cs:curx],MOSTX
        jle     @@NotOffScreen
        mov     bx,1
@@NotOffScreen:
        cmp     al,10                   ; return sent
        jne     @@NoIncY
        inc     [cs:cury]
        cmp     [cs:cury],MOSTY         ; See if Scroll Necessary
        jg      @@ScrollUp
@@NoScrollUp:
        mov     bx,1
@@NoIncY:
        cmp     al,13                   ; New line
        jne     @@NoNewLine
        mov     [cs:curx],LEASTX
        mov     bx,1
@@NoNewLine:
        cmp     bx,0
        jne     @@NoDraw
        stosb
        inc     [cs:curx]
@@NoDraw:

        inc     dx
        loop    @@TopLoop
        jmp     @@SkipThis
@@ScrollUp:
        dec     [cs:cury]
        ScrollUP
        jmp     @@NoScrollUp

@@SkipThis:

        sti
        mov     al,20h
        out     20h,al                  ; tell 8259PIC to enable interrupts

        pop     es
        pop     si
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        add     sp,6                    ; set the pushed flags to clear
        clc
        pushf
        sub     sp,4                    ; skip back over the return address

	iret

ENDP    _NewDOS

END