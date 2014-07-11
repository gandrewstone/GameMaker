// ObjectWindows - (C) Copyright 1992 by Borland International

/* --------------------------------------------------------
  OWLMATH.CPP
  Math support for scroller.cpp
  -------------------------------------------------------- */

#include   <windows.h>

#pragma inline

struct Quadword {
  WORD w0, w1, w2, w3;
};

/* LongMulDiv multiplies the first two arguments and then
  divides by the third.  This is used so that real number arithmetic
  is not necessary.  This routine saves the possible 64-bit value in
  a temp before doing the divide.  Does not do error checking like
  divide by zero.  Also assumes that the result is in the 32-bit range
  (Actually 31-bit, since this algorithm is for unsigned). */
long LongMulDiv(long Mult1, long Mult2, long Div1)
{
  Quadword Temp;
  WORD WTemp1, WTemp2;

  asm {
    MOV DX, Mult1.w1
    MOV AX, Mult1.w0
    MOV CX, Mult2.w1
    MOV BX, Mult2.w0

    MOV DI,DX
    MOV     SI,AX
    MUL     BX
    MOV     Temp.w0,AX
    MOV     Temp.w1,DX

    MOV     AX,DI
    MUL     CX
    MOV     Temp.w2,AX
    MOV     Temp.w3,DX
    MOV     AX,DI
    MUL     BX
    ADD     Temp.w1,AX
    ADC     Temp.w2,DX
        ADC     WORD PTR Temp.w3,0

        MOV     AX,SI
        MUL     CX
        ADD     Temp.w1,AX
        ADC     Temp.w2,DX
        ADC     WORD PTR Temp.w3,0

	MOV	DX,Temp.w3
	MOV	SI,Temp.w2
	MOV	BX,Temp.w1
	MOV	AX,Temp.w0

	MOV	CX,Div1.w1              /* Adjust for rounding */
	MOV	DI,Div1.w0
	SHR	CX,1
        RCR	DI,1
        ADD	AX,DI
	ADC	BX,CX
	ADC	SI,0
	ADC	DX,0

        MOV     CX,32          /* Div */
        CLC
  }
Label1:
  asm {
	  RCL     AX,1
        RCL     BX,1
        RCL     SI,1
        RCL     DX,1
        JNC     Label3
  }
Label2:
  asm {
        SUB	SI,Div1.w0
        SBB	DX,Div1.w1
        STC
        LOOP    Label1
        JMP     Label5
  }
Label3:
  asm {
        CMP     DX,Div1.w1
        JC      Label4
        JNE     Label2
        CMP     SI,Div1.w0
        JNC     Label2
  }
Label4:
  asm {
        CLC
        LOOP    Label1
  }
Label5:
  asm {
        RCL     AX,1
        RCL     BX,1

        MOV     CX,SI
        MOV     DX,BX
  }
  WTemp1 = _AX;
  WTemp2 = _DX;
  return MAKELONG(WTemp1, WTemp2);
}

