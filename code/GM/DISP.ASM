	.286p
	ifndef	??version
?debug	macro
	endm
publicdll macro	name
	public	name
	endm
$comm	macro	name,dist,size,count
	comm	dist name:BYTE:count*size
	endm
	else
$comm	macro	name,dist,size,count
	comm	dist name[size]:BYTE:count
	endm
	endif
	?debug	V 301h
	?debug	S "disp.c"
	?debug	C E96B67C41C06646973702E63
	?debug	C E900204F1C14433A5C4243345C494E435C70726F636573732E68
	?debug	C E900204F1C12433A5C4243345C494E435C5F646566732E68
	?debug	C E900204F1C12433A5C4243345C494E435C737464696F2E68
	?debug	C E900204F1C13433A5C4243345C494E435C5F6E66696C652E68
	?debug	C E900204F1C12433A5C4243345C494E435C5F6E756C6C2E68
	?debug	C E900204F1C10433A5C4243345C494E435C646F732E68
	?debug	C E900204F1C11433A5C4243345C494E435C62696F732E68
	?debug	C E900204F1C12433A5C4243345C494E435C616C6C6F632E68
	?debug	C E900204F1C13433A5C4243345C494E435C7374646C69622E68
	?debug	C E99571C41C0567656E2E68
	?debug	C E90384C21C0570616C2E68
	?debug	C E9EC8A681A0877696E64696F2E68
	?debug	C E92078C41C096D6F757365666E2E68
	?debug	C E9CF78061B0A7363726E726F75742E68
	?debug	C E9460DFB1A06626C6F632E68
	?debug	C E975096A18056769662E68
	?debug	C E99A82781B086A737469636B2E68
	?debug	C E92A23BB1C0767726170682E68
	?debug	C E911A1E11A0A666163656C6966742E68
	?debug	C E93071C41C0C7472616E6D6F75732E687070
	?debug	C E900204F1C13433A5C4243345C494E435C737472696E672E68
	?debug	C E900204F1C13433A5C4243345C494E435C6C6F63616C652E68
DISP_TEXT	segment byte public 'CODE'
DISP_TEXT	ends
DGROUP	group	_DATA,_BSS
	assume	cs:DISP_TEXT,ds:DGROUP
_DATA	segment word public 'DATA'
d@	label	byte
d@w	label	word
_DATA	ends
_BSS	segment word public 'BSS'
b@	label	byte
b@w	label	word
_BSS	ends
DISP_TEXT	segment byte public 'CODE'
   ;	
   ;	void NewBombOut(void)
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@NewBombOut$qv	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  TextMode();
   ;	
	call	far ptr @TextMode$qv
   ;	
   ;	  printf("GM:Not Enough Memory! (new operator)\n");
   ;	
	push	ds
	push	offset DGROUP:s@
	call	far ptr _printf
	add	sp,4
   ;	
   ;	  exit(1);
   ;	
	push	1
	call	far ptr _exit
	pop	cx
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@NewBombOut$qv	endp
	?debug	C E92563581C0C67656E636C6173732E687070
	?debug	C E900204F1C10433A5C4243345C494E435C6D656D2E68
	?debug	C E900204F1C12433A5C4243345C494E435C616C6C6F632E68
	?debug	C E900204F1C13433A5C4243345C494E435C7374646C69622E68
	?debug	C E900204F1C12433A5C4243345C494E435C737464696F2E68
	?debug	C E9E369581C0974696D65722E687070
	?debug	C E99571C41C0567656E2E68
	?debug	C E92563581C0C67656E636C6173732E687070
DISP_TEXT	ends
_BSS	segment word public 'BSS'
_Time	label	word
	db	9 dup (?)
	?debug	C E94564581C0C77696E64636C73732E687070
	?debug	C E90E60691C0C676173636C6173732E687070
	?debug	C E92E87A81C0C67656E696E7075742E687070
	?debug	C E900204F1C10433A5C4243345C494E435C646F732E68
	?debug	C E900204F1C12433A5C4243345C494E435C737464696F2E68
	?debug	C E99571C41C0567656E2E68
	?debug	C E99A82781B086A737469636B2E68
	?debug	C E92563581C0C67656E636C6173732E687070
	?debug	C E9E369581C0974696D65722E687070
	?debug	C E94564581C0C77696E64636C73732E687070
	?debug	C E90E60691C0C676173636C6173732E687070
_BSS	ends
_DATA	segment word public 'DATA'
_mouinstall	label	word
	db	0
	db	0
_DATA	ends
_BSS	segment word public 'BSS'
_Scrn	label	word
	db	34 dup (?)
_i	label	word
	db	41 dup (?)
_GPFArrow	label	word
	db	4 dup (?)
_BSS	ends
_DATA	segment word public 'DATA'
_Cur	label	word
	db	40 dup (0)
_Jbuttons	label	word
	db	0
	db	0
_DATA	ends
DISP_TEXT	segment byte public 'CODE'
   ;	
   ;	void MouseMoved(void)
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@MouseMoved$qv	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  i.m.Change();
   ;	
	push	ds
	push	offset DGROUP:_i
	call	far ptr @MouseClass@Change$qv
	add	sp,4
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@MouseMoved$qv	endp
   ;	
   ;	void KeyUpdate(int KeyInfo)  
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@KeyUpdate$qi	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  Jbuttons=KeyInfo&255;
   ;	
	mov	ax,word ptr [bp+6]
	and	ax,255
	mov	word ptr DGROUP:_Jbuttons,ax
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@KeyUpdate$qi	endp
   ;	
   ;	int initmouse(void)
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@initmouse$qv	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  //set_new_handler(NewBombOut);
   ;	  _new_handler = NewBombOut;
   ;	
	mov	word ptr DGROUP:__new_handler+2,seg @NewBombOut$qv
	mov	word ptr DGROUP:__new_handler,offset @NewBombOut$qv
   ;	
   ;	  Time.TurnOn();
   ;	
	push	ds
	push	offset DGROUP:_Time
	call	far ptr @TimerClass@TurnOn$qv
	add	sp,4
   ;	
   ;	  InitGraphics(&Scrn);
   ;	
	push	ds
	push	offset DGROUP:_Scrn
	call	far ptr @InitGraphics$qn12DrawingBoard
	add	sp,4
   ;	
   ;	  Cur.NewGPF(&GPFArrow);
   ;	
	push	ds
	push	offset DGROUP:_GPFArrow
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@NewGPF$qn8GPFClass
	add	sp,8
   ;	
   ;	  Cur.SetHotSpot(1,0);
   ;	
	push	0
	push	1
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@SetHotSpot$qii
	add	sp,8
   ;	
   ;	  CurMode=GMODE;
   ;	
	mov	word ptr DGROUP:_CurMode,19
   ;	
   ;	  Cur.Limit(0,0,319,199);
   ;	
	push	199
	push	319
	push	0
	push	0
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@Limit$qiiii
	add	sp,12
   ;	
   ;	  CurMode=TMODE;
   ;	
	mov	word ptr DGROUP:_CurMode,3
   ;	
   ;	  i.Init();
   ;	
	push	ds
	push	offset DGROUP:_i
	call	far ptr @InputClass@Init$qv
	add	sp,4
   ;	
   ;	  if (!i.m.Has) i.j.AddCursor(&Cur);
   ;	
	cmp	byte ptr DGROUP:_i+16,0
	jne	short @4@86
	push	ds
	push	offset DGROUP:_Cur
	push	ds
	push	offset DGROUP:_i+17
	call	far ptr @JoyStickClass@AddCursor$qn11CursorClass
	jmp	short @4@114
@4@86:
   ;	
   ;	  else i.m.AddCursor(&Cur);
   ;	
	push	ds
	push	offset DGROUP:_Cur
	push	ds
	push	offset DGROUP:_i
	call	far ptr @MouseClass@AddCursor$qn11CursorClass
@4@114:
	add	sp,8
   ;	
   ;	  mouinstall=i.m.Has;
   ;	
	mov	al,byte ptr DGROUP:_i+16
	cbw	
	mov	word ptr DGROUP:_mouinstall,ax
   ;	
   ;	  return (i.m.Has|i.j.Has);
   ;	
	mov	al,byte ptr DGROUP:_i+16
	or	al,byte ptr DGROUP:_i+40
	cbw	
	pop	bp
	ret	
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@initmouse$qv	endp
   ;	
   ;	int moucur(int show)    /* show mouse cursor if show = 1 */
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@moucur$qi	proc	far
	push	bp
	mov	bp,sp
	push	si
	mov	si,word ptr [bp+6]
   ;	
   ;	                        /* hide cursor if show = 0, or anything else */
   ;	  {                     /* Return cursor status if 2 */
   ;	  if (CurMode==GMODE)
   ;	
	cmp	word ptr DGROUP:_CurMode,19
	jne	short @5@254
   ;	
   ;	    {
   ;	    if (show==2) return(Cur.Show);
   ;	
	cmp	si,2
	jne	short @5@114
	mov	al,byte ptr DGROUP:_Cur+19
	cbw	
	jmp	short @5@310
@5@114:
   ;	
   ;	    if (show==0) Cur.Off();
   ;	
	or	si,si
	jne	short @5@170
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@Off$qv
	add	sp,4
@5@170:
   ;	
   ;	    if (show==1) Cur.On();
   ;	
	cmp	si,1
	jne	short @5@226
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@On$qv
	add	sp,4
@5@226:
   ;	
   ;	    return(TRUE);
   ;	
	mov	ax,1
	jmp	short @5@310
@5@254:
   ;	
   ;	    }
   ;	  Cur.Forget();
   ;	
	mov	byte ptr DGROUP:_Cur+19,0
   ;	
   ;	  return(FALSE);
   ;	
	xor	ax,ax
	jmp	short @5@310
@5@310:
   ;	
   ;	  }
   ;	
	pop	si
	pop	bp
	ret	
@moucur$qi	endp
   ;	
   ;	int setmoupos(int x,int y)  // sets the mouse position on the screen
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@setmoupos$qii	proc	far
	push	bp
	mov	bp,sp
	push	si
	push	di
   ;	
   ;	  {
   ;	  if (CurMode==GMODE) Cur.Goto(x,y);
   ;	
	cmp	word ptr DGROUP:_CurMode,19
	jne	short @6@86
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@Goto$qii
	add	sp,8
	jmp	short @6@226
@6@86:
   ;	
   ;	  else
   ;	    {
   ;	    int tx,ty;
   ;	    tx=x<<2; ty=y<<2;
   ;	
	mov	ax,word ptr [bp+6]
	shl	ax,2
	mov	si,ax
	mov	ax,word ptr [bp+8]
	shl	ax,2
	mov	di,ax
   ;	
   ;	    if (!InBox(Cur.Curx,Cur.Cury,tx,ty,tx+4,ty+4)) Cur.Goto(tx+2,ty+2);
   ;	
	cmp	word ptr DGROUP:_Cur+24,si
	jl	short @6@198
	cmp	word ptr DGROUP:_Cur+26,di
	jl	short @6@198
	lea	ax,word ptr [si+4]
	cmp	ax,word ptr DGROUP:_Cur+24
	jl	short @6@198
	lea	ax,word ptr [di+4]
	cmp	ax,word ptr DGROUP:_Cur+26
	jge	short @6@226
@6@198:
	lea	ax,word ptr [di+2]
	push	ax
	lea	ax,word ptr [si+2]
	push	ax
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@Goto$qii
	add	sp,8
@6@226:
   ;	
   ;	    }
   ;	  return(TRUE);
   ;	
	mov	ax,1
	jmp	short @6@254
@6@254:
   ;	
   ;	  }
   ;	
	pop	di
	pop	si
	pop	bp
	ret	
@setmoupos$qii	endp
   ;	
   ;	int moustats(int far *x,int far *y,int far *but)  // get mouse position and the buttons hit
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@moustats$qnit1t1	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  if (Time.OldClock<Clock)
   ;	
	mov	ax,seg _Clock
	mov	es,ax
	mov	dx,word ptr es:_Clock+2
	mov	ax,word ptr es:_Clock
	cmp	dx,word ptr DGROUP:_Time+7
	jb	short @7@142
	ja	short @7@114
	cmp	ax,word ptr DGROUP:_Time+5
	jbe	short @7@142
@7@114:
   ;	
   ;	    {
   ;	    Time.OldClock=Clock;
   ;	
	mov	ax,seg _Clock
	mov	es,ax
	mov	dx,word ptr es:_Clock+2
	mov	ax,word ptr es:_Clock
	mov	word ptr DGROUP:_Time+7,dx
	mov	word ptr DGROUP:_Time+5,ax
   ;	
   ;	    Time.Traverse(CallEm);
   ;	
	push	0
	push	0
	push	seg @CallEm$qn9NodeClass
	push	offset @CallEm$qn9NodeClass
	push	ds
	push	offset DGROUP:_Time
	call	far ptr @LinkedListClass@Traverse$qnqn9NodeClass$zcn9NodeClass
	add	sp,12
@7@142:
   ;	
   ;	    }
   ;	  *x=Cur.Curx;
   ;	
	les	bx,dword ptr [bp+6]
	mov	ax,word ptr DGROUP:_Cur+24
	mov	word ptr es:[bx],ax
   ;	
   ;	  *y=Cur.Cury;
   ;	
	les	bx,dword ptr [bp+10]
	mov	ax,word ptr DGROUP:_Cur+26
	mov	word ptr es:[bx],ax
   ;	
   ;	  if (i.m.Has) *but=MouseButs;
   ;	
	cmp	byte ptr DGROUP:_i+16,0
	je	short @7@198
	les	bx,dword ptr [bp+14]
	mov	ax,word ptr DGROUP:_MouseButs
	mov	word ptr es:[bx],ax
@7@198:
   ;	
   ;	  *but|=Jbuttons;
   ;	
	les	bx,dword ptr [bp+14]
	mov	ax,word ptr DGROUP:_Jbuttons
	or	word ptr es:[bx],ax
   ;	
   ;	  if (CurMode==TMODE)
   ;	
	cmp	word ptr DGROUP:_CurMode,3
	jne	short @7@254
   ;	
   ;	    {
   ;	    *x >>=2;
   ;	
	les	bx,dword ptr [bp+6]
	sar	word ptr es:[bx],2
   ;	
   ;	    *y >>=2;
   ;	
	les	bx,dword ptr [bp+10]
	sar	word ptr es:[bx],2
@7@254:
   ;	
   ;	    }
   ;	  return(TRUE);
   ;	
	mov	ax,1
	pop	bp
	ret	
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@moustats$qnit1t1	endp
   ;	
   ;	int moucurbox(int x,int y,int x1,int y1)   /* set box for mouse to move in */
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@moucurbox$qiiii	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  if (CurMode==GMODE) Cur.Limit(x,y,x1,y1);
   ;	
	cmp	word ptr DGROUP:_CurMode,19
	jne	short @8@86
	push	word ptr [bp+12]
	push	word ptr [bp+10]
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@Limit$qiiii
	add	sp,12
@8@86:
   ;	
   ;	  return(TRUE);
   ;	
	mov	ax,1
	pop	bp
	ret	
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@moucurbox$qiiii	endp
   ;	
   ;	void InitGraphics(DrawingBoard *d)
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@InitGraphics$qn12DrawingBoard	proc	far
	push	bp
	mov	bp,sp
   ;	
   ;	  {
   ;	  d->WindAddr= (void *) 0xA0000000;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+2],-24576
	mov	word ptr es:[bx],0
   ;	
   ;	  d->MaxX=320;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+4],320
   ;	
   ;	  d->MaxY=200;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+6],200
   ;	
   ;	  d->XSkip=320;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+8],320
   ;	
   ;	  d->Point  = Low256Point;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+12],seg @Low256Point$quiuiuin12DrawingBoard
	mov	word ptr es:[bx+10],offset @Low256Point$quiuiuin12DrawingBoard
   ;	
   ;	  d->GetCol = Low256GetCol;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+16],seg @Low256GetCol$quiuin12DrawingBoard
	mov	word ptr es:[bx+14],offset @Low256GetCol$quiuin12DrawingBoard
   ;	
   ;	  d->Rep1Col= Low256Rep1Col;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+20],seg _Low256Rep1Col
	mov	word ptr es:[bx+18],offset _Low256Rep1Col
   ;	
   ;	  d->RepCols= Low256RepCols;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+24],seg _Low256RepCols
	mov	word ptr es:[bx+22],offset _Low256RepCols
   ;	
   ;	  d->GetCols= Low256GetCols;
   ;	
	les	bx,dword ptr [bp+6]
	mov	word ptr es:[bx+28],seg _Low256GetCols
	mov	word ptr es:[bx+26],offset _Low256GetCols
   ;	
   ;	  GPFArrow.Get(GPFofArrow);
   ;	
	push	ds
	push	offset DGROUP:_GPFofArrow
	push	ds
	push	offset DGROUP:_GPFArrow
	call	far ptr @GPFClass@Get$qnuc
	add	sp,8
   ;	
   ;	  }
   ;	
	pop	bp
	ret	
@InitGraphics$qn12DrawingBoard	endp
   ;	
   ;	void mouclearbut(void)
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
@mouclearbut$qv	proc	far
	enter	6,0
   ;	
   ;	  {
   ;	  int x,y,but;
   ;	  
   ;	  if (i.m.Has) do { moustats(&x,&y,&but); } while(but);
   ;	
	cmp	byte ptr DGROUP:_i+16,0
	je	short @10@114
@10@58:
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
	jne	short @10@58
@10@114:
   ;	
   ;	  }
   ;	
	leave	
	ret	
@mouclearbut$qv	endp
DISP_TEXT	ends
_BSS	segment word public 'BSS'
_colors	label	word
	db	768 dup (?)
_BSS	ends
DISP_TEXT	segment byte public 'CODE'
   ;	
   ;	void main(void)
   ;	
	assume	cs:DISP_TEXT,ds:DGROUP
_main	proc	far
	enter	6,0
   ;	
   ;	  {
   ;	  int bgcol;
   ;	  FILE *fipo;
   ;	  if ((fipo=fopen("d:\\gmpro\\gmtitle.gif","rb"))==NULL)
   ;	
	push	ds
	push	offset DGROUP:s@+59
	push	ds
	push	offset DGROUP:s@+38
	call	far ptr _fopen
	add	sp,8
	mov	word ptr [bp-4],dx
	mov	word ptr [bp-6],ax
	or	ax,dx
	jne	short @11@114
   ;	
   ;	     { printf("File Not Found!\n"); return; }
   ;	
	push	ds
	push	offset DGROUP:s@+62
	call	far ptr _printf
	add	sp,4
	leave	
	ret	
@11@114:
   ;	
   ;	
   ;	  GraphMode();
   ;	
	call	far ptr @GraphMode$qv
   ;	
   ;	//  rewind(fipo);
   ;	  if ( (bgcol=intro(fipo,colors)) == -1) { fclose(fipo); return; }
   ;	
	push	ds
	push	offset DGROUP:_colors
	push	word ptr [bp-4]
	push	word ptr [bp-6]
	call	far ptr @intro$qn4FILEn7RGBdata
	add	sp,8
	mov	word ptr [bp-2],ax
	cmp	ax,-1
	jne	short @11@226
	push	word ptr [bp-4]
	push	word ptr [bp-6]
	call	far ptr _fclose
	add	sp,4
	leave	
	ret	
	jmp	short @11@254
@11@226:
   ;	
   ;	  else
   ;	    {
   ;	    GraphMode();
   ;	
	call	far ptr @GraphMode$qv
   ;	
   ;	
   ;	    SetAllPal(colors);
   ;	
	push	ds
	push	offset DGROUP:_colors
	call	far ptr _SetAllPal
	add	sp,4
   ;	
   ;	//    setscrn(bgcol);
   ;	    decode(fipo,1);
   ;	
	push	1
	push	word ptr [bp-4]
	push	word ptr [bp-6]
	call	far ptr @decode$qn4FILEi
	add	sp,6
@11@254:
   ;	
   ;	//    FadeTo(colors);
   ;	   // redo=getthem();    // this returns a 1 if returning from help,etc.
   ;	    }                  // this was it acts as a redraw screen.
   ;	  fclose(fipo);
   ;	
	push	word ptr [bp-4]
	push	word ptr [bp-6]
	call	far ptr _fclose
	add	sp,4
   ;	
   ;	  TextMode();
   ;	
	call	far ptr @TextMode$qv
   ;	
   ;	  }
   ;	
	leave	
	ret	
_main	endp
DISP_TEXT	ends
_INIT_	segment word public 'INITDATA'
	db	1
	db	32
	dd	@_STCON_$qv
_INIT_	ends
DISP_TEXT	segment byte public 'CODE'
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
_$A	label	word
	dd	@@$xt$10TimerClass
	db	0
	db	0
	dd	DGROUP:_Time
	dd	@@$xt$10InputClass
	db	0
	db	0
	dd	DGROUP:_i
	dd	@@$xt$11CursorClass
	db	0
	db	0
	dd	DGROUP:_Cur
	db	0
	db	0
	db	0
	db	0
_$B	label	word
	db	0
	db	0
	db	0
	db	0
	db	226
	db	255
	db	0
	db	0
	db	5
	db	0
	db	0
	db	0
	dw	_$A
	assume	cs:DISP_TEXT,ds:DGROUP
@_STCON_$qv	proc	far
	enter	30,0
	mov	ax,DISP_TEXT
	mov	bx,offset _$B
	call	far ptr ___InitExceptBlock
	mov	word ptr [bp-18],6
	push	ds
	push	offset DGROUP:_Time
	call	far ptr @LinkedListClass@$bctr$qv
	add	sp,4
	add	word ptr ss:[16],1
	adc	word ptr ss:[18],0
	push	ds
	push	offset DGROUP:_i+12
	call	far ptr @LinkedListClass@$bctr$qv
	add	sp,4
	add	word ptr ss:[16],1
	adc	word ptr ss:[18],0
	push	ds
	push	offset DGROUP:_i+36
	call	far ptr @LinkedListClass@$bctr$qv
	add	sp,4
	add	word ptr ss:[16],1
	adc	word ptr ss:[18],0
	add	word ptr ss:[16],1
	adc	word ptr ss:[18],0
	mov	word ptr DGROUP:_GPFArrow+2,0
	mov	word ptr DGROUP:_GPFArrow,0
	push	ds
	push	offset DGROUP:_Scrn
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@$bctr$qn12DrawingBoard
	add	sp,8
	mov	ax,word ptr [bp-30]
	mov	word ptr ss:[20],ax
	leave	
	ret	
@_STCON_$qv	endp
DISP_TEXT	ends
_EXIT_	segment word public 'EXITDATA'
	db	1
	db	32
	dd	@_STDES_$qv
_EXIT_	ends
DISP_TEXT	segment byte public 'CODE'
	assume	cs:DISP_TEXT,ds:DGROUP
@_STDES_$qv	proc	far
	push	bp
	mov	bp,sp
	push	2
	push	ds
	push	offset DGROUP:_Cur
	call	far ptr @CursorClass@$bdtr$qv
	add	sp,6
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	push	2
	push	ds
	push	offset DGROUP:_i+36
	call	far ptr @LinkedListClass@$bdtr$qv
	add	sp,6
	push	2
	push	ds
	push	offset DGROUP:_i
	call	far ptr @MouseClass@$bdtr$qv
	add	sp,6
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	push	0
	push	ds
	push	offset DGROUP:_Time
	call	far ptr @LinkedListClass@$bdtr$qv
	add	sp,6
	pop	bp
	ret	
@_STDES_$qv	endp
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@$xt$11CursorClass	segment	virtual
@@$xt$11CursorClass	label	word
	db	40
	db	0
	db	3
	db	0
	db	38
	db	0
	db	255
	db	255
	db	3
	db	0
	db	50
	db	0
	db	54
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	2
	db	0
	db	2
	db	0
	dd	@CursorClass@$bdtr$qv
	db	5
	db	0
	db	58
	db	0
	db	67
	db	117
	db	114
	db	115
	db	111
	db	114
	db	67
	db	108
	db	97
	db	115
	db	115
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	dd	@@$xt$15LinkedListClass
	db	12
	db	0
	db	0
	db	0
	db	0
	db	0
@$xt$11CursorClass	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@$xt$10InputClass	segment	virtual
@@$xt$10InputClass	label	word
	db	41
	db	0
	db	3
	db	0
	db	38
	db	0
	db	255
	db	255
	db	3
	db	0
	db	50
	db	0
	db	54
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	5
	db	0
	db	5
	db	0
	dd	@@InputClass@$bdtr$qv
	db	5
	db	0
	db	58
	db	0
	db	73
	db	110
	db	112
	db	117
	db	116
	db	67
	db	108
	db	97
	db	115
	db	115
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	dd	@@$xt$10MouseClass
	db	0
	db	0
	dd	@@$xt$13JoyStickClass
	db	17
	db	0
	db	0
	db	0
	db	0
	db	0
@$xt$10InputClass	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@$xt$10TimerClass	segment	virtual
@@$xt$10TimerClass	label	word
	db	9
	db	0
	db	3
	db	0
	db	38
	db	0
	db	255
	db	255
	db	7
	db	0
	db	50
	db	0
	db	62
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	2
	db	0
	db	2
	db	0
	dd	@@TimerClass@$bdtr$qv
	db	5
	db	0
	db	66
	db	0
	db	84
	db	105
	db	109
	db	101
	db	114
	db	67
	db	108
	db	97
	db	115
	db	115
	db	0
	db	0
	dd	@@$xt$15LinkedListClass
	db	0
	db	0
	db	3
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
@$xt$10TimerClass	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@InputClass@$bdtr$qv	segment	virtual
	assume	cs:@InputClass@$bdtr$qv,ds:DGROUP
@@InputClass@$bdtr$qv	proc	far
	push	bp
	mov	bp,sp
	push	si
	mov	si,word ptr [bp+10]
	mov	ax,word ptr [bp+6]
	or	ax,word ptr [bp+8]
	je	short @14@142
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	push	2
	mov	ax,word ptr [bp+6]
	add	ax,36
	push	word ptr [bp+8]
	push	ax
	call	far ptr @LinkedListClass@$bdtr$qv
	add	sp,6
	push	2
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	call	far ptr @MouseClass@$bdtr$qv
	add	sp,6
	test	si,1
	je	short @14@114
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	call	far ptr @$bdele$qnv
	add	sp,4
	jmp	short @14@114
@14@114:
	jmp	short @14@170
@14@142:
	xor	ax,ax
@14@170:
	jmp	short @14@198
@14@198:
	pop	si
	pop	bp
	ret	
@@InputClass@$bdtr$qv	endp
@InputClass@$bdtr$qv	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@TimerClass@$bdtr$qv	segment	virtual
	assume	cs:@TimerClass@$bdtr$qv,ds:DGROUP
@@TimerClass@$bdtr$qv	proc	far
	push	bp
	mov	bp,sp
	push	si
	mov	si,word ptr [bp+10]
	mov	ax,word ptr [bp+6]
	or	ax,word ptr [bp+8]
	je	short @15@142
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	push	0
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	call	far ptr @LinkedListClass@$bdtr$qv
	add	sp,6
	test	si,1
	je	short @15@114
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	call	far ptr @$bdele$qnv
	add	sp,4
	jmp	short @15@114
@15@114:
	jmp	short @15@170
@15@142:
	xor	ax,ax
@15@170:
	jmp	short @15@198
@15@198:
	pop	si
	pop	bp
	ret	
@@TimerClass@$bdtr$qv	endp
@TimerClass@$bdtr$qv	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@$xt$13JoyStickClass	segment	virtual
@@$xt$13JoyStickClass	label	word
	db	24
	db	0
	db	3
	db	0
	db	38
	db	0
	db	255
	db	255
	db	3
	db	0
	db	52
	db	0
	db	56
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	2
	db	0
	db	2
	db	0
	dd	@@JoyStickClass@$bdtr$qv
	db	5
	db	0
	db	60
	db	0
	db	74
	db	111
	db	121
	db	83
	db	116
	db	105
	db	99
	db	107
	db	67
	db	108
	db	97
	db	115
	db	115
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	dd	@@$xt$15LinkedListClass
	db	19
	db	0
	db	0
	db	0
	db	0
	db	0
@$xt$13JoyStickClass	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@$xt$10MouseClass	segment	virtual
@@$xt$10MouseClass	label	word
	db	17
	db	0
	db	3
	db	0
	db	38
	db	0
	db	255
	db	255
	db	3
	db	0
	db	50
	db	0
	db	54
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	2
	db	0
	db	2
	db	0
	dd	@MouseClass@$bdtr$qv
	db	5
	db	0
	db	58
	db	0
	db	77
	db	111
	db	117
	db	115
	db	101
	db	67
	db	108
	db	97
	db	115
	db	115
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	dd	@@$xt$15LinkedListClass
	db	12
	db	0
	db	0
	db	0
	db	0
	db	0
@$xt$10MouseClass	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@$xt$15LinkedListClass	segment	virtual
@@$xt$15LinkedListClass	label	word
	db	4
	db	0
	db	3
	db	0
	db	38
	db	0
	db	255
	db	255
	db	3
	db	0
	db	54
	db	0
	db	58
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	1
	db	0
	db	1
	db	0
	dd	@LinkedListClass@$bdtr$qv
	db	5
	db	0
	db	62
	db	0
	db	76
	db	105
	db	110
	db	107
	db	101
	db	100
	db	76
	db	105
	db	115
	db	116
	db	67
	db	108
	db	97
	db	115
	db	115
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
	db	0
@$xt$15LinkedListClass	ends
DISP_TEXT	ends
DISP_TEXT	segment byte public 'CODE'
@JoyStickClass@$bdtr$qv	segment	virtual
	assume	cs:@JoyStickClass@$bdtr$qv,ds:DGROUP
@@JoyStickClass@$bdtr$qv	proc	far
	push	bp
	mov	bp,sp
	push	si
	mov	si,word ptr [bp+10]
	mov	ax,word ptr [bp+6]
	or	ax,word ptr [bp+8]
	je	short @16@142
	sub	word ptr ss:[16],1
	sbb	word ptr ss:[18],0
	push	2
	mov	ax,word ptr [bp+6]
	add	ax,19
	push	word ptr [bp+8]
	push	ax
	call	far ptr @LinkedListClass@$bdtr$qv
	add	sp,6
	test	si,1
	je	short @16@114
	push	word ptr [bp+8]
	push	word ptr [bp+6]
	call	far ptr @$bdele$qnv
	add	sp,4
	jmp	short @16@114
@16@114:
	jmp	short @16@170
@16@142:
	xor	ax,ax
@16@170:
	jmp	short @16@198
@16@198:
	pop	si
	pop	bp
	ret	
@@JoyStickClass@$bdtr$qv	endp
	?debug	C E9
	?debug	C FA00000000
@JoyStickClass@$bdtr$qv	ends
DISP_TEXT	ends
_DATA	segment word public 'DATA'
s@	label	byte
	db	'GM:Not Enough Memory! (new operator)'
	db	10
	db	0
	db	'd:\gmpro\gmtitle.gif'
	db	0
	db	'rb'
	db	0
	db	'File Not Found!'
	db	10
	db	0
_DATA	ends
DISP_TEXT	segment byte public 'CODE'
DISP_TEXT	ends
_s@	equ	s@
	extrn	@$bdele$qnv:far
	extrn	___InitExceptBlock:far
	extrn	_exit:far
	extrn	_fclose:far
	extrn	_fopen:far
	extrn	_printf:far
_peek	equ	peek
_peekb	equ	peekb
_poke	equ	poke
_pokeb	equ	pokeb
_abs	equ	abs
_atoi	equ	atoi
	extrn	@GraphMode$qv:far
	extrn	@TextMode$qv:far
	public	@NewBombOut$qv
	extrn	_SetAllPal:far
	public	@initmouse$qv
	public	@moucur$qi
	public	@moustats$qnit1t1
	public	@setmoupos$qii
	public	@moucurbox$qiiii
	public	@mouclearbut$qv
	extrn	@decode$qn4FILEi:far
	extrn	@intro$qn4FILEn7RGBdata:far
	public	@KeyUpdate$qi
	extrn	_CurMode:word
	extrn	__new_handler:dword
	extrn	@LinkedListClass@$bctr$qv:far
	extrn	@LinkedListClass@$bdtr$qv:far
	extrn	@LinkedListClass@Traverse$qnqn9NodeClass$zcn9NodeClass:far
	extrn	@CallEm$qn9NodeClass:far
	extrn	_Clock:word
	extrn	@TimerClass@TurnOn$qv:far
	public	_Time
	extrn	@Low256Point$quiuiuin12DrawingBoard:far
	extrn	@Low256GetCol$quiuin12DrawingBoard:far
	extrn	_Low256Rep1Col:far
	extrn	_Low256RepCols:far
	extrn	_Low256GetCols:far
	extrn	@GPFClass@Get$qnuc:far
	extrn	@CursorClass@$bctr$qn12DrawingBoard:far
	extrn	@CursorClass@$bdtr$qv:far
	extrn	@CursorClass@SetHotSpot$qii:far
	extrn	@CursorClass@NewGPF$qn8GPFClass:far
	extrn	@CursorClass@On$qv:far
	extrn	@CursorClass@Off$qv:far
	extrn	@CursorClass@Goto$qii:far
	extrn	@CursorClass@Limit$qiiii:far
	extrn	@JoyStickClass@AddCursor$qn11CursorClass:far
	extrn	@MouseClass@Change$qv:far
	extrn	@MouseClass@$bdtr$qv:far
	public	@MouseMoved$qv
	extrn	@MouseClass@AddCursor$qn11CursorClass:far
	extrn	@InputClass@Init$qv:far
	extrn	_GPFofArrow:byte
	public	_mouinstall
	extrn	_MouseButs:word
	public	_Scrn
	public	_i
	public	_GPFArrow
	public	_Cur
	public	_Jbuttons
	public	@InitGraphics$qn12DrawingBoard
	public	_colors
	public	_main
	end
