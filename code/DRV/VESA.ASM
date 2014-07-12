;------------
; vesa.asm      VESA  SVGA Driver                               Andrew Stone
; Copyright 1994 G. Andrew Stone                                05/26/94
; NOT PUBLIC DOMAIN!
;------------

P386N
IDEAL
;model TINY
model LARGE
CALLSIZE      EQU     6

;PUBLIC     _SetMode
;PUBLIC     _Point
;PUBLIC     _GetCol
;PUBLIC     _Rep1Col
;PUBLIC     _RepCols
;PUBLIC     _GetCols
;PUBLIC     _Scrn


CODESEG
include "vgaregs.inc"
include "DRIVER.INC"

; CLIPX   EQU 1  ;; currently clipping must be off so the scroller can
; CLIPY   EQU 1  ;; write to other parts of the screen.
ORG        00h
Start:
retf
Header    DriverHeader <,,110,,,'VESA',5>
Scrn1 VideoMode   <320,200,320,8,0A0000000h,0,013h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,    0,0,0,0,0,0,0,OFFSET _SetStart,0,0,0>
Scrn2 VideoMode   <640,400,640,8,0A0000000h,0,100h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetStart,0,0,0>
Scrn3 VideoMode   <640,480,640,8,0A0000000h,0,101h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetStart,0,0,0>
Scrn4 VideoMode   <800,600,800,8,0A0000000h,0,103h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetStart,0,0,0>
Scrn5 VideoMode <1024,768,1024,8,0A0000000h,0,105h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols ,0,0,0,0,0,0,0,OFFSET _SetStart,0,0,0>
Scrn6 VideoMode <1280,1024,1280,8,0A0000000h,0,107h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetStart,0,0,0>

; Not done!
Scrn7 VideoMode <640,480,2048,24,0A0000000h,0,112h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,0,0,0>
Scrn8 VideoMode <800,600,0,24,0A0000000h,0,115h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,0,0,0>
Scrn9 VideoMode <1024,768,0,24,0A0000000h,0,118h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,0,0,0>

Scrn10 VideoMode <800,600,1600,16,0A0000000h,0,113h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,0,0,0>
Scrn11 VideoMode <1024,768,2048,16,0A0000000h,0,117h,OFFSET _SetVESAMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,0,0,0>

OldPage      db      0ffh

MACRO   pushall
        push    bp
        mov     bp,sp
        pushad
        push    es
        push    ds
ENDM

MACRO   popall
        pop    ds
        pop    es
        popad
        pop    bp
ENDM

MACRO   pushr registers
        IRP   reg,<registers>
          push  reg
        ENDM
ENDM

MACRO   popr registers
        IRP   reg,<registers>
          pop  reg
        ENDM
ENDM


MACRO   CalcAddress X,Y,AddrSeg,AddrOff,Seg,Off
        sub       e&AddrOff,e&AddrOff
        l&AddrSeg  AddrOff,[DWORD &Seg:&Off+VideoMode.Address]
        sub       edx,edx
        add       e&AddrOff,[DWORD &Seg:&Off+VideoMode.VPageOff]
        mov       ax,Y                ; Find memory spot
        movsx     eax,ax
        mov       dx,[WORD &Seg:&Off+VideoMode.LineSep]
        imul      eax,edx

        add       e&AddrOff,eax
        mov       dx,X
        movsx     edx,dx
        add       e&AddrOff,edx
ENDM    CalcAddress

; ax = pagenum, dx destroyed
MACRO    SetWPage
        LOCAL   Skip
        cmp     al,[BYTE cs:OldPage]
        je      Skip
        mov     [cs:OldPage],al
        push    bx
        push    dx
        xor     ah,ah
        xor     bh,bh
        mov     dx,ax
        mov     ax,4f05h
        int     10h
        pop     dx
        pop     bx
Skip:
ENDM    SetWPage


PROC    SetReadPage FAR
        jmp _SetWritePage
ENDP
PROC    _SetWritePage FAR
PAGENUM EQU [BP+CALLSIZE]
        push    bp
        mov     bp,sp
        push    ax
        push    dx

        mov     ax,PAGENUM
        SetWPage
        pop     dx
        pop     ax
        pop     bp
        retf
ENDP    _SetWritePage

PROC     _SetStart      FAR
        NEWPOS     EQU [bp+CALLSIZE]
        VidDrv     EQU [bp+CALLSIZE+4]
        push       bp
        mov        bp,sp
        push       ds
        lds        bx,VidDrv             ; Load the data structure
        xor        ecx,ecx
        mov        eax,NEWPOS
        mov        cx,[ds:bx+VideoMode.LineSep]
        mov        edx,0
        mov        [ds:bx+VideoMode.VPageOff],eax
        div        ecx
        ; now x is in edx, y is in eax.
        ; Do VESA CALL.
        ;
        mov        cx,dx        ; Load the x
        mov        bx,ax

        mov        dx,03dah     ; Wait for V-retrace
@@WaitForRetrace:
        in         al,dx
        test       al,8
        jz         @@WaitForRetrace

        mov        dx,bx        ; Load the y
        mov        ax,04f07h    ; VESA Set start addr fn.
        mov        bx,0000h
        int        10h
        pop        ds
        pop        bp
        ret
ENDP    _SetStart

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


PROC    _SetVESAMode   FAR
        VidDrv  EQU    [DWORD bp+CALLSIZE]

        push       bp
        mov        bp,sp
        pushr      <ax,bx,ds>
        lds        bx,VidDrv
        mov        bx,[ds:bx+VideoMode.BIOSMode]
        mov        ax,4F02h
        int        10h
        popr       <ds,bx,ax,bp>
        retf
ENDP    _SetVESAMode

PROC    _SetMode   FAR
        VidDrv  EQU    [DWORD bp+CALLSIZE]

        push       bp
        mov        bp,sp
        pushr      <ax,bx,ds>
        lds        bx,VidDrv
        mov        ax,[ds:bx+VideoMode.BIOSMode]
        int        10h

        popr       <ds,bx,ax,bp>
        retf
ENDP    _SetMode



PROC    _Point
        VidDrv  EQU    [DWORD bp+6+CALLSIZE]
        COL     EQU    [WORD bp+4+CALLSIZE]
        SY      EQU    [WORD bp+2+CALLSIZE]
        SX      EQU    [WORD bp+CALLSIZE]

        push    bp
        mov     bp,sp
        pushr   <es ds edi eax ebx edx>

        lds     bx,VidDrv             ; Load the data structure
        CalcAddress <SX>,<SY>,es,di,ds,bx
        mov     eax,edi
        shr     eax,16
        SetWPage
        mov     ax,COL
        mov     [BYTE es:di],al

        popr    <edx ebx eax edi ds es bp>
        retf
ENDP    _Point

PROC    _GetCol      FAR
        VidDrv  EQU    [DWORD bp+4+CALLSIZE]
        SY      EQU    [WORD  bp+2+CALLSIZE]
        SX      EQU    [WORD  bp+CALLSIZE]

        push     bp
        mov      bp,sp
        pushr    <ebx,edx,edi,ds,es>

        lds     bx,VidDrv             ; Load the data structure
        CalcAddress <SX>,<SY>,es,di,ds,bx
        mov     eax,edi
        shr     eax,16
        SetWPage

        mov     ah,0
        mov     al,[BYTE es:di]
        popr    <es ds edi edx ebx bp>
        retf
ENDP    _GetCol

PROC    _Rep1Col FAR
X       EQU     [WORD bp+CALLSIZE]
Y       EQU     [WORD bp+2+CALLSIZE]
LEN     EQU     [WORD bp+4+CALLSIZE]
COL     EQU     [WORD bp+6+CALLSIZE]
VidDrv  EQU     [DWORD PTR bp+8+CALLSIZE]

        pushall
                
        lds     bx,VidDrv                 ; ES:BX = beginning of class data

IFDEF CLIPY     ; Check Y Boundary
        mov     ax,Y                    ; y var. sent to function
        cmp     ax,[word ptr ds:bx+VideoMode.Width]   ; MaxY
        jge     @@EndFunction        
ENDIF
IFDEF CLIPX     ; Check x Boundary
        mov     ax,[word ptr ds:bx+VideoMode.Length]  ; ax = MaxX
        mov     cx,X                    ; cx = x
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.

        sub     ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:
ENDIF
        ; Calculating Target

        CalcAddress <SX>,<SY>,es,di,ds,bx
        mov     eax,edi
        shr     eax,16
        SetWPage

        ; Check to see if it goes across a page boundary here.
        mov     ebx,0
        mov     edx,0
        mov     bx,LEN            ; bx gets the length
        mov     dx,di             ; see if len goes across a page.
        add     edx,ebx
        cmp     edx,0FFFFh        ; Does the draw go betw. pages?
        jle     @@NoPageSwitch
        and     edx,0FFFFh
        sub     bx,dx             ; Now bx is the amount to put in this page.
        jmp     @@DrawIt
@@NoPageSwitch:
        mov     edx,0

@@DrawIt:

@@DrawLoop:
        mov     ax,COL                  ; Get the Color var sent to function
        mov     ah,al                   ; Copy the color so it's a word
        push    ax
        push    ax
        pop     eax

        mov     cx,bx
        shr     cx,2                    ; divide by 4 'cause eax goes by 4 pixels
        cmp     cx,0
        je      @@skipdword
        rep     stos [DWORD es:di]      ; copy EAX in the necessary amt.
@@skipdword:
        test    bl,00000010b            ; do we need to copy at least 2 more bytes?
        jz      @@skipword
        stosw                           ; Yes. copy the bytes
@@skipword:
        test    bl,00000001b            ; copy 1 more byte?
        jz      @@skipbyte              
        stosb                           ; yes, so copy the byte.
@@skipbyte:
        cmp     dx,0
        je      @@EndFunction           ; Don't need to switch pages.
        add     edi,10000h              ; Make edi point to the next page.
        mov     eax,edi
        shr     eax,16
        mov     bx,dx
        SetWPage
        mov     dx,0                    ; This will only allow 1 page boundary.
;       mov     edi,0 implicitly done   ; OK for now, but l8r may change.
        jmp     @@DrawLoop

@@EndFunction:
        popall
        retf
ENDP    _Rep1Col


PROC    _RepCols  FAR
X       EQU     [WORD bp+CALLSIZE]
Y       EQU     [WORD bp+2+CALLSIZE]
LEN     EQU     [WORD bp+4+CALLSIZE]
COLS    EQU     [DWORD PTR bp+6+CALLSIZE]
VidDrv  EQU     [DWORD PTR bp+10+CALLSIZE]

        pushall

        lds     bx,VidDrv               ; ES:BX = beginning of class data

IFDEF CLIPY     ; Check y Boundary
        mov     ax,Y     
        cmp     ax,[word ptr ds:bx+VideoMode.Width]
        jge     @@EndFunction        
ENDIF
IFDEF CLIPX     ; Check x Boundary
        mov     ax,[word ptr ds:bx+VideoMode.Length]
        mov     cx,X
        cmp     cx,ax
        jge     @@EndFunction           ; starting x is greater the Maximum x, so quit.

        sub     ax,cx                   ; ax = MaxX-x
        cmp     ax,LEN                  ; if (MaxX-x>=len) don't truncate
        jge     @@NoTruncate
        mov     LEN,ax                  ; Truncate 
@@NoTruncate:
ENDIF   ; Calculating Target
        CalcAddress <SX>,<SY>,es,di,ds,bx
        mov     eax,edi
        shr     eax,16
        SetWPage

        ; Check to see if it goes over a page boundary.
        mov     ebx,0
        mov     edx,0
        mov     bx,LEN            ; bx gets the length
        mov     dx,di             ; see if len goes across a page.
        add     edx,ebx
        cmp     edx,0FFFFh        ; Does the draw go betw. pages?
        jle     @@NoPageSwitch
        and     edx,0FFFFh
        sub     bx,dx             ; Now bx is the amount to put in this page.
        jmp     @@DrawIt
@@NoPageSwitch:
        mov     edx,0

@@DrawIt:
        lds     si,COLS                 ; Get the Start of copy from mem

@@DrawLoop:                             ; Loop for each page.
        mov     cx,bx
        shr     cx,2                    ; divide by 4 'cause eax goes by 4 pixels
        cmp     cx,0
        je      @@skipdword
        rep     movs [DWORD es:di],[DWORD ds:si]  ; copy by 4s.
@@skipdword:
        test    bl,00000010b            ; do we need to copy at least 2 more bytes?
        jz      @@skipword
        movsw                           ; Yes. copy the bytes
@@skipword:
        test    bl,00000001b            ; copy 1 more byte?
        jz      @@skipbyte              
        movsb                           ; yes, so copy the byte.
@@skipbyte:
        cmp     dx,0                    
        je      @@EndFunction           ; Don't need to switch pages.
        add     edi,10000h              ; Make edi point to the next page.
        mov     bx,dx
        mov     eax,edi
        shr     eax,16
        SetWPage
        mov     dx,0                    ; This will only allow 1 page boundary.
                                        ; OK for now, but l8r may change.
        jmp     @@DrawLoop

@@EndFunction:
        popall
        retf
ENDP    _RepCols

PROC    _GetCols   FAR
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
        CalcAddress <SX>,<SY>,ds,si,es,bx
        mov     eax,esi
        shr     eax,16
        SetWPage

        ; Check to see if it goes over a page boundary.
        mov     ebx,0
        mov     edx,0
        mov     bx,LEN            ; bx gets the length
        mov     dx,si             ; see if len goes across a page.
        add     edx,ebx
        cmp     edx,0FFFFh        ; Does the draw go betw. pages?
        jle     @@NoPageSwitch
        and     edx,0FFFFh
        sub     bx,dx             ; Now bx is the amount to put in this page.
        jmp     @@DrawIt
@@NoPageSwitch:
        mov     edx,0

@@DrawIt:
        les     di,COLS                 ; Get the Start of copy from mem

@@PageLoop:
        mov     cx,bx
        shr     cx,2                    ; divide by 4 'cause eax goes by 4 pixels
        cmp     cx,0
        je      @@skipdword
        rep     movs [DWORD es:di],[DWORD ds:si]  ; copy by 4s. Explicitly written so compiler does not use esi,edi.
@@skipdword:
        test    bl,00000010b              ; Do we need to copy at least 2 more bytes?
        jz      @@skipword
        movs    [WORD es:di],[WORD ds:si] ; Yes. copy the bytes
@@skipword:
        test    bl,00000001b              ; copy 1 more byte?
        jz      @@skipbyte              
        movs    [BYTE es:di],[BYTE ds:si] ; yes, so copy the byte.
@@skipbyte:
        cmp     dx,0
        je      @@EndFunction           ; Don't need to switch pages.
        add     esi,10000h              ; Make edi point to the next page.
        mov     bx,dx
        mov     eax,esi
        shr     eax,16
        SetWPage
        mov     dx,0                    ; This will only be OK for across 1 page.
        jmp     @@PageLoop

@@EndFunction:
        popall
        retf
ENDP    _GetCols

END     Start

