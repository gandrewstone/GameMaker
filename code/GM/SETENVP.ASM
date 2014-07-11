;[]-----------------------------------------------------------------[]
;|      SETENVP.ASM -- Prepare Environment                           |
;|                                                                   |
;|      C/C++ Run Time Library        Version 4.0                    |
;|                                                                   |
;|      Copyright (c) 1987, 1991 by Borland International Inc.       |
;|      All Rights Reserved.                                         |
;[]-----------------------------------------------------------------[]

;gas
DEBUG equ 0
IDEAL
MACRO DebugPrinter String
IF DEBUG
   pushf
   push         ax
   push         dx
   push         ds
   mov          ax,DGROUP
   mov          ds,ax
   mov          dx,OFFSET DebugStr
   mov          ah,9h
   int          21h
   mov          dx,OFFSET String
   mov          ah,9h
   int          21h
   mov          al,[ds:DebugCtr]
   inc          al
   mov          [ds:DebugCtr],al
   pop          ds
   pop          dx
   pop          ax
   popf
ENDIF
ENDM
MASM
        INCLUDE RULES.ASI

;       Segment and Group declarations

Header@

;       External references
EXTRN DebugPrint:PROC
ExtProc@        malloc, __CDECL__
;ExtProc@        abort, __CDECL__
extrn           ErrAbort:PROC


ExtSym@         _envseg, WORD, __CDECL__
ExtSym@         _envLng, WORD, __CDECL__
ExtSym@         _envSize, WORD, __CDECL__
dPtrExt@        _C0environ, __CDECL__

        SUBTTL  Prepare Environment
        PAGE
;/*                                                     */
;/*-----------------------------------------------------*/
;/*                                                     */
;/*     Prepare Environment                             */
;/*     -------------------                             */
;/*                                                     */
;/*-----------------------------------------------------*/
;/*                                                     */

;This is the symbol which pulls in _setenvp
                public __setenvp__
__setenvp__     equ     0

_INIT_          SEGMENT WORD PUBLIC 'INITDATA'
                db      0                       ;near call
                db      16                      ;priority 16
                dw      offset __setenvp
                dw      ?
_INIT_          ENDS

;       Allocate a buffer to hold environment variables

_TEXT           segment

__setenvp       proc near
                push    si
                push    di
                DebugPrinter DBUGsetenvpS

IF      LDATA EQ 0
                mov     cx, _envLng@
                push    cx
                call    malloc@
                pop     cx
                mov     di, ax
                or      ax, ax
                jz      _Failed         ; Memory allocation failed
                push    ds
                push    ds
                pop     es
                mov     ds, _envseg@
                xor     si, si
                cld
                rep     movsb
                pop     ds
                mov     di, ax
ELSE
                mov     es, _envseg@
                xor     di, di
ENDIF

;       Allocate a buffer to hold envp array

                push    es              ; Save Environment Segment address
                push    _envSize@
                DebugPrinter DBUGBeforeMalloc
                call    malloc@
                pop     bx
                mov     bx, ax
                pop     es              ; Restore Environment Segment address
IF      LDATA
                mov     word ptr environ@, ax
                mov     word ptr environ@+2, dx
                push    ds
                mov     ds, dx
                or      ax, dx
ELSE
                mov     word ptr environ@, ax
                or      ax, ax
ENDIF
                jnz     SetEnviron
_Failed         label   near            ; Memory allocation failed
                mov     dx, SEG MemAllocFailureMsg
                mov     es, dx
                mov     dx, OFFSET MemAllocFailureMsg
                jmp     ErrAbort

;       Now, store environment variables address

SetEnviron      label   near
                xor     ax, ax
                mov     cx, -1
IF  LDATA
                cmp byte ptr es:[di], 0     ; Is the environment empty?
ELSE
                cmp byte ptr [di], 0        ; Is the environment empty?
ENDIF
                je  SetEnviron1
SetEnviron0     label   near
                mov     [bx], di
IF      LDATA
                mov     [bx+2], es
                add     bx, 4
ELSE
                add     bx, 2
ENDIF
                repnz   scasb
                cmp     es:[di], al
                jne     SetEnviron0         ; Set next pointer
SetEnviron1     label   near
IF      LDATA
                mov     [bx], ax
                mov     [bx+2], ax
                pop     ds
ELSE
                mov     [bx], ax
ENDIF
                pop     di
                pop     si

IF      LDATA
                mov     ax,word ptr _environ+2
                mov     word ptr __C0environ+2,ax
ENDIF

                DebugPrinter DBUGsetenvpF
                mov     ax,word ptr _environ
                mov     word ptr __C0environ,ax
                ret

                endp
                ends

_DATA           segment

MemAllocFailureMsg   db 42,'GM.Setenvp: Out of near memory (malloc)!',13,10


DBUGsetenvpS    db  ' Start',13,10,'$'
DBUGsetenvpF    db  ' Finish!',13,10,'$'
DBUGBeforeMalloc db  ' Just b4 the malloc.',13,10,'$'

DebugStr        db  'SetENVp.asm:'
DebugCtr        db  'A'
                db  '. $'

dPtrPub@        environ,        0,                      __CDECL__
                ends
                END
