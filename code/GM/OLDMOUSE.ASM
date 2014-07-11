        IDEAL
        MODEL LARGE
        P286
        LARGESMALL EQU 6
        ADDRSIZE   EQU 4
        extrn   _int86:proc

        DATASEG
_mouinstall dw 0
_MouseMaxy  dw 0
_MouseMaxx  dw 0
mx          dw 0
my          dw 0
mx1         dw 639
my1         dw 199
_MXlatx     dw 8
_MXlaty     dw 8
On          db 0

SEGMENT MOUSEFN_TEXT byte public 'CODE'
	assume	cs:MOUSEFN_TEXT
PROC @initmouse$qv  FAR
        push    ds
        mov     ax, SEG _mouinstall
        mov     ds, ax
        mov     ax,0
        int     33h
        cmp     ax,0FFFFh
        je      @@HasMouse
        mov     ax,0                   ; Return 0
        mov     [_mouinstall],ax
        pop     ds
        ret

@@HasMouse:
        mov     [_mouinstall],1

        mov     ax,4                    ; Set Mouse Position to (1000,1000)
        mov     cx,1000                 ; that is, to the bottom right.
        mov     dx,cx
        int     33h

        mov     ax,3                    ; Get the position to save max info.
        int     33h
        mov     [WORD ds:_MouseMaxy],dx
        cmp     cx,1000
        jle     @@MouseDriverNoBug
        mov     cx,639
@@MouseDriverNoBug:
        mov     [WORD ds:_MouseMaxx],cx
        mov     ax,1

        pop     ds
        ret
ENDP    @initmouse$qv

PROC    @moucur$qi far
        mov     ah,0
        mov     al,[On]
        cmp     [word ptr _mouinstall],0
        je      @@EndFunc
        push    bp
        mov     bp,sp
        mov     bx,[ss:bp+LARGESMALL]
        pop     bp
        cmp     bx,2                      ; Status wanted?
        je      @@EndFunc                 ; yes

        cmp     bx,ax                     ; Does desired = [On]? (ie is it already set correctly?)
        je      @@EndFunc                 ; Yes
        jg      @@TurnOn
; Turn off mouse
        mov     [On],0
        mov     ax,2
        int     33h
        mov     ax,0
        ret
@@TurnOn:
        mov     [On],1
        mov     ax,1
        int     33h
@@EndFunc: ret
ENDP    @moucur$qi

PROC    @setmoupos$qii  FAR

        cmp     word ptr DGROUP:_mouinstall,0
        je      @@EndFunc
        push    bp
        mov     bp,sp

        mov     ax,[word ptr ss:bp+LARGESMALL]  ; Set up the x coord.
        mov     cx,[DGROUP:_MXlatx]
        mul     cx
        shr     cx,1
        add     ax,cx
        push    ax

        mov     ax,[word ptr bp+LARGESMALL+2] ; Set up y coord.
        mov     cx,[DGROUP:_MXlaty]
        mul     cx
        shr     cx,1
        add     ax,cx
        mov     dx,ax

        pop     cx

        mov     ax,4
        int     33h
        pop     bp
@@EndFunc:
        ret
ENDP    @setmoupos$qii

        BUTS EQU [DWORD ss:bp+(2*ADDRSIZE)+LARGESMALL]
        Y    EQU [DWORD ss:bp+(1*ADDRSIZE)+LARGESMALL]
        X    EQU [DWORD ss:bp+LARGESMALL]
PROC    @moustats$qnit1t1 FAR
        push    bp
        mov     bp,sp
        push    es
        push    di
        mov     ax,3
        int     33h
        les     di,BUTS
        mov     [word es:di],bx         ; Store button status.
        mov     ax,dx                   ; Calc + store Y stats.
        mov     dx,0
        div     [word ptr DGROUP:_MXlaty]
        les     di,Y
        mov     [word es:di],ax

        mov     ax,cx                   ; Calc + stor X stats.
        mov     dx,0
        div     [word ptr DGROUP:_MXlatx]
        les     di,X
        mov     [word es:di],ax

        mov     ax,1
        pop     di
        pop     es
        pop     bp
        ret
ENDP    @moustats$qnit1t1

MASM
        assume  cs:MOUSEFN_TEXT
@moucurbox$qiiii        proc    far
	enter	18,0
	push	si
	push	di
	mov	si,word ptr [bp+6]
	mov	di,word ptr [bp+8]
	cmp	word ptr DGROUP:_mouinstall,0
	jne	@@0
	jmp	@5@226
@@0:
	cmp	word ptr [bp+10],si
	jge	short @5@114
	mov	ax,word ptr [bp+10]
	mov	word ptr [bp-2],ax
	mov	word ptr [bp+10],si
	mov	si,word ptr [bp-2]
@5@114:
	cmp	word ptr [bp+12],di
	jge	short @5@170
	mov	ax,word ptr [bp+12]
	mov	word ptr [bp-2],ax
	mov	word ptr [bp+12],di
	mov	di,word ptr [bp-2]
@5@170:
	mov	word ptr DGROUP:mx,si
	mov	word ptr DGROUP:my,di
	mov	ax,word ptr [bp+10]
	mov	word ptr DGROUP:mx1,ax
	mov	ax,word ptr [bp+12]
	mov	word ptr DGROUP:my1,ax
	mov	word ptr [bp-18],7
	mov	ax,si
	imul	word ptr DGROUP:_MXlatx
	mov	word ptr [bp-14],ax
	mov	ax,word ptr [bp+10]
	imul	word ptr DGROUP:_MXlatx
	mov	word ptr [bp-12],ax
	push	ss
	lea	ax,word ptr [bp-18]
	push	ax
	push	ss
	lea	ax,word ptr [bp-18]
	push	ax
	push	51
        call    _int86
	add	sp,10
	mov	word ptr [bp-18],8
	mov	ax,di
	imul	word ptr DGROUP:_MXlaty
	mov	word ptr [bp-14],ax
	mov	ax,word ptr [bp+12]
	imul	word ptr DGROUP:_MXlaty
	mov	word ptr [bp-12],ax
	push	ss
	lea	ax,word ptr [bp-18]
	push	ax
	push	ss
	lea	ax,word ptr [bp-18]
	push	ax
	push	51
        call    _int86
	add	sp,10
	mov	ax,1
@5@198:
	jmp	short @5@254
@5@226:
	xor	ax,ax
	jmp	short @5@198
@5@254:
	pop	di
	pop	si
	leave	
	ret	
@moucurbox$qiiii        endp
	assume	cs:MOUSEFN_TEXT
@getmoubox$qnit1t1t1	proc	far
	push	bp
	mov	bp,sp
	cmp	word ptr DGROUP:_mouinstall,0
	je	short @6@142
	les	bx,dword ptr [bp+6]
	mov	ax,word ptr DGROUP:mx
	mov	word ptr es:[bx],ax
	les	bx,dword ptr [bp+10]
	mov	ax,word ptr DGROUP:my
	mov	word ptr es:[bx],ax
	les	bx,dword ptr [bp+14]
	mov	ax,word ptr DGROUP:mx1
	mov	word ptr es:[bx],ax
	les	bx,dword ptr [bp+18]
	mov	ax,word ptr DGROUP:my1
	mov	word ptr es:[bx],ax
	mov	ax,1
@6@86:
	jmp	short @6@170
	jmp	short @6@170
@6@142:
	xor	ax,ax
	jmp	short @6@86
@6@170:
	pop	bp
	ret	
@getmoubox$qnit1t1t1	endp
	assume	cs:MOUSEFN_TEXT
@mouclearbut$qv	proc	far
	enter	6,0
	cmp	word ptr DGROUP:_mouinstall,0
	je	short @7@114
@7@58:
	push	ss
	lea	ax,word ptr [bp-6]
	push	ax
	push	ss
	lea	ax,word ptr [bp-4]
	push	ax
	push	ss
	lea	ax,word ptr [bp-2]
	push	ax
	push	cs
	call	near ptr @moustats$qnit1t1
	add	sp,12
	cmp	word ptr [bp-6],0
	jne	short @7@58
@7@114:
	leave	
	ret	
@mouclearbut$qv	endp
MOUSEFN_TEXT	ends

        public  _MXlaty
	public	_MXlatx
_my1	equ	my1
_mx1	equ	mx1
_my	equ	my
_mx	equ	mx
	public	_MouseMaxx
	public	_MouseMaxy
	public	_mouinstall
	public	@mouclearbut$qv
	public	@getmoubox$qnit1t1t1
	public	@moucurbox$qiiii
	public	@setmoupos$qii
	public	@moustats$qnit1t1
	public	@moucur$qi
	public	@initmouse$qv
_pokeb	equ	pokeb
_poke	equ	poke
_peekb	equ	peekb
_peek	equ	peek
_s@	equ	s@
        end


