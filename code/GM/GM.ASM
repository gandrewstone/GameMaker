IDEAL
P286
MODEL TINY

REGOK     EQU   045h

STRUC  LoadExecDataBlock
  EnvSeg        dw      0
  CmdLineOff    dw      0
  CmdLineSeg    dw      0
  FCB1Off       dw      0
  FCB1Seg       dw      0
  FCB2Off       dw      0
  FCB2Seg       dw      0
ENDS

CODESEG

ORG 100h
Start:
jmp Main

Stak           db       10 DUP('Stack')
StackBottom:

Programs       db       'quit        ',4 DUP (0)
               db       'utility.exe ',4 DUP (0)
               db       'palchos.exe ',4 DUP (0)
               db       'blocedit.exe',4 DUP (0)
               db       'monedit.exe ',4 DUP (0)
               db       'mapmaker.exe',4 DUP (0)
               db       'charedit.exe',4 DUP (0)
               db       'image.exe   ',4 DUP (0)
               db       'sndedit.exe ',4 DUP (0)
               db       'grator.exe  ',4 DUP (0)
               db       'playgame.exe',4 DUP (0)
Menu           db       'menu.exe    ',4 DUP (0)
;CheckReg       db       'chk.exe     ',4 DUP (0)

CmdLine        db      7,'cOoL ' ; Format: length of string, string, 0dh
Selection      db      ('A'+10),0dh

ErrorMsg       db      'Unable to execute overlay program, try running it seperately.',13,'$'

EPBlock   LoadExecDataBlock <,,,,,>

PROC ExecProg FAR

          mov     ax, cs
          mov     es, ax
          mov     bx, OFFSET EPBlock
  
          mov     ax,4b00h             ; Load and execute program.
          int     21h                  ; Carry flag set if not successful.
          jc      @@ProcEnd
          mov     ah,04dh              ; Get program return code function.
          int     21h                  ; Call dos
          cmp     al,0
          jne     @@ProcEnd
          stc                          ; Set Carry flag -- indicates quit.
          ; Return code in AL
@@ProcEnd:
  retf
ENDP ExecProg

PROC Main
  mov     ax,cs

  mov     ss,ax                         ; Set up my stack.
  mov     sp,OFFSET StackBottom         ; Stack grows up!

  mov     ds,ax                         ; Setup my data seg.

  mov     [EPBlock.CmdLineOff],OFFSET CmdLine  ; Set up cmd line params
  mov     [EPBlock.CmdLineSeg],ax

  mov     bx, 2048  ;OFFSET LastByte ; Free unused memory.
  shr     bx,4                      ; Convert mem amt to paragraphs
  inc     bx                        ; Round up
  mov     ah,4ah                    ; DOS realloc call.
  int     21h

  mov     ax,0003h                  ; Clear the Screen.
  int     10h                       ;

  ; Run check registration program to see if we should continue with GameMaker.
;  mov      dx,OFFSET CheckReg
;  call far ExecProg
;  jc       @@Failure               ; Carry flag set = loading error.
;  cmp      al,REGOK                ; check to see what reg prog returned.
;  jne      @@Failure               ; if its not correct, quit.

  mov      dx,OFFSET Menu           ; Get the first menu selection.
  call far ExecProg
  jc       @@NiceQuit               ; carry set on both error and user quit.

  mov      bl,10                    ; Tell menu.exe that prev program was playgame.

@@LoopTop:             ; Main loop - keep getting programs and running them.
  cmp     bl,11                     ; Was menu last run?
  je      @@Skipit                  ; if menu was last run, use program b4 menu.
  add     bl,'A'                    ; Make the # into a printable char.
  mov     [Selection],bl            ; put prev run program in the cmd line.

@@Skipit:

  mov     ah, 0                     ; Program to run is in al.
  push    ax                        ; Save the program about to be run.
  mov     dx, OFFSET Programs
  shl     ax, 4                     ; each program name uses 16 bytes.
  add     dx,ax
  call far ExecProg
  pop     bx                        ; Restore prev run program.
  jnc     @@LoopTop
@@NiceQuit:
  mov     bl,al
  mov     ax,0003h                  ; Set the Palette Colors back + clear
  int     10h                       ; the screen.
  cmp     bl,0                      ; did we mean to quit?
  je      @@end
@@Failure:
  mov     dx,offset ErrorMsg
  mov     ah,9h
  int     21h
@@end:
  mov     ax,4c00h                  ; Terminate program.
  int     21h
ENDP Main
LastByte:
END Start
