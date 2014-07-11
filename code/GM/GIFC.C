// gifc.c  - Miscellaneous .gif routines
//#pragma inline
#include<stdio.h>
#include<dos.h>
#include<bios.h>
#include<string.h>
#include"gmgen.h"
#include"palette.h"
#include"windio.h"
#include"mousefn.h"
#include"scrnrout.h"
#include"graph.h"

static void readcols(FILE *fp,RGBdata colors[256]);    /* reads palette from file */
static void initable(void);            /* initializes string table */
static void initchop(void);            /* initializes table of _chop values(see below) */
static void datatrans(unsigned char from[],unsigned int *to,int *bits);
       /* datatrans returns the next code from the characters read */
static int interpret(int coded,int skip);   /* changes code to char string */
static int usedata(int size,int skip);      /* puts pixels on screen */
static void extension(FILE *gif);                                                                 
void decode(FILE *gif,int skip);            /* main program.  draws gif */
int intro(FILE *gif,RGBdata *colors);       /* interprets header data, returns background color */
int drawbkd(char *bkdname,RGBdata *colors); /* draws gifs from bkd files */
int DrawGif(char *fname,RGBdata *colors);

typedef struct
 {
  int code;               /* position of next entry in chain */
  unsigned char next;     /* character to add to string */
  unsigned char first;    /* character used to get tab.next for next code */
 } table;

static table    tab[4097];        /*  STRING TABLE!  */
static unsigned char outcol[600]; /* contains string of output characters */
static int      lastcode,oldcode; /* place holder for tab[],int for next tab[] entry */
static unsigned int chop[17];     /* contains bits which make an int smaller if &ed */
static int      x,y;
static char codestart,codesize;   /* nextlimit--increase codestart if lastcode is */
static int  nextlimit;            /*   greater. codestart--# of bits per code     */
static char interlaced;           /* Is file interlaced? */
static unsigned int numcol;
int xstart=0,ystart=0;            /* x,y current and first coords of point being drawn */
int imagex=320,imagey=200;        /* dimentions of image. codesize-start val. of codestart */
int scrx=0,scrx1=320,scry=0,scry1=200;
extern RGBdata Black;

extern char  WorkDir[];

/*  xstart and ystart are the coordinates of the image to start drawing at.
   if these are greator than 0, the toppart of the picture will be cut off.
   The first coordinate is drawn at scrx,scry, and the last at scrx1,scry1. 
   So to cut off the bottom or left of a picture change scrx1,scry1.
   imagex and imagey are the uncut dimentions of the gif image.
   x and y are the current coordinates in the GIF image.
   GIF coordinate system: x,y,xstart,ystart
   Screen coordinate system: scrx,scrx1,scry,scry1   */

  
#pragma loop_opt(off)

int intro(FILE *gif,RGBdata *colors)
  {
  register int i;
  int flag=0;                         /* general flag */
  unsigned char flagbt,bkcol;         /* flag byte, background color */
  char header[8] = { 0,0,0,0,0,0,0 }; /* will contain GIF file header */

  if (gif==NULL)  // get data from header of file
    {
    errorbox("No graphics file was indicated!","(R)eturn to Menu");
    return(-1);
    }
  flag=0;
  fgets(header,7,gif);
  if (header[0] != 'G')
    {
    errorbox("This file is not in GIF format!","(R)eturn to Menu");
    return(-1);
    }
  fread(&imagex,2,1,gif); // screen dim.'s,unimportant(not image size)
  fread(&imagey,2,1,gif);
  flagbt = fgetc(gif);                  //  flag byte
  bkcol  = fgetc(gif);                  //  background color
  fgetc(gif);                           //  zero byte
  numcol = 2 << (flagbt & 0x07);
  if ( flagbt & 0x80 )
    readcols(gif,colors);
  
  while ((flag=fgetc(gif))!=',')
    {
    if (flag=='!') extension(gif);
    else
      {
      errorbox("There is no image in this file!","(R)eturn to Menu");
      return(-1);
      }
    }
  fgetc(gif); fgetc(gif); fgetc(gif); fgetc(gif); //  top and left coord's.  I always use 0
  fread(&imagex,2,1,gif);                         //  image coord's
  fread(&imagey,2,1,gif);
  imagex--; imagey--;          // an image row goes from 0 to imagex
  flagbt = fgetc(gif);         //  flag byte
  interlaced = (flagbt & 0x40);
  if (flagbt & 0x80) {
    numcol = 2 << (flagbt & 0x07);
    readcols(gif,colors);
   }
  codesize = fgetc(gif); // # of bits to read for _trans after initializing table
  return(bkcol);
  }

void decode(FILE *gif,int skip)
  {
  unsigned register int i;           /* for loops                           */
  int place=0;                       /* place-bits left unused              */
  unsigned int trans = 0;            /* code value                          */ 
  unsigned char work[2] = { 0,0 };   /* char read, to be translated to code */
  unsigned char jumper = 0;          /* length of packet                    */
  int done=0;

  i=0;  x=0;  y=0;

  initable();
  initchop();
  do
    {
    if (place<codestart)
      {
      while (i==0) 
        if (!feof(gif)) i = (jumper=fgetc(gif));
        else { i= -1; jumper=0; }
      if (!feof(gif)) { work[1]= fgetc(gif);  i--; } else jumper=0;
      } else place-=8;
    datatrans(work,&trans,&place);
    if (place<0)
      {
      while (i==0)
        {
        if (!feof(gif)) i = (jumper=fgetc(gif));
        else { i= -1; jumper=0; }
        }
      if (!feof(gif)) { work[0] = fgetc(gif);  i--; } else jumper=0;
      trans|=( (work[0]&chop[(-place)]) << (place+codestart) );
      work[0]= (work[0]>>(-place))&chop[8+place];
      work[1]=0;
      place+=8; 
      }
    if (trans==numcol) initable();
    else
      {
      if (trans==numcol+1) jumper=0;    /* End of Image! */
      else
        {
        if (tab[numcol+2].code==4097)   /* First code received */
          {
          tab[numcol+2].code=4096;
          outcol[1]=trans;
          usedata(1,skip);
          }
        else (done=interpret(trans,skip));  /* Draw pattern associated */
        oldcode=trans;                      /*        with code.       */ 
        }
      }
    trans=0;
    } while ( (jumper!=0)&&(done==0) );
  }

void extension(FILE *gif)
  {
  char jumper=-1;
  register int j;

  fgetc(gif);           // throw away this byte
  while(jumper)
    {
    jumper = fgetc(gif);
    for (j=0;j<jumper;j++)
      fgetc(gif);
    }
  }

void readcols(FILE *fp,RGBdata *colors)
  {
  register int j;
  int datain;

  datain=fread(colors,sizeof(RGBdata),numcol,fp);
  for (j=0;j<datain;j++)
    {
    colors[j].red   >>= 2;  // Shift right 2 is divide by 4.
    colors[j].green >>= 2;  // This makes color values go
    colors[j].blue  >>= 2;  // between 0 and 63, not 0 and 256
    }

  for (j=datain;j<256;j++)  // Zero the rest of the colors
    insertcol(j,0,0,0,colors);
  }
  
#pragma loop_opt(on)

void datatrans(unsigned char from[],unsigned int *to,int *bits)
  {
  unsigned int temp;
  temp = (from[0] | (from[1] << *bits));
  *to = temp & chop[codestart];
  from[0]=(temp>>codestart);
  from[1]=(temp>>(8+codestart) ); 
  *bits+=(8-codestart);
  }
  
#pragma loop_opt(off)

int interpret(int coded,int skip)
  {
  int i=0;
  int properone;

  if (tab[coded].code==4096)
    properone=oldcode;
  else properone = coded;
  tab[lastcode].next=tab[properone].first;
  tab[lastcode].code=oldcode;
  tab[lastcode].first=tab[oldcode].first;
  lastcode++;
  if ( (lastcode==nextlimit)&&(codestart<12) )
    {
    codestart++;
    nextlimit*=2;
    }
  while(coded>numcol)
    {
    outcol[++i]=tab[coded].next;
    coded=tab[coded].code;
    }
  outcol[++i]=coded;
  if (i>=600) printf("Code Too LARGE, memory error 1");
  return(usedata(i,skip));
  }

int usedata(int size,int skip)   // draw one every 'skip' pixels when sent
  {                              // size of them in outcol
  register int i;
  int sx,sy;
  int done=0;
 
  for(i=size;i>0;i--)
    {
    sx = (x-xstart)/skip;  sy = (y-ystart)/skip;
    if ( (x>=xstart)&&(sx<scrx1-scrx)&&((x%skip)==0)&&
         ((y%skip)==0)&&(y>=ystart)&&(sy<scry1-scry) )
      Point(scrx+sx,scry+sy,outcol[i]);
    if (x<imagex) x++;
    else
      {
      x=0;
      if (!interlaced) { y++; if ((y-ystart)/skip > scry1-scry) done=1; }
      else
        {
        switch(y%8) {
          case 0:   y+=8; if (y>=imagey) y=4; break;
          case 4:   y+=8; if (y>=imagey) y=2; break;
          case 2:
          case 6:   if (skip==4) done=1;
                    y+=4; if (y>=imagey) y=1; break;
          default:  y+=2;
          }
        }
      }
    }
  return(done);
  }

void initable()
  {
  register int j;

  for (j=numcol;j<4096;j++)
    {
    tab[j].code=4096;                       /* a code of LASTCOL signifies */
    tab[j].next=0;                          /* a reinitialization of the   */
    tab[j].first=0;                         /* table (clear code)      .   */ 
    }                         /* code LASTCOL+1 is an end of image code.    */
  for (j=0;j<numcol;j++)
    {
    tab[j].code=0;
    tab[j].next=j;
    tab[j].first=j;
    }
  tab[numcol+2].code=4097;
  lastcode=numcol+2;
  codestart=codesize+1;
  nextlimit=numcol*2;
  }

void initchop(void)
 {
  register int j;

  for(j=0;j<17;j++)
    {
    chop[j]=0;
    chop[j]=(1<<j)-1;
    }
  }
  
int drawbkd(char *bkdname,RGBdata *colors)
  {
  register int i;
  char str[MAXFILENAMELEN];
  char error[81];
  FILE *fipo;

  if ((fipo=fopen(bkdname,"rb"))==NULL)
    { 
    TextMode(); 
    sprintf(error,"Error Opening Bkd %s!",bkdname);
    errorbox(error,"(C)ontinue");
    return(FALSE);
    }
  fread(&xstart,2,1,fipo);
  fread(&ystart,2,1,fipo);
  fgets(str,40,fipo);
  for(i=0;((i<MAXFILENAMELEN)&&(str[i]!=10));i++);  // Find the end of the gif name.
  str[i]=0;                      // Zero terminate the name.

  fclose(fipo);

  strcpy(error,str);
  MakeFileName(str,WorkDir,error,"");
  if ((fipo=fopen(str,"rb"))==NULL)
    {
    TextMode(); 
    sprintf(error,"Error Opening Gif %s!",str);
    errorbox(error,"(C)ontinue");
    return(FALSE);
    }
  scrx=0;  scry=0;  scrx1=320; scry1=200;

  if (intro(fipo,colors) == -1)
    { 
    fclose(fipo); 
    TextMode();
    errorbox("Bad GIF File","(C)ontinue");
    return(FALSE);
    }

  GraphMode();
  SetAllPalTo(&Black);
  decode(fipo,1);
  fclose(fipo);
  FadeTo(colors);
  return(TRUE);
  }

#pragma loop_opt(on)

int DrawGif(char *fname,RGBdata *colors)
  {
  FILE *fipo;

  if ((fipo=fopen(fname,"rb"))==NULL) return(FALSE);

  if (intro(fipo,colors) == -1) 
    { 
    fclose(fipo); 
    return(FALSE);
    }
  xstart=0;ystart=0;
  scrx=0;  scry=0;  scrx1=320; scry1=200;
  decode(fipo,1);
  fclose(fipo);
  return(TRUE);
  }
