;-------------------------------------------------------------------------
; scrnrout.asm     Screen routines to enhance C               Andy Stone
;                                                             Aug 29, '90
; Last Edited: 8/6/93
;-------------------------------------------------------------------------

        IDEAL
        MODEL LARGE
        LARGESMALL EQU 6
        PUBLIC          _copypage        
        PUBLIC          _writestr
        PUBLIC          _writech 
        PUBLIC          _readch
        PUBLIC          _drawbox
        PUBLIC          _savebox
        PUBLIC          _xchgbox
        PUBLIC          _restorebox
        PUBLIC          _clrbox
   
        VIDMEM          EQU     0B800h
        TOTALSIZE       EQU     4000            ; total bytes in screen       
        VIDLEN          EQU     80              ; num of chars on a line
        
        DATASEG
        _clrchar        DB      32              ; char to clear scrn to.

        CODESEG
MACRO   pushall
        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    bx
        push    cx
        push    dx
        push    es
        push    di
        push    ds
        push    si
        pushf
ENDM    pushall

MACRO   popall
        popf
        pop     si
        pop     ds
        pop     di
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
ENDM    popall


;-------------------------------------------------------------------------
; clrbox clears a section of the screen to a specified attribute.
; stack =       x
;               y
;               x1
;               y1
;               ATTRIBUTE
;------------------------------------------------------------------------


PROC _clrbox
        SX      EQU     [bp]
        SY      EQU     [bp+2]
        SX1     EQU     [bp+4]
        SY1     EQU     [bp+6]
        ATTRIB  EQU     [bp+8]

        pushall

        mov     ax,VIDMEM                       ; find start of clearing
        mov     es,ax
        mov     bx,SY                           ; calc the starting offset
        mov     ax,VIDLEN
        mul     bx
        add     ax,SX
        mov     bx,2
        mul     bx
        mov     di,ax                               

        mov     bx,SY1                          ; # of lines to save
        sub     bx,SY
        
        mov     cx,SX1                          ; length to save
        sub     cx,SX
        add     cx,1
                                                ; length to skip
        mov     dx,VIDLEN                     
        sub     dx,cx
        mov     ax,2
        mul     dx
        mov     dx,ax
        
        mov     ax,ATTRIB
        mov     ah,al
        mov     al,[_clrchar]
@@lptp: 
        dec     bx      
        push    cx
        repne   stosw
        pop     cx
        add     di,dx
        cmp     bx,-1
        jnz     @@lptp

        popall

        ret
ENDP _clrbox

;-------------------------------------------------------------------------
; xchgbox swaps what's on the scrn w/ what's in memory.
; 
; stack =       x
;               y
;               x1
;               y1
;               OFFSET of place to save to
;------------------------------------------------------------------------

PROC _xchgbox
        SX      EQU     [bp]
        SY      EQU     [bp+2]
        SX1     EQU     [bp+4]
        SY1     EQU     [bp+6]
        TARGET  EQU     [bp+8]

        pushall
                                             ; init addresses 
                                            
        les     di,TARGET                    ; destination is es:di     
        mov     ax,VIDMEM                       ; source is ds:si
        mov     ds,ax
        mov     bx,SY                           ; calc the starting offset
        mov     ax,VIDLEN
        mul     bx
        add     ax,SX
        mov     bx,2
        mul     bx
        mov     si,ax                               

        mov     bx,SY1                          ; # of lines to save
        sub     bx,SY
        
        mov     cx,SX1                          ; length to save
        sub     cx,SX
        add     cx,1
    
                                                ; length to skip
        mov     dx,VIDLEN
        sub     dx,cx
        mov     ax,2
        mul     dx
        mov     dx,ax

@@lptp: 
        dec     bx      
        push    cx
@@lpx:
        dec     cx
        mov     ax,[es:di]
        movsw
        mov     [ds:si - 2],ax
        cmp     cx,0
        jne     @@lpx

        pop     cx
        add     si,dx
        cmp     bx,-1
        jnz     @@lptp

        popall
        
        ret
ENDP _xchgbox

;-------------------------------------------------------------------------
; savebox saves the character and attribute parts of a section of the
; screen
; stack =       x
;               y
;               x1
;               y1
;               SEG:OFFSET of place to save to
;------------------------------------------------------------------------

PROC _savebox
        SX      EQU     [bp]
        SY      EQU     [bp+2]
        SX1     EQU     [bp+4]
        SY1     EQU     [bp+6]
        TARGET  EQU     [bp+8]

        pushall
                                                ; init addresses 
        les     di, TARGET                      ; Dest is in es:di

        mov     ax,VIDMEM                       ; source is ds:si
        mov     ds,ax
        mov     bx,SY                           ; calc the starting offset
        mov     ax,VIDLEN
        mul     bx
        add     ax,SX
        mov     bx,2
        mul     bx
        mov     si,ax                               

        mov     bx,SY1                          ; # of lines to save
        sub     bx,SY
        
        mov     cx,SX1                          ; length to save
        sub     cx,SX
        add     cx,1
        mov     ax,2
        mul     cx
        mov     cx,ax
                                                ; length to skip
        mov     ax,160                          ; VIDLEN * 2
        sub     ax,cx

@@lptp: 
        dec     bx      
        push    cx
        repne   movsb
        pop     cx
        add     si,ax
        cmp      bx,-1
        jnz     @@lptp

        popall

        ret
ENDP _savebox


;-------------------------------------------------------------------------
; restbox restores the character and attribute parts of a section of
; the screen that was saved using the savewindow procedure.
; stack =       x
;               y
;               x1
;               y1
;               OFFSET of where the saved screen is.
;------------------------------------------------------------------------

PROC _restorebox
        SX      EQU     [bp]
        SY      EQU     [bp+2]
        SX1     EQU     [bp+4]
        SY1     EQU     [bp+6]
        SOURCE  EQU     [bp+8]

        pushall
                                                ; init addresses 
        lds     si,SOURCE

        mov     ax,VIDMEM                       ; dest is es:di (on screen)
        mov     es,ax
        mov     bx,SY                           ; calc the starting offset
        mov     ax,VIDLEN
        mul     bx
        add     ax,SX
        mov     bx,2
        mul     bx
        mov     di,ax                               

        mov     bx,SY1                          ; # of lines to save
        sub     bx,SY
        
        mov     cx,SX1                          ; length to save
        sub     cx,SX
        add     cx,1
        mov     ax,2
        mul     cx
        mov     cx,ax
                                                ; length to skip
        mov     ax,160                          ; VIDLEN * 2
        sub     ax,cx

@@lptp: 
        dec     bx      
        push    cx
        push    ax
        repne   movsb
        pop     ax
        pop     cx
        add     di,ax
        cmp     bx,-1
        jnz     @@lptp

        popall
        ret
ENDP _restorebox

 linetypes       db      '      ڿ��ĳɻȼͺ'
 corners         dw     4 dup(0)

;-----------------------------------------------------------------------------
; drawbox draws a box on the screen using one of the above character sets
; stack  =      x
;               y
;              x1    
;              y1
;             col
;            type
;            shcol
; drawbox (x,y,x1,y1,col,type,shcol)
;----------------------------------------------------------------------------

PROC _drawbox
        SX      EQU     [bp]
        SY      EQU     [bp+2]
        SX1     EQU     [bp+4]
        SY1     EQU     [bp+6]
        COLOR   EQU     [bp+8]
        LTYPE   EQU     [bp+10]
        SHCOL   EQU     [bp+12]

        pushall
        
        mov     ax,VIDMEM               ; Set ds to the video memory segment
        mov     es,ax

        mov     ax, LTYPE                ; get start of correct lines
        mov     bx, 6
        mul     bx
        mov     bx,ax                   ; correct start now in bx   

                           ; Draw Upper left corner
        mov     cx,VIDLEN
        mov     ax,SY
        mul     cx                      ; Get y offset
        add     ax,SX                   ; add x offest to it
        mov     cx,2
        mul     cx                      ; 2 bytes per char so multiply by 2
        mov     di, ax
        mov     [cs:corners],ax

        mov     al,[cs:linetypes + bx]     ; load char
        mov     dx,COLOR                ; load color        
        mov     ah,dl
        stosw

                          ; Draw Upper right corner
        inc     bx
        mov     ax,SX1
        sub     ax,SX                   ; get the length
        dec     ax
        shl     ax,1                    ; mult. by 2 - 2 bytes per character on screen
        add     di,ax        
        mov     [cs:corners+2],di

        mov     al,[cs:linetypes + bx]     ; load char
        mov     dx,COLOR                ; load color        
        mov     ah,dl
        stosw 

                          ; Draw lower left corner
        inc     bx
        mov     cx,VIDLEN
        mov     ax,SY1
        mul     cx                      ; Get y offset
        add     ax,SX                   ; add x offest to it
        shl     ax,1                    ; mult by 2
        mov     di, ax
        mov     [cs:corners+4],ax
        mov     al,[cs:linetypes + bx]     ; load char
        mov     dx,COLOR                ; load color        
        mov     ah,dl
        stosw 
                          ; Draw lower right corner
        inc     bx
        mov     ax,SX1
        sub     ax,SX                   ; get the length
        dec     ax
        shl     ax,1                    ; mult. by 2 - 2 bytes per character on screen
        add     di,ax        
        mov     [cs:corners+6],di

        mov     al,[cs:linetypes + bx]     ; load char
        mov     dx,COLOR                ; load color        
        mov     ah,dl
         cld
        stosw 

                          ; Draw horizontal lines
        inc     bx
        mov     cx,SX1                 ; cx now contains the length of the
        sub     cx,SX                  ; horizontal line. 
        sub     cx,1                   ; Sub so that I don't write over the corners

        mov     di,[cs:corners]
        add     di,2
        mov     al,[cs:linetypes + bx]
        mov     dx,COLOR               ; load color        
        mov     ah,dl
        cld
        push    cx
        repnz   stosw
        pop     cx
        mov     di,[cs:corners+4]
        add     di,2
        cld
        repnz   stosw

                                        ; draw vertical lines
        inc     bx

        mov     al,[cs:linetypes + bx]     ; Get the character to draw in
        mov     dx,COLOR                ; load color        
        mov     ah,dl                   ; load color into ax
        push    ax                      ; Save the integer to put on the screen
        
        mov     cx,SX1                  ; find length of box
        sub     cx,SX                   
        dec     cx                      ; cx is now the length to add between
        shl     cx,1                    ; sides of the box.

        mov     ax,VIDLEN
        shl     ax,1                    ; compute length of a line in memory
        sub     ax,cx                   
        mov     bx,ax       
        sub     bx,4                    ; sub some for the shadow
                                        ; bx = len AROUND to the other side.        
        mov     dx,SY1
        sub     dx,SY                   
        dec     dx                      ; dx is num of vertical lines to draw
        
        mov     di,[cs:corners+2]
        add     di,2
        add     di,bx
        pop     ax

@@looptp:     
        dec     dx                      
        stosw                           ; draw left vertical line
        add     di,cx
        stosw                           ; draw right vertical line

        push    ax
        mov     ax,SHCOL
        cmp     al,0
        je      @@novshadow
        inc     di                      ; drawing the vert shadow
        stosb
        inc     di
        stosb
        sub     di,4
@@novshadow:
        pop     ax

        add     di,bx
        cmp     dx,0
        jne     @@looptp

        add     di,cx                            ; Finish Vertical shadow
        add     di,4
        mov     ax,SHCOL
        cmp     al,0
        je      @@done
        inc     di
        stosb
        inc     di
        stosb
        sub     di,4
                                        ; draw horizontal shadow
        mov     di,[cs:corners+4]
        add     di,VIDLEN
        add     di,VIDLEN
        add     di,5
        mov     ax,SHCOL
;        cld

        mov     cx,SX1                  ; number of interations
        sub     cx,SX
        add     cx,1
@@looptp2:
        dec     cx
        stosb
        inc     di                      ; change every other byte to get
                                        ; every color, and no characters
        or      cx,cx
        jnz     @@looptp2
                
@@done:
        popall
        ret

ENDP _drawbox
        
;----------------------------------------------------------------------------
; copypage copies one text video page to another
; stack = source
;         destination
; copypage(source,dest)
;----------------------------------------------------------------------------

PROC _copypage
        
        SOURCE EQU [bp]
        DEST   EQU [bp+2]

        pushall

        mov     ax,VIDMEM               ; Set segment registers to the
        mov     ds,ax                   ; Video memory.
        mov     es,ax

        mov     bx,TOTALSIZE            ; Put the bytes per page in bx

        mov     ax,SOURCE               ; Find the offset of the source
        mul     bx                      ;  video page by multiplying the
        mov     si,ax                   ;  page # (0-3) by the bytes per pg.

        mov     ax,DEST                 ; Find the offest of the destination
        mul     bx                      ; video page using the same method.
        mov     di,ax

        cld                             ; movsb goes forewards
        mov     cx,TOTALSIZE
        repnz   movsb                

        popall
        ret

ENDP _copypage

;----------------------------------------------------------------------------
;  Puts a null terminated string on the screen at an x,y location,
;                       in any color
;  stack top =     
;                        x
;                        y
;                        col
;                        SEG:offset to start of string 
;  writestr(x,y,col,char *str)
;----------------------------------------------------------------------------

PROC  _writestr

        pushall
                ; Calculate the position on the screen

        mov     ax, [bp+2]              ; Y coordinate to put char
        mov     bx, VIDLEN              ; length of scrn in bx
        mul     bx                      ; y offset is in ax
        add     ax,[bp]                 ; X coordinate added
        mov     bx,2
        mul     bx                      ; 2 bytes per scrn cell

        mov     bx, VIDMEM              ; set up for stosw instruction
        mov     es, bx
        mov     di, ax

        mov     dx,[bp+4]              ; get color into a register
        lds     bx,[bp+6]               ; Put address of string in bx
@@nextch:
        
        mov     al, [BYTE ds:bx]        ; put char in al
        cmp     al,0                    ; is cx 0?
        jz      @@Exit                  ; Do this if yes.

        cld
        stosb                           ; put char on the scrn
        cmp     dx,256
        jge     @@skipcolor
        mov     al,dl;
        stosb
        inc     bx
        jmp     @@nextch
@@skipcolor:
        inc     di
        inc     bx                      ; bx now points to the next char
      jmp     @@nextch

@@Exit:
        popall
        ret

ENDP    _writestr
      
;----------------------------------------------------------------------------
; Writes a character directly onto the screen
; stack =    x 
;            y
;            col
;            char
;   writech(x,y,col,char)
;----------------------------------------------------------------------------

PROC  _writech
      
        pushall

        mov     ax, [bp+2]              ; Y coordinate to put char
        mov     bx, VIDLEN              ; length of scrn in bx
        mul     bx                      ; y offset is in ax
        add     ax,[bp]                 ; X coordinate added
        mov     bx,2
        mul     bx                      ; 2 bytes per scrn cell

        mov     bx, VIDMEM              ; set up for stosb instruction
        mov     es, bx
        mov     di, ax
        
        mov     ax, [bp+6]              ; put char in al
        cmp     ax,256                  ; if >= don't draw character
        jge     @@skipchar
        cld
        stosb                           ; put char on the scr
        jmp     @@docolor
@@skipchar:
        inc     di
@@docolor:
        mov     ax, [bp+4]              ; put color in ah
        cmp     ax,256                  ; if >= don't draw a color
        jge     @@skipcolor 
        stosb       
@@skipcolor:

        popall
        ret
ENDP  _writech


PROC  _readch
      
        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    bx
        push    ds
        push    si
        
        mov     ax, [bp+2]              ; Y coordinate to get char from
        mov     bx, VIDLEN              ; length of scrn in bx
        mul     bx                      ; y offset is in ax
        add     ax,[bp]                 ; X coordinate added
        mov     bx,2
        mul     bx                      ; 2 bytes per scrn cell

        mov     bx, VIDMEM              ; set up for lodsw instruction
        mov     ds, bx
        mov     si, ax
        
        lodsw                           ; get char from scrn

        pop     si
        pop     ds
        pop     bx
        pop     bp
        ret
ENDP  _readch
      
END          
