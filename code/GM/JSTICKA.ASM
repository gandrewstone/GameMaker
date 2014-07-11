
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;       jsticka.asm                                       ;
;                                                         ;
;       Assembly Language Functions to Read               ;
;       the Game Port/Joystick Values                     ;
;                                                         ;
;       Creation Date:2-Jan-1992                          ;
;       Author:Peter Savage                               ;
;                                                         ;
;       Call Format                                       ;
;               ReadJoyStick(&x,&y,&butn)                 ;
;                       on Return                         ;
;                   x=-1, y=-1, butn=-1                   ;
;               when gameport not in system               ;
;                                                         ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
        IDEAL
        MODEL  LARGE
        CODESEG
;
GAMPORT EQU     0201H                   ; Define game port I/O address
GAMLCNT EQU     0300H                   ; Loop counter value for bit count loop
NOPORT  EQU     0FFh                    ; Define value game port not present
;
        PUBLIC _ReadJoyStick
PROC    _ReadJoyStick FAR
        ARG  joyx: Dword, joyy: Dword, butn: Dword

        push bp
        mov  bp,sp
 ;       push    ax                      ; Save off AX as a work register
 ;       push    cx                      ; Save off CX as a work register
 ;       push    bx                      ; Save off BX as a work register
 ;       push    dx                      ; Save off DX as a work register
        push    di                      ; Save off DI as a work register
        push    si                      ; Save off SI as a work register
        push    ds                      ; Save off DS as a work segment
        push    es                      ; Save off ES as a work segment
        sub     ax,ax                   ; Clear a work register
        dec     ax                      ; Set register value to -1
        lds     bx,[butn]               ; Fetch address of the button set flag
        mov     [ds:bx],ax              ; Set no game port button flag
        lds     di,[joyx]               ; Fetch address of stick X return value
        mov     [ds:di],ax              ; Set no game port X return value
        les     si,[joyy]               ; Fetch address of stick Y return value
        mov     [es:si],ax              ; Set no game port Y return value
        mov     cx,GAMLCNT              ; Set up loop count index value
        mov     dx,GAMPORT              ; Set I/O address of the game port
        in      al,dx                   ; Fetch data, see if game port on system
        cmp     al,NOPORT               ; Is there a game port on this machine?
        je      @@endfn                 ; If not then exit the function
        inc     [word ptr ds:di]        ; Set initial X Coordinates to zero
        inc     [word ptr es:si]        ; Set initial Y Coordinates to zero
        cli                             ; Disable interrupts while counting
        out     dx,al                   ; Tell Game port to return status
@@xbit:                                 ; Label for X bit check branch
        in      al,dx                   ; Fetch data from the game port
        test    al,1h                   ; Is there an X Coordiantes bit set?
        je      @@ybit                  ; If not then don't count it
        inc     [word ptr ds:di]        ; Bump up the X Coordinates counter
@@ybit:                                 ; Label for Y bit check branch
        test    al,2h                   ; Is there an Y Coordiantes bit set?
        je      @@tstend                ; If not then time to bump loop index
        inc     [word ptr es:si]        ; Bump up the Y Coordinates counter
@@tstend:                               ; Label for end of loop check
        test    al,3                    ; Any X or Y Coordiantes bit set?
        je      @@rbdat                 ; Exit loop if no more bits to count
        loop    @@xbit                  ; Loop until index poll count expended
@@rbdat:                                ; Label to branch to return data values
        xor     ax,30h                  ; Invert joy stick button bits
        and     ax,30h                  ; Strip off all but button bits
        mov     cl,04h                  ; Setup for move to low part of register
        shr     ax,cl                   ; Move Button bits to bits 0 and 1
        lds     bx,[butn]               ; Load ds:bx with the button address
        mov     [ds:bx],ax              ; Set Button is pushed return value
        sti                             ; Re-Enable system interrupts
@@endfn:                                ; Label for end of function cleanup
        pop     es                      ; Restore seg reg. ES 
        pop     ds                      ; Restore seg reg. DS
        pop     si                      ; Restore register SI
        pop     di                      ; Restore register DI
;        pop     dx                      ; Restore register DX
;        pop     cx                      ; Restore register CX
;        pop     bx                      ; Restore register BX
;        pop     ax                      ; Restore register AX
        pop     bp
        ret                            ; Return from function
ENDP    _ReadJoyStick                    ; Define end of _ReadJoyStick function
        END
