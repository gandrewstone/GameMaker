IDEAL
MODEL LARGE
PUBLIC _MicroChannel
PUBLIC _NewMicroKbd
PUBLIC _NewATKbd
PUBLIC _KeyBoardOff
PUBLIC _KeyBoardOn
PUBLIC _ascii
EXTRN  _keydn:PTR
EXTRN  _KeysPending:PTR,_PendCtr:WORD


DATASEG

; Scan to ascii code table
; they have 16 in the spot on the first lines that I put an 8 in.
; changed [1] from 14 to 27 (ESC key).
; The first is the table w/o any shift keys pressed.
_ascii    db      0,27,'1234567890-=',8,15,'qwertyuiop[]',13,0
          db      'asdfghjkl;''`',0,'\zxcvbnm,./',0,'*',0,32,0
          db      1,2,3,4,5,6,7,8,9,10,0,0,19,0,0,'-',0,'5'
          db      0,'+',0,0,0,0,0,0,0,0,11,12,27h dup(0)
; Shift table
;          db      0,27,'!@#$%^&*()_+',8,15,'QWERTYUIOP{}',13,0
;          db      'ASDFGHJKL:"~',0,'|ZXCVBNM<>?',0,'*',0,32,0
;          db      1,2,3,4,5,6,7,8,9,10,0,0,19,25,21,'-',23,'5'
;          db      24,'+',20,26,22,17,18,0,0,0,11,12,27h dup(0)

;_ascii    db      0,27,'1234567890-=',8,15,'qwertyuiop[]',13,0
;          db      'asdfghjkl;''`',0,'\zxcvbnm,./',0,'*',0,32,0
;          db      1,2,3,4,5,6,7,8,9,10,0,0,19,25,21,'-',23,'5'
;          db      24,'+',20,26,22,17,18,0,0,0,11,12,27h dup(0)
;; Shift table
;          db      0,27,'!@#$%^&*()_+',8,15,'QWERTYUIOP{}',13,0
;          db      'ASDFGHJKL:"~',0,'|ZXCVBNM<>?',0,'*',0,32,0
;          db      1,2,3,4,5,6,7,8,9,10,0,0,19,25,21,'-',23,'5'
;          db      24,'+',20,26,22,17,18,0,0,0,11,12,27h dup(0)
;



OldKey    db      0
InInt     db      0
CurIRQ    db      0
CODESEG


PROC   _KeyBoardOff
       or          [CurIRQ],2    ; Set the keyboard mask bit.
       mov         al,[CurIRQ]
       out         21h,al        ; Tell the controller to turn it off.
       ret
ENDP   _KeyBoardOff

PROC   _KeyBoardOn
       and         [CurIRQ],11111101b  ; Clear the keyboard mask bit.
       mov         al,[CurIRQ]
       out         21h,al              ; Tell the controller to turn it on.
       ret
ENDP   _KeyBoardOn



MACRO EnablePICInts
        mov     al,20h
        out     20h,al
ENDM

MACRO   KeyInt
	push	ax
        push    bx
        push    cx
        push    ds

        mov     ax,DGROUP                  ; Set up Data Seg.
        mov     ds,ax

        cmp     [byte ptr ds:InInt],1
        je      @@EndMacro
        mov     [byte ptr ds:InInt],1
        mov     al,[byte ptr ds:OldKey]
@@KeyStart:
        mov     cl,al
        in      al,60h                     ; Get the keypress.
IF MICROCNL
        cmp     al,0E0h                    ; For microchannels, if the
        jne     short @@ValidKey           ; key is not really one, it = 0E0h
@@GetNextKey:
        in      al,64h                     ; Wait for next key.
        test    al,0
        jne     short @@GetNextKey
        in      al,60h
@@ValidKey:
ENDIF
        cmp     al,cl
        je      @@KeyIntStop

        mov     bx,[word ptr ds:_PendCtr]
        inc     [byte ptr ds:_PendCtr]
        mov     [byte ptr ds:_KeysPending+bx],al
        mov     [byte ptr ds:OldKey],al
        jmp     @@KeyStart

@@KeyIntStop:
        mov     [byte ptr ds:InInt],0  ; Time needs to be wasted here.
@@EndMacro:
                   ; Tell keyboard interrupt serviced.
        in      al,61H                  ; get control port
        mov     ah,AL                   ; copy for later reset
        or      al,80H                  ; set bit to acknowledge
        jmp     short $+2               ; wait for settle
        out     61H,al                  ; do the acknowledge
        jmp     short $+2               ; wait for settle
        mov     al,ah                   ; get original setting
        out     61H,al                  ; put it back
        jmp     short $+2               ; wait for settle

        pop     ds          ; Need to waste some time here, so
        pop     cx          ; do all the pops we can.
        pop     bx
        EnablePICInts
        pop     ax
        iret    
ENDM 


Proc _NewATKbd
     MICROCNL EQU 0
     KeyInt
ENDP


PROC _NewMicroKbd
     MICROCNL EQU 1
     KeyInt
ENDP


PROC    _MicroChannel
     push   es
     push   bx
     mov    ah,0C0h
     int    15h
     add    bx,05h
     mov    al,[es:bx]
     and    al,2h
     shr    al,1
     ; value returned in al
     pop  bx
     pop  es
     ret
ENDP _MicroChannel

END
