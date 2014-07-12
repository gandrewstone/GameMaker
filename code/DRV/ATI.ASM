;------------
; ati.asm      ati wonder  VGA Driver                         Andy Stone
;                                                               05/26/94
; compile: TASM  /m2  vga      ; /m2 means 2 passes (resolve fwd refs).
;          TLINK /t/3 vga
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
include "VGAREGS.INC"
include "DRIVER.INC"
; CLIPX   EQU 1  ;; currently clipping must be off so the scroller can
; CLIPY   EQU 1  ;; write to other parts of the screen.

ORG        00h
Start:
retf
Header    DriverHeader <,,110,,,'ATI VGA Wonder',4>
Scrn1 VideoMode   <320,200,320,8,0A0000000h,0,013h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetVGAStart>
Scrn2 VideoMode   <640,400,640,8,0A0000000h,0,061h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetSVGAStart>
Scrn3 VideoMode   <640,480,640,8,0A0000000h,0,062h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetSVGAStart>
Scrn4 VideoMode   <800,600,800,8,0A0000000h,0,063h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetSVGAStart>
Scrn5 VideoMode  <1024,768,1024,8,0A0000000h,0,064h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetSVGAStart>

; Not done!
Scrn6 VideoMode   <640,480,640,16,0A0000000h,0,072h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetSVGAStart>
Scrn7 VideoMode   <640,480,2048,24,0A0000000h,0,075h,OFFSET _SetMode,0,OFFSET _Point,OFFSET _GetCol,OFFSET _Rep1Col,0,OFFSET _RepCols,0,OFFSET _GetCols,0,0,0,0,0,0,0,OFFSET _SetSVGAStart>


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
        and     al,0Fh                          ; Mask out bottom four bits
        mov     [cs:OldPage],al
        mov     ah,al
        shl     ah,4                            ; Shift read bank into position
        or      ah,al                           ; AH := combined read/write register numbers
        rol     ah,1                            ; Shift banks numbers in position
        mov     dx,1CEh                         ; Port of extended registers
        mov     al,0B2h
        out     dx,ax                           ; Set the ATI Bank Register
Skip:
ENDM    SetWPage

MACRO   SetSVGAScrnAddr
        and        cl,3            ; TSENG ET4000 set start address code.
        mov        dx,03d4h
        mov        al,033h
        mov        ah,cl
        out        dx,ax
ENDM


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
ENDIF
        ; Calculating Target
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

ALIGN 2
VGASt   dw       0FFFFh
SVgaSt  db       0FFh
BankSt  db       0FFh
PROC    _SetSVGAStart      FAR
        NEWPOS     EQU [bp+CALLSIZE]
        VidDrv     EQU [bp+CALLSIZE+4]
        push       bp
        mov        bp,sp
        push       di
        push       ds

        lds        di,VidDrv
        mov        ecx,NEWPOS
        mov        ax,cx
        and        ecx,0FFFFFFFCh  ; Mask out the page shift.
        mov        [ds:di+VideoMode.VPageOff],ecx

        mov        di,ax
@@WaitOutRetrace:
        mov        dx,03DAh
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace

        mov        dx,CRTCBASEREG
        shr        ecx,3                ; Remove bank info
        mov        bx,[cs:VGASt]
       
        cmp        cl,bl
        je         @@SkipSetVGALow
        mov        al,0Dh               ; Set Normal VGA low byte.
        mov        ah,cl
        mov        [cs:VGASt],cx
        out        dx,ax

@@SkipSetVGALow:

        cmp        ch,bh
        je         @@SkipSetVGAHigh
        mov        al,0Ch               ; Set Normal VGA high byte.
        mov        ah,ch
        mov        [cs:VGASt],cx
        out        dx,ax
@@SkipSetVGAHigh:

        shr        ecx,16
        mov        bl,[cs:SVgaSt]
        cmp        bl,cl
        je         @@SkipSetSVGA

        mov        [cs:SVgaSt],cl
        SetSVGAScrnAddr
@@SkipSetSVGA:
        mov        dx,03dah
@@WaitOutRetrace1:
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace1
@@WaitForRetrace:
        in         al,dx
        test       al,8
        jz         @@WaitForRetrace
        and        di,7
        mov        bx,di
        cmp        bl,[cs:BankSt]
        je         @@SkipBankSet
        mov        [cs:BankSt],bl
        ; an in    al,03da is required b4 setting the bank shift to
        ; reset a flip-flop.
        ; Set which bank (0-3) should come first.
        mov        dx,03C0h
        mov        al,(13h+20h)
        out        dx,al        ; Output index w/out destroying bit 20h
        mov        al,bl
        out        dx,al
@@SkipBankSet:

        pop  ds
        pop  di
        pop  bp
        ret
ENDP    _SetSVGAStart

PROC     _SetVGAStart      FAR
        NEWPOS     EQU [bp+CALLSIZE]
        push       bp
        mov        bp,sp
        push       di
        push       ds

        lds        di,VidDrv
        mov        ecx,NEWPOS
        mov        ax,cx
        and        ecx,0FFFFFFFCh  ; Mask out the page shift.
        mov        [ds:di+VideoMode.VPageOff],ecx

        mov        di,ax
@@WaitOutRetrace:
        mov        dx,03DAh
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace

        mov        dx,CRTCBASEREG
        shr        ecx,2                ; Remove bank info
        mov        bx,[cs:VGASt]
       
        cmp        cl,bl
        je         @@SkipSetVGALow
        mov        al,0Dh               ; Set Normal VGA low byte.
        mov        ah,cl
        mov        [cs:VGASt],cx
        out        dx,ax

@@SkipSetVGALow:

        cmp        ch,bh
        je         @@SkipSetVGAHigh
        mov        al,0Ch               ; Set Normal VGA high byte.
        mov        ah,ch
        mov        [cs:VGASt],cx
        out        dx,ax
@@SkipSetVGAHigh:

        shr        ecx,16
        mov        bl,[cs:SVgaSt]
        cmp        bl,cl
        je         @@SkipSetSVGA

        mov        [cs:SVgaSt],cl
        SetSVGAScrnAddr
@@SkipSetSVGA:
        mov        dx,03dah
@@WaitOutRetrace1:
        in         al,dx
        test       al,8
        jnz        @@WaitOutRetrace1
@@WaitForRetrace:
        in         al,dx
        test       al,8
        jz         @@WaitForRetrace

        and        di,3
        mov        bx,di
        cmp        bl,[cs:BankSt]
        je         @@SkipBankSet
        mov        [cs:BankSt],bl
        ; Set which bank (0-3) should come first.
        ; an in    al,03da is required b4 setting the bank shift to
        ; reset a flip-flop.
        mov        dx,03C0h     ; set the bank shift regs.
        mov        al,(13h+20h)
        out        dx,al        ; Output index w/out destroying bit 20h
        mov        al,bl
        shl        al,1
        out        dx,al
@@SkipBankSet:

        pop  ds
        pop  di
        pop  bp
        ret
ENDP    _SetVGAStart



END     Start

