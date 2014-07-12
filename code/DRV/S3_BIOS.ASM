
proc    LoadPort
        out     dx,al
;        jmp     $+2
        inc     dx
        mov     ah,al
        in      al,dx
        dec     dx
        xchg    al,ah
        ret
endp    LoadPort

proc     VESASetP
         PUSH  AX
         PUSH  DX
         PUSH  BX
         MOV   BX,DX
         CALL  LoadCRTC
         CALL  InitWrit
         MOV   AL,OFFSET 34H
         CALL  LoadPort
         TEST  AH,2
         JZ    Noshift
         SHL   BX,1
         SHL   BX,1
Noshift: CALL  WritePg
         CALL  L0FA9H
         POP   BX
         POP   DX
         POP   AX
         RET
endp

L0FA9H:  PUSH  dx         ; Done
         CALL  LoadCRTC
         MOV   al,038h
         MOV   ah,0B7h
         OUT   dx,ax
         MOV   al,039h
         MOV   ah,05Ah
         OUT   dx,ax
         POP   dx
         RET
;


proc WritePg
         push  ax
         push  dx
         CALL  InitWrit
         CALL  LoadCRTC
         MOV   AL,OFFSET 31H
         CALL  LoadPort
         OR    AH,1
         OUT   AX,DX
         MOV   AL,OFFSET 6AH
         MOV   AH,BL
         OUT   AX,DX
         pop   dx
         pop   ax
         ret
endp


proc     InitWrit
         PUSH  DX
         CALL  LoadCRTC
         MOV   AL,38h
         CALL  LoadPort
         MOV   AH,48h
         OUT   AX,DX
         MOV   AL,39h
         MOV   AH,0A5h
         OUT   AX,DX
         POP   DX
         ret
endp


proc     LoadCRTC                       ; Done
         push  ax
         MOV   dx,03CCh
         IN    al,DX
         MOV   dx,03B4h
         TEST  al,1
         JZ    Monochrome
         MOV   dx,03D4h
Monochrome:
         POP   AX
         RET
endp
