;------------
; vga.asm       Standard VGA Driver                             Andy Stone
;                                                               05/26/94
; compile: TASM  /m2  vga      ; /m2 means 2 passes (resolve fwd refs).
;          TLINK /t/3 vga
;------------

;P386N
P286N
IDEAL
model LARGE

CALLSIZE      EQU     6

PUBLIC        _VGAScrn
PUBLIC        _MemScrn

include "d:\drv\DRIVER.INC"

DATASEG
Header    DriverHeader <,,,,,"Standard VGA",1>
_VGAScrn   VideoMode  <320,200,320,8,0A0000000h,0,013h,SetMode,ClearL256,PointL256,GetColL256,Rep1ColL256,0,RepColsL256,0,GetColsL256,0,0,0,0,0,0,Dummy,Dummy,Dummy,_SetAllPal,_GetAllPal,_SetAllPalTo,_Palette>
_MemScrn   VideoMode  <320,200,320,8,0A0000000h,0,013h,SetMode,ClearL256,PointL256,GetColL256,Rep1ColL256,0,RepColsL256,0,GetColsL256,0,0,0,0,0,0,Dummy,Dummy,Dummy,_SetAllPal,_GetAllPal,_SetAllPalTo,_Palette>

CODESEG


;ORG        00h
Start:
PROC  Dummy FAR
retf
ENDP  Dummy
include "d:\drv\DRVMACRO.INC"

MACRO   CalcAddress X,Y,AddrSeg,AddrOff,Seg,Off
        l&AddrSeg  AddrOff,[DWORD &Seg:&Off+VideoMode.Address]
        mov       ax,Y                ; Find memory spot
        mov       dx,[WORD &Seg:&Off+VideoMode.LineSep]
        mul       dx
        add       AddrOff,ax
        add       AddrOff,X
ENDM    CalcAddress

PROC    ClearL256
        COL     EQU    [WORD bp+CALLSIZE]
        push    bp
        mov     bp,sp
        pushr   <ax cx di es>

        mov     ax,0A000h
        mov     es,ax

        mov     di,0
        mov     ax,COL
        mov     cx,08FFFh
        mov     ah,al
        rep     stosw
        popr    <es di cx ax bp>
        retf
ENDP    ClearL256

CRTCINDEX    EQU    03D4h
CRTCADDRESS  EQU    03D5h

PROC    SetMode   FAR
        VidDrv  EQU    [DWORD bp+CALLSIZE]

        push       bp
        mov        bp,sp
        pushr      <ax,bx,ds>
        lds        bx,VidDrv
        mov        ax,[ds:bx+VideoMode.BIOSMode]
        int        10h

        popr       <ds,bx,ax,bp>
        retf
ENDP    SetMode

PROC    PointL256
        VidDrv  EQU    [DWORD bp+6+CALLSIZE]
        COL     EQU    [bp+4+CALLSIZE]
        SY      EQU    [bp+2+CALLSIZE]
        SX      EQU    [bp+CALLSIZE]

        push    bp
        mov     bp,sp
        pushr   <es ds di ax bx dx>

        lds     bx,VidDrv             ; Load the data structure
        CalcAddress <SX>,<SY>,es,di,ds,bx

        mov     ax,COL
        mov     [BYTE es:di],al

        popr    <dx bx ax di ds es bp>
        retf
ENDP    PointL256

PROC    GetColL256      FAR
        VidDrv  EQU    [DWORD bp+4+CALLSIZE]
        SY      EQU    [WORD  bp+2+CALLSIZE]
        SX      EQU    [WORD  bp+CALLSIZE]

        push     bp
        mov      bp,sp
        pushr    <bx,dx,di,ds,es>

        lds     bx,VidDrv             ; Load the data structure
        CalcAddress <SX>,<SY>,es,di,ds,bx

        mov     ah,0
        mov     al,[BYTE es:di]

        popr    <es ds di dx bx bp>
        retf
ENDP    GetColL256

PROC    Rep1ColL256 FAR
X       EQU     [WORD bp+CALLSIZE]
Y       EQU     [WORD bp+2+CALLSIZE]
LEN     EQU     [WORD bp+4+CALLSIZE]
COL     EQU     [WORD bp+6+CALLSIZE]
VidDrv  EQU     [DWORD PTR bp+8+CALLSIZE]

        pushall
                
        lds     bx,VidDrv                 ; ES:BX = beginning of class data

                ; Check Y Boundary
        mov     ax,Y                    ; y var. sent to function
        cmp     ax,[word ptr ds:bx+VideoMode.Width]   ; MaxY
        jge     @@EndFunction        

                ; Check x Boundary
        mov     ax,[word ptr ds:bx+VideoMode.Length]  ; ax = MaxX
        mov     cx,X                    ; cx = x
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.

        sub     ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:

        ; Calculating Target
        mov     ax,[word ptr ds:bx+VideoMode.LineSep]   ; ax += LineSeperation

        mov     cx,Y                    ; y var. sent to function
        mul     cx                      ; Saves time to mul by a register

        les     di,[dword ptr ds:bx+VideoMode.Address]  ; Get the start address
        add     di,ax
        add     di,X                    ; x var sent to function

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
        popall
        retf
ENDP    Rep1ColL256


PROC    RepColsL256  FAR
X       EQU     [WORD bp+CALLSIZE]
Y       EQU     [WORD bp+2+CALLSIZE]
LEN     EQU     [WORD bp+4+CALLSIZE]
COLS    EQU     [DWORD PTR bp+6+CALLSIZE]
VidDrv  EQU     [DWORD PTR bp+10+CALLSIZE]

        pushall

        lds     bx,VidDrv               ; ES:BX = beginning of class data

                ; Check y Boundary
        mov     ax,Y     
        cmp     ax,[word ptr ds:bx+VideoMode.Width]
        jge     @@EndFunction        
        
                ; Check x Boundary
        mov     ax,[word ptr ds:bx+VideoMode.Length]
        mov     cx,X
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.

        sub     ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:
        ; Calculating Target
        mov     ax,[word ptr ds:bx+VideoMode.LineSep]   ; ax = XSkip
        mov     cx,Y                    ; y var. sent to function
        mul     cx                      ; Saves time to mul by a register

        les     di,[dword ptr ds:bx+VideoMode.Address]    ; Get the start address
        add     di,ax
        add     di,X                    ; x var sent to function

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
        popall
        retf
ENDP    RepColsL256

PROC    GetColsL256   FAR
X       EQU     [WORD bp+CALLSIZE]
Y       EQU     [WORD bp+2+CALLSIZE]
LEN     EQU     [WORD bp+4+CALLSIZE]
COLS    EQU     [DWORD PTR bp+6+CALLSIZE]
VidDrv  EQU     [DWORD PTR bp+10+CALLSIZE]

        pushall

        les     bx,VidDrv    ; ES:BX = beginning of class data

                ; Check y Boundary
        mov     ax,Y     ; y var. sent to function
        cmp     ax,[word ptr es:bx+VideoMode.Width]
        jge     @@EndFunction        

                ; Check x Boundary
        mov     ax,[word ptr es:bx+VideoMode.Length]
        mov     cx,X     ; cx = x
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.
        sub     ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:
        ; Calculating Target
        mov     ax,[word ptr es:bx+VideoMode.LineSep]   ; ax += XSkip
        mov     cx,Y                    ; y var. sent to function
        mul     cx                      ; Saves time to mul by a register

        lds     si,[dword ptr es:bx+VideoMode.Address]    ; Get the start address
        add     si,ax
        add     si,X                    ; x var sent to function

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
        popall
        retf
ENDP    GetColsL256

PROC    _BoxFill

        SX      EQU [bp+CALLSIZE]
        SY      EQU [bp+2+CALLSIZE]
        SX1     EQU [bp+4+CALLSIZE]
        SY1     EQU [bp+6+CALLSIZE]
        COL     EQU [bp+8+CALLSIZE]

        push    bp
        mov     bp,sp
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

RETRACE_PORT   EQU      03DAh
VERT           EQU      8
HORIZ          EQU      1
ON             =       1
OFF            =       0

MACRO   WaitRetrace HorizVert,OnOff, SaveR
LOCAL   Top
        IFB     <SaveR>
        push    ax
        push    dx
        ELSEIF  SaveR EQ SAVEREGS
        push    ax
        push    dx
        ENDIF
Top:
        mov     dx,RETRACE_PORT
        in      al,dx
        test    al,HorizVert
        IF      OnOff EQ ON
        jnz     Top
        ELSE
        jz      Top
        ENDIF
        IFB     <SaveR>
        pop     dx
        pop     ax
        ELSEIF  SaveR EQ SAVEREGS
        pop     dx
        pop     ax
        ENDIF
ENDM


MACRO   PUSHCKILLS
        push      ax
        push      bx
        push      cx
        push      dx
ENDM

MACRO   POPCKILLS
        pop       dx
        pop       cx
        pop       bx
        pop       ax
ENDM

PROC    _SetAllPalTo FAR
        PALADDR   EQU [bp+CALLSIZE]
        push    bp
        mov     bp,sp
        push    ds
        push    si

        lds     si,PALADDR

        mov     al,0
        mov     dx,03c8h

        mov     bx,256

        out     dx,al
        inc     dx

@@top:
        mov     cx,3
        rep     outsb
        sub     si,3
        dec     bx
        cmp     bx,0
        jg      @@top

        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _SetAllPalTo


PROC    _SetAllPal FAR
        PALADDR   EQU [bp+CALLSIZE]
        push    bp
        mov     bp,sp
        push    ds
        push    si

        lds     si,PALADDR

        mov     al,0
        mov     dx,03c8h

        out     dx,al
        inc     dx
        mov     cx,768
        rep     outsb

        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _SetAllPal

PROC    _GetAllPal FAR
        PALADDR   EQU [bp+CALLSIZE]
        push    bp
        mov     bp,sp
        push    es
        push    di

        les     di,PALADDR

        mov     al,0
        mov     dx,03c8h

        out     dx,al
        inc     dx
        mov     cx,768
        rep     insb

        pop     di
        pop     es
        pop     bp
        ret
ENDP    _GetAllPal

PROC    _Palette

        push    bp
        mov     bp,sp

        mov     dx,03C8h           ; VGA color number port
        mov     al,[bp+CALLSIZE] ; color number
        out     dx,al

        inc     dx                 ; RBG color additive port

        mov     al,[bp+2+CALLSIZE]          ; user's red
        out     dx,al
        mov     al,[bp+3+CALLSIZE]          ; user's green
        out     dx,al
        mov     al,[bp+4+CALLSIZE]          ; user's blue
        out     dx,al

        pop     bp
        ret
ENDP    _Palette

END     Start

