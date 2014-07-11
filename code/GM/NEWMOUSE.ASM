; vgaa.asm - generic 320x200x256 color routines (SVGA replacment for gena.asm)
; Programmer:  Andy Stone    Created: 12/27/90
; Last Edited: 2/20/91

        IDEAL
        MODEL LARGE
        P286
        LARGESMALL EQU 6

        PUBLIC _MouseInterrupt
        PUBLIC _SetMouseRoutine
        PUBLIC _MyDS
        PUBLIC _Low256Point
        PUBLIC _Low256GetCol
        PUBLIC _Low256Rep1Col
        PUBLIC _Low256RepCols
        PUBLIC _Low256GetCols
        PUBLIC @NewTimer$qve
        PUBLIC _OldTimer
        PUBLIC _Clock

	EXTRN  @CallEm$qn9NodeClass:PROC
        EXTRN  @MouseMoved$qv:PROC
        EXTRN  @KeyUpdate$qi:PROC
	EXTRN  @LinkedListClass@Traverse$qnqn9NodeClass$zcn9NodeClass:PROC

        EXTRN  _Time:DATAPTR
        EXTRN  _MouseX:WORD,_MouseY:WORD,_MouseButs:WORD,_MickeyX:WORD,_MickeyY:WORD

        CODESEG
_MyDS           dw      0
_Clock          dd      0
_OldTimer       dd      0

PROC   _SetMouseRoutine
ROUTINE  EQU     [bp+LARGESMALL]
        push    bp
        mov     bp,sp
        push    es
        push    dx
        push    cx

        les     dx,ROUTINE
        mov     ax,0Ch                    ;// Put my interrupt handler in
        mov     cx,07Fh                   ;// Interrupt on any occurance
                                          ;// Set up the address of my interrupt
        int     33h
        pop     cx
        pop     dx
        pop     es
        pop     bp
        ret
ENDP   _SetMouseRoutine

PROC    @NewTimer$qve FAR
        pushf
        add     [word ptr cs:_Clock],1
        adc     [word ptr cs:_Clock+2],0
;        inc     [cs:_Clock]                  ; 386 Only!
        popf
        jmp     [dword ptr cs:_OldTimer]       ; Call the old timer fn.
ENDP    @NewTimer$qve

PROC    _Low256RepCols
X       EQU     [WORD bp+LARGESMALL]
Y       EQU     [WORD bp+2+LARGESMALL]
LEN     EQU     [WORD bp+4+LARGESMALL]
COLS    EQU     [DWORD PTR bp+6+LARGESMALL]
WIND    EQU     [DWORD PTR bp+10+LARGESMALL]

        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    dx
	push	di
	push	si
        push    ds
        push    es
                 
	lds	bx,WIND     ; ES:BX = beginning of class data

                ; Check y Boundary
      	mov	ax,Y     
        cmp     ax,[word ptr ds:bx+6]   ; MaxY
        jge     @@EndFunction        
        
                ; Check x Boundary
	mov	ax,[word ptr ds:bx+4]   ; MaxX
        mov     cx,X     ; cx = x
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.
	sub	ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:
        ; Calculating Target
	mov	ax,[word ptr ds:bx+8]   ; ax += XSkip

	mov	cx,Y                    ; y var. sent to function
	mul	cx                      ; Saves time to mul by a register

        les     di,[dword ptr ds:bx]    ; Get the start address
	add	di,ax
	add	di,X                    ; x var sent to function

        lds     si,COLS                  ; Get the Start of copy from mem

        mov     bx,LEN                  ; bx gets the length
        mov     cx,bx
        shr     cx,1                    ; divide by 4 'cause eax goes by 4 pixels
        cmp     cx,0
        je      @@skipword
        rep     movsw                   ; copy by 4s.
;@@skipdword:
;        test    bl,00000010b            ; do we need to copy at least 2 more bytes?
;        jz      @@skipword
;        movsw                           ; Yes. copy the bytes
@@skipword:
        test    bl,00000001b            ; copy 1 more byte?
        jz      @@skipbyte              
        movsb                           ; yes, so copy the byte.
@@skipbyte:
@@EndFunction:
        pop     es
        pop     ds
	pop	si
      	pop	di
        pop     dx
        pop     cx
        pop     bx
        pop     bp
        retf
ENDP    _Low256RepCols

PROC    _Low256GetCols   FAR
X       EQU     [WORD bp+LARGESMALL]
Y       EQU     [WORD bp+2+LARGESMALL]
LEN     EQU     [WORD bp+4+LARGESMALL]
COLS    EQU     [DWORD PTR bp+6+LARGESMALL]
WIND    EQU     [DWORD PTR bp+10+LARGESMALL]

        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    dx
	push	di
	push	si
        push    ds
        push    es
                 
	les	bx,WIND     ; ES:BX = beginning of class data

                ; Check y Boundary
      	mov	ax,Y     ; y var. sent to function
        cmp     ax,[word ptr es:bx+6]   ; MaxY
        jge     @@EndFunction        

                ; Check x Boundary
	mov	ax,[word ptr es:bx+4]   ; ax = MaxX
        mov     cx,X     ; cx = x
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.
	sub	ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:
        ; Calculating Target
	mov	ax,[word ptr es:bx+8]   ; ax += XSkip

	mov	cx,Y                    ; y var. sent to function
	mul	cx                      ; Saves time to mul by a register

        lds     si,[dword ptr es:bx]    ; Get the start address
	add	si,ax
	add	si,X                    ; x var sent to function

        les     di,COLS                 ; Get the Start of copy from mem

        mov     bx,LEN                  ; dx gets the length
        mov     cx,bx
        shr     cx,1                    ; divide by 4 'cause eax goes by 4 pixels
        cmp     cx,0
        je      @@skipword
        rep     movsw                   ; copy by 4s.
;@@skipdword:
;        test    bl,00000010b            ; do we need to copy at least 2 more bytes?
;        jz      @@skipword
;        movsw                           ; Yes. copy the bytes
@@skipword:
        test    bl,00000001b            ; copy 1 more byte?
        jz      @@skipbyte              
        movsb                           ; yes, so copy the byte.
@@skipbyte:
@@EndFunction:
        pop     es
        pop     ds
	pop	si
      	pop	di
        pop     dx
        pop     cx
        pop     bx
        pop     bp
	ret	
ENDP    _Low256GetCols



PROC    _Low256Rep1Col FAR
X       EQU     [WORD bp+LARGESMALL]
Y       EQU     [WORD bp+2+LARGESMALL]
LEN     EQU     [WORD bp+4+LARGESMALL]
COL     EQU     [WORD bp+6+LARGESMALL]
WIND    EQU     [DWORD PTR bp+8+LARGESMALL]
        push    bp
        mov     bp,sp
        push    bx
        push    cx
        push    dx
	push	di
	push	si
        push    ds
        push    es
                 
	lds	bx,WIND                 ; ES:BX = beginning of class data

                ; Check Y Boundary
      	mov	ax,Y                    ; y var. sent to function
        cmp     ax,[word ptr ds:bx+6]   ; MaxY
        jge     @@EndFunction        

                ; Check x Boundary
	mov	ax,[word ptr ds:bx+4]   ; ax = MaxX
        mov     cx,X                    ; cx = x
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.
	sub	ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:

        ; Calculating Target
	mov	ax,[word ptr ds:bx+8]   ; ax += XSkip

	mov	cx,Y                    ; y var. sent to function
	mul	cx                      ; Saves time to mul by a register

        les     di,[dword ptr ds:bx]    ; Get the start address
	add	di,ax
	add	di,X                    ; x var sent to function

        mov     ax,COL                  ; Get the Color var sent to function
        mov     ah,al                   ; Copy the color so it's a word

        mov     bx,LEN                  ; bx gets the length
        mov     cx,bx
        shr     cx,1                    ; divide by 4 'cause eax goes by 4 pixels
        cmp     cx,0
        je      @@skipword
        rep     stosw                   ; copy EAX in the necessary amt.
;@@skipdword:
;        test    bl,00000010b            ; do we need to copy at least 2 more bytes?
;        jz      @@skipword
;        stosw                           ; Yes. copy the bytes
@@skipword:
        test    bl,00000001b            ; copy 1 more byte?
        jz      @@skipbyte              
        stosb                           ; yes, so copy the byte.
@@skipbyte:
@@EndFunction:
        pop     es
        pop     ds
	pop	si
      	pop	di
        pop     dx
        pop     cx
        pop     bx
        pop     bp
	ret	
ENDP   _Low256Rep1Col

PROC    _MouseInterrupt
        push    ds
        mov     cx,[cs:_MyDS]
        mov     ds,cx
        mov     [ds:_MouseButs],bx
        mov     [ds:_MickeyX],si
        mov     [ds:_MickeyY],di
        test    al,1                    ; bit 0 of al set if mouse moved
        jz      @@NoMouseMovement
        push    ax                      ; save the register.
	call	far ptr @MouseMoved$qv
        pop     ax                      ; restore register.
@@NoMouseMovement:
        test    al,1111110b             ; these bits mean Mouse but. changed.
        jz      @@NoMouseButtons
        mov     ax,[ds:_MouseButs]      ; Get what happened into al
        mov     ah,2                    ; 1=keybd 2=mouse 3,4=jstick 1,2
        push    ax
	call	far ptr @KeyUpdate$qi
        add     sp,2                    ; Unpop ax
@@NoMouseButtons:
        pop     ds
	ret	
ENDP    _MouseInterrupt

PROC    _Low256Point
        COL     EQU    [bp+4]
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL         ; 6 to turn this into a far procedure
        push    es
        push    di
        push    ax
        push    dx

        mov     ax,0A000h
        mov     es,ax

        mov     ax,SY                ; Find memory spot
        mov     dx,320
        mul     dx
        mov     di,ax
        add     di,SX
;        add     di,[_zeroaddon]

        mov     ax,COL

@@dostore:
        stosb
        pop     dx
        pop     ax
        pop     di
        pop     es
        pop     bp 
        ret
ENDP    _Low256Point

PROC    _Low256GetCol

        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ds
        push    si
        push    dx

;        mov     si,[_zeroaddon]   ; Put into bx, 'cause DS is changed
        add     si,SX
         
        mov     ax,0A000h
        mov     ds,ax

        mov     ax,SY
        mov     dx,320
        mul     dx
        add     si,ax
        mov     ax,0
        lodsb

        pop     dx
        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _Low256GetCol


END
