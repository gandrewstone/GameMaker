/*  Windowing Input routines for use in programs */
/*  must have mousefn.c  and scrnrout.obj        */
/*  Last Edited: 3/3/91                          */
/*  By Andy Stone                                */

#include <alloc.h>
#include <dir.h>
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>  // Gotoxy
#include <time.h>
#include <ctype.h>

#include "gmgen.h"
//#include "windio.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "jstick.h"

#define DBGPRT(x) printf(x)


char interrupt far NewCritError(...);

int  getfname(int x,int y,const char *q,const char *fspec,char *ans);
  /* stick default path in ans */
void closemenu(int x,int y,int len,int wid,char *buffer);
int openmenu(int x,int y,int lenx,int widy,char *w);
     // returns the window's background attribute (already multiplied)

int errorbox(char *l,char *l1,unsigned int maxpause=30000);
     // returns the character selected.

int horizmenu(int numitems, int curitem, int y,...); /* ... is x of each item and final x of end */
int vertmenu(int x,int y,int lenOFitems,int numitems, int curitem);
     // returns the item number selected, first item is # 1
int qwindow(int x,int y,int len, char *q,char *ans);
     // question window - returns answer in 'ans'.
     // Returns TRUE if question answered, FALSE if user hit ESC.

#define REMNAME 1   // remember file name in case saved
int loadany(char *prompt,char *ext,char *path,unsigned int bytes,char *buffer,int cansave);
int saveany(char *prompt,char *ext,char *path,unsigned int bytes,char *buffer);
      // returns TRUE - load successful, or FALSE - load unsuccessful
int delany(char *prompt,char *ext,char *path);

int getstr(int x,int y, int maxlen, char *ans);

#define FOREATTR    14
#define BACKATTR    1
#define MENUATTR    (FOREATTR+(BACKATTR*16))
#define MENUBOX     2
#define SHADATTR    7*16
#define ERRORFORE   12
#define ERRORBACK   4
#define ERRORATTR   (ERRORFORE+(ERRORBACK*16))
//#define CURATTR     (7*16)+15
#define CURATTR     PGMTITLECOL
#define BASE        8

// End windio.c header file

#define LASTFILE   150

#ifdef USEJOYSTICK
extern int joyinstall;
extern joystruct js;
#endif

extern int mouinstall;
extern int MXlatx,MXlaty;
static int butstat     = 0;
char       curfile[31] = "";
int        windlvl     = 0;
char       CritVal     = 100;

char interrupt far NewCritError(...);
static char interrupt far (*OldCritError) (...);
static int  del(char *file,char *path);
static void drawcursor(int,int,int,int);
static void erasecursor(void);
int FindFiles(const char *filespec,const char *path,char *result,uint max,uint *Idx);
int FindDirs(const char *dirspec,const char *path,char *result,uint max,uint *Idx);

#pragma loop_opt(off)

static char Spaces[31]="                              ";

int GetChoices(const char *filespec,const char *path,char *result,uint max)
  {
  const int FNAMELEN = 14;
  int   NumFiles     =  1;
  uint  Idx          =  0;

  for (uint i=0;i<FNAMELEN;i++) result[i]=0;  // Give an empty slot.
  result+=FNAMELEN;  // Skip the empty slot.
  max   -=FNAMELEN;

  NumFiles+=FindDirs("*.*",path,result,max,&Idx);
  result+=Idx; max-=Idx;
  while (*filespec != 0)  // Get all the possible file extensions 1 at a time.
    {
    NumFiles+=FindFiles(filespec,path,result,max,&Idx);
    result+=Idx; max-=Idx;
    while (*filespec != 0) filespec++;
    filespec++;
    }

  return(NumFiles);
  }


int  getfname(int x,int y,const char *q,const char *fspec,char *ans)
  {
  const int FNAMELEN  = 14;
  const int CURSORCOL = CURATTR;
  // ends in q -- pertains to question box
  // ends in f -- pertains to filename pick box
  boolean NewDir=False;     // Set to true if we need to load up a new directory list.
  int  l1,l;                // looping variables and temporary vars.
  int  attrq,attrf;         // question and answer screen attributes
  char *wq;                 // stores data behind openmenu of question
  char *wf;                 //    "    "     "      "       "  files
  char *fnames;             // list of file names
  uint EmptyArea;
  int  maxfname = LASTFILE; // maximum number of file names to be read
  int  lenq     = 0;        // length of question
  int  maxa     = 30; // MAXFILENAMELEN; // max len. of answer
  int  widf     = 0;        // width (y dir) of filename pick box
  int  done     = -1;
  int  topf     = 0;        // array index which is first on the list
  int  numf     = 0;        // currently selected array number
  int  maxf     = 0;        // total # of file names in array
  int  numq     = 0;        // length of current answer (file+path)
  int  dirq     = 0;        // length of path of current answer
  int  xf;                  // left side of file list
  int  fchange  = 0;
  int  inkey,upkey;
  int  mmove=FALSE;
  int  mx,my,mbuts,oy;
  FILE *fp;

  lenq=strlen(q)+2;
  if (ans[0]==0) strcpy(ans,WorkDir);
  dirq=numq=strlen(ans);

  if (lenq+x+maxa>76)       // Handle screen overruns by moving the window,
    {                       // and shortening the max answer length.
    x    = 76-lenq-maxa;
    if (x<1) x=1;
    maxa = 76-lenq-x;
    if (maxa<1) return(False);
    }
  if (numq>maxa) { dirq = numq = maxa; ans[numq]=0; }

  if ((y>20)||(y<1)||(x<1)) return(FALSE);

  wq = new char [2000];
  wf = new char [2000];

  while ( ((fnames = new char [FNAMELEN*maxfname])==NULL)&&(maxfname>1)) maxfname--;
  if ((maxfname<2)||(wf == NULL))
    {
    errorbox("Out Of Memory!","(E)xit.");
    if (wq)     delete wq;
    if (wf)     delete wf;
    if (fnames) delete fnames;
    return(FALSE);
    }
  mouclearbut();
  widf=21-y;

  attrq=openmenu(x,y,lenq+maxa+1,1,wq);
  writestr(x+1,y,(attrq+14)&255,q);
  writestr(x+lenq,y,attrq+15,ans);


  maxf=GetChoices(fspec,ans,fnames,maxfname*FNAMELEN);
  if (maxf<widf) widf=maxf;
  if (dirq+10>maxa) xf=(x+lenq+maxa-10);
  else              xf=(x+lenq+dirq);
  attrf=openmenu(xf-1,y+2,15,widf,wf);
  writestr(xf-2,y+1,attrf+14,"» ÄÄÄÄÄÄÄÄÄÄÄÄÄ É");
  for (l=0;l<widf;l++)                 // put file names in box
    if (l<maxf)                        // don't need to consider topf yet
      {                                // because it must be zero when fn starts
      writech(xf-1,y+2+l,attrf+11,' ');
      for (l1=0;l1<FNAMELEN-1;l1++)
        if (fnames[(FNAMELEN*l)+l1]!=0) writech(xf+l1,y+2+l,attrf+11,fnames[(FNAMELEN*l)+l1]);
        else while (l1<FNAMELEN-1) { writech(xf+l1,y+2+l,attrf+11,' '); l1++; }
      }
  my=oy=y+2;
  mx=x+lenq;
  if (mouinstall) setmoupos(mx,my);
   
  do
    {
    if (fchange==1)
      {
      for (l=0;l<widf;l++)                          // put file names in box
        if (l+topf<maxf)
          {
          writech(xf-1,y+2+l,attrf+11,' ');
          for (l1=0;l1<FNAMELEN-1;l1++)
            if (fnames[(FNAMELEN*(l+topf))+l1]!=0) writech(xf+l1,y+2+l,attrf+11,fnames[(FNAMELEN*(l+topf))+l1]);
            else while (l1<FNAMELEN-1) { writech(xf+l1,y+2+l,attrf+11,' '); l1++; }
          }
      }
    fchange=0;
    drawcursor(xf-1,y+2+(numf-topf),xf+FNAMELEN-1,CURSORCOL);
    gotoxy(x+lenq+numq+1,y+1);
    do
      {
      if (mouinstall)
        {
        moustats(&mx,&my,&mbuts);
        if ((mbuts>0)||(my!=oy)) mmove=TRUE;
        }
      } while((!bioskey(1))&&(!mmove));

    if (bioskey(1)) 
      {
      inkey=bioskey(0);
      upkey= (inkey>>8)&255;
      switch(inkey&255)
        {
        case 0:
          switch(upkey)
            {
            case 72:                  // UP
              if (numf>0)
                {
                numf--;
                if (numf<topf) { topf--; fchange=1; }
                writestr(x+lenq,y,attrq+15,Spaces);
                strcpy(&(ans[dirq]),fnames+(numf*FNAMELEN));
                writestr(x+lenq,y,attrq+15,ans);
                numq=strlen(ans);
                my=(oy=y+2+numf-topf);
                setmoupos(xf,oy);
                }
              break;
            case 80:                  // DOWN
              if (numf<maxf-1)
                {
                numf++;
                if (numf>topf+widf-1) {topf++; fchange=1; }
                writestr(x+lenq,y,attrq+15,Spaces);
                strcpy(&(ans[dirq]),fnames+(numf*FNAMELEN));
                writestr(x+lenq,y,attrq+15,ans);
                numq=strlen(ans);
                my=(oy=y+2+numf-topf);
                setmoupos(xf,oy);
                }
              break;
            }
          break;
        case 27:
          done=0;
          break;
        case 13:
          mbuts=1;
          mmove=TRUE;     // Deal with accepted name in mouse section
          break;
        case 8:           // Rub out key hit--erase char on screen only
          if (numq<=0) break;
          numq--;
          if ( (ans[numq]!='\\')||(numq==0) )
            {
            writech(x+lenq+numq,y,256,' ');
            ans[numq]=0;
            break;                          // in most cases, break here
            }
          else do {                         // Else erase whole dir segment
            writech(x+lenq+numq,y,256,' ');
            ans[numq]=0;
            numq--;
            } while ( (numq>0)&&(ans[numq]!='\\') );
          if (numq==0) NewDir=True;
          if (ans[numq]!='\\')
            {
            ans[numq]=0;
            writech(x+lenq+numq,y,256,' ');
            break;                          // Break if at beginning else
            }                               // No break statement so will act
          else inkey='\\';                  // as if backslash was just typed
        default:
          if (numq<maxa) 
            {
            ans[numq]=(inkey&255);
            writech(x+lenq+numq,y,attrq+15,ans[numq]);
            if (ans[numq] == 92) NewDir=True; // backslash (92) typed--load another dir
            numq++;
            ans[numq]=0;
            }
        break;
        }
      }

    if (mmove)  // Mouse handling routine.  Won't happen unless mouinstall=TRUE
      {
      mmove=FALSE;
      if (mbuts)
        {
        ans[numq]=0;
        if (numq==dirq) { done=1; }
        else if (ans[numq-1]=='\\') NewDir=True;
        else if ((fp=fopen(ans,"rb"))==NULL)
          {
          inkey=errorbox("Sorry, the file does not exist!","(R)eenter name (E)xit");
          inkey&=255;
          if ((inkey=='E')|(inkey=='e')) done=0; 
          ans[numq]=0;
          inkey=0;
          oy=(y+2+numf-topf);
          setmoupos(xf,oy);
          }
        else { fclose (fp); done=numq; }
        }
      else  // Write the string up in the entry box.
        {
        if (my<y+2)      { my=y+2;      setmoupos(mx,my); } // Limit the mouse
        if (my>y+1+widf) { my=y+1+widf; setmoupos(mx,my); }

        numf=topf+(my-(y+2));
        oy=my;
        writestr(x+lenq,y,attrq+15,Spaces);
        strcpy(&(ans[dirq]),fnames+(numf*FNAMELEN));
        writestr(x+lenq,y,attrq+15,ans);
        numq=strlen(ans);
        if ((my==y+2)&&(topf>0))
          {
          oy=my+1;
          topf--;
          setmoupos(mx,oy);
          fchange=1;
          } 
        if ((my==y+1+widf)&&(numf<maxf-1))
          {
          oy=my-2;
          topf++;
          setmoupos(mx,oy);
          fchange=1;
          }
        }
      }

    if (NewDir)
      {
      NewDir=False;
      if (strcmp(&ans[dirq],"..\\")==0) // deal with the go up a directory case.
        {
        writestr(x+lenq,y,attrq+15,Spaces);
        if (dirq>0)
          {
          dirq--;
          do { ans[dirq]=0; dirq--; } while ((dirq>0)&&(ans[dirq]!='\\'));
          if (dirq==0) ans[0]=0;
          else dirq++;
          numq=dirq;
          }
        else { numq=0; ans[0]=0; }
        writestr(x+lenq,y,attrq+15,ans);
        }
      if (((l1=GetChoices(fspec,ans,fnames,maxfname*FNAMELEN))!=FALSE)&&(ans[numq-2]!=92))
        {                  // do this only if directory exists
        erasecursor();
        closemenu(xf-1,y+2,15,widf,wf);
        maxf=l1;
        dirq=numq;
        widf=21-y;
        if (maxf<widf) widf=maxf;
        if (dirq+10>maxa) xf=(x+lenq+maxa-10);
        else              xf=(x+lenq+dirq);
        numf=0; topf=0;
        attrf=openmenu(xf-1,y+2,15,widf,wf);
        writestr(xf-2,y+1,attrf+14,"» ÄÄÄÄÄÄÄÄÄÄÄÄÄ É");
        drawcursor(xf-1,y+2,xf+FNAMELEN-1,CURSORCOL);
        fchange=1;
        oy=(y+2+(numf-topf));
        setmoupos(xf,oy);
        }
      else
        {
        ans[numq]=0;
        writech(x+lenq+numq,y,attrq+15,' ');
        numq--;
        }  // numq-- because it's added right back
      if (mouinstall) mouclearbut();
      }
    erasecursor();
    } while (done==-1);

  closemenu(xf-1,y+2,15,widf,wf);
  closemenu(x,y,lenq+maxa+1,1,wq);
  gotoxy(80,25);
  if (done != 0) ParseFileName(ans,wq,WorkDir);
  if (wq)     delete wq;
  if (wf)     delete wf;
  if (fnames) delete fnames;
  return(done);
  }


/*
static int findfiles(char *filespec, char *path,char *result,int maxfiles)
  {
  register int l;
  int ret=0;
  int ctr=1;
  struct ffblk f;
  char pf[80];

  OldCritError=(char interrupt (*)(...)) getvect(0x24);
  setvect(0x24,(void interrupt (*)(...)) NewCritError);

  sprintf(pf,"%s*.*",path);
  if (findfirst(pf,&f,0xFF)==-1) 
    {
    if (CritVal==100) errorbox("This directory doesn't exist!","(C)ontinue");
    *result=0;
    CritVal=100;
    return(FALSE);
    }

  strcpy (pf,path);
  strcat (pf,filespec);
  *result=0;
     // find all files in this directory
  ret = findfirst(pf,&f,15);
  while(ret==0)
    {
    sprintf(result+(ctr*9),"%s\0",f.ff_name);
    for (l=0;l<9;l++) 
      {
      if (f.ff_name[l]!='.') *(result+(ctr*9)+l)=f.ff_name[l];
      else { *(result+(ctr*9)+l)=0; l=9; }
      }  
 
    ctr++;                     // ctr is the # of files found total.
    if (ctr>=maxfiles) break;
    ret=findnext(&f);
    }
  setvect(0x24,(void interrupt (*)(...)) OldCritError);
  return(ctr);
  }
*/

int getstr(int x,int y, int maxlen, char *ans)
  {
  int l=0;
  int done=-1;
  int inkey=0;
  char Old[81];

  l = strlen(ans);
  strcpy(Old,ans);
  writestr(x,y,256,ans);
  gotoxy(x+l+1,y+1);
  do 
    {
    inkey=bioskey(0);
    switch(inkey&255)
      {
      case 8:           // Rub out key hit
        if (l>0) { l--; writech(x+l,y,256,' '); }
        gotoxy(x+l+1,y+1);
      break;
      case 27:
        strcpy(ans,Old);
        done=0;
      break;
      case 13:
        ans[l]=0;
        done=l;
      break;
      case 0:
      break;
      default:
        if (l<maxlen) 
          {
          ans[l]=(inkey&255);
          writech(x+l,y,256,ans[l]&255);
          l++;
          gotoxy(x+l+1,y+1);
          }
      break;
      }
    } while (done==-1);
  gotoxy(80,25);
  return(done);
  }

#pragma loop_opt(on)

int qwindow(int x,int y,int ln, char *q,char *ans)
  {
  int attr;
  char w[4000];
  int lenq=0;
  int done=0;

  lenq=strlen(q);
  
  attr=openmenu(x,y,lenq+ln+2,1,w);
  moucur(0);
  writestr(x+1,y,(attr+14)&255,q);
  done = getstr(x+lenq+1,y,ln,ans);
  closemenu(x,y,lenq+ln+2,1,w);
  return(done);
  }

#pragma loop_opt(off)

int errorbox(char *l1,char *l2,uint pauselen)
  {                          // ^^ OPTIONAL ARGUEMENT - nothing means wait for key.
  time_t curtime=0,starttime=0;
  int ink;
  int mx=0,my=0,buts=0;
  int x,y,x1,y1;
  int oldx,oldy,outofbounds=FALSE;
  int len;
  int bzero=0;
  char w[1000];

  len = strlen(l1);
  if (strlen(l2) > len) len = strlen(l2);

  x=40-(len/2)-1;
  y=8;
  y1=9;
  x1=x+len+2;

  moucur(FALSE);

  savebox(x-1,y-1,x1+3,y1+2,w);
  drawbox(x-1,y-1,x1+1,y1+1,ERRORATTR,2,SHADATTR);
  clrbox(x,y,x1,y1,ERRORATTR);
  writestr(x+1,y,ERRORATTR,l1);
  writestr(x+1,y+1,ERRORATTR,l2);

  setmoupos(oldx=x,oldy=y);

  time(&starttime);
  if (mouinstall) drawcursor(x,y,x,CURATTR);
  do
    { 
    if (mouinstall)
      {
      moustats(&mx,&my,&buts);
      if (mx<x)  {  mx=x; outofbounds=TRUE; }          // Keep it in bounds
      if (my<y)  {  my=y; outofbounds=TRUE; }
      if (mx>x1) { mx=x1; outofbounds=TRUE; }
      if (my>y1) { my=y1; outofbounds=TRUE; }
      if (outofbounds)
        {
        outofbounds=FALSE;
        setmoupos(mx,my);
        }
      if ((mx!=oldx)||(my!=oldy))  // Move the cursor
        {
        erasecursor();
        drawcursor(mx,my,mx,CURATTR);
        oldx=mx; oldy=my;
        }
      if (buts==0) bzero=1;
      }
    time(&curtime);
    } while ((bioskey(1)==0)&&((buts==0)||(bzero==0))&&(curtime<starttime+pauselen));

  erasecursor();
  if (curtime>=starttime+pauselen) ink=0;
  else if (bioskey(1)) ink=bioskey(0);
  else ink = (readch(mx,my)&255);

  restorebox(x-1,y-1,x1+3,y1+2,w);
  mouclearbut();
  return(ink);
  }


int vertmenu(int x,int y,int length,int itemnum, int curitem)
  {
  int selected=FALSE;
  int mx,my,mbut;
  int butwaszero=0;
  int inkey;
  char upkey,lowkey; 
  #ifdef USEJOYSTICK
  int joypos=0,joybut=0;
  #endif

  curitem--;
  length--;  
  if (curitem>=itemnum) curitem=itemnum-1;
  if (mouinstall)
    {
    setmoupos(x,y+curitem);
    moustats(&mx,&my,&butstat);
    }
  drawcursor(x,y+curitem,x+length,CURATTR);
  
  do
    {
    if (bioskey(1))
      {
      inkey = bioskey (0);
      lowkey = inkey&255;
      switch (lowkey)
        {
        case 0:
          upkey = inkey>>8;
          switch (upkey)
            {
            case 80:   // down arrow
              erasecursor();
              curitem++; if (curitem>=itemnum) curitem=0;
              setmoupos(x+length,y+curitem);
              drawcursor(x,y+curitem,x+length,CURATTR);
             break;
            case 72:   /* up arrow */
              erasecursor();
              curitem--; if (curitem<0) curitem=itemnum-1;
              setmoupos(x+length,y+curitem);
              drawcursor(x,y+curitem,x+length,CURATTR);
             break;
            }
          break;
        case 13:
          selected = TRUE;
         break;
        case 27:   // Escape
          selected=TRUE;
          curitem=-1;        // Return zero if abort key hit
         break;
        }
      }
    if (mouinstall)
      {
      moustats(&mx,&my,&mbut);
      if (mbut==0) butwaszero=1;
      my -= y;
      if ((my>=0)&&(my<itemnum)&&(my != curitem)&&(curitem!= -1))
        {
        erasecursor();
        curitem = my;
        drawcursor(x,y+curitem,x+length,CURATTR);
        }
      else setmoupos(x+length,y+curitem);

      if ((mbut>0)&(butwaszero==1))
        {
        butstat  = mbut;
        selected = TRUE;
        }
      }
    } while (!selected);
  erasecursor();
  return(curitem+1);
  }

int horizmenu(int itemnum,int curitem,int y,...)  // Dots are start of each item
  {
  int selected=FALSE;
  int mx,my,mbut;
  int inkey=0;
  char upkey=0,lowkey=0;
  int *lens;
  register int l;        // Loop variable
  #ifdef USEJOYSTICK
  int joypos=0,joybut=0;
  #endif  

  lens=&y;
  lens++;                // Lens now points to the x coord of the first item.

  if (curitem==0) curitem++;
  curitem--;
  moucur(0);
  setmoupos( (lens[curitem]+lens[curitem+1])/2,y);
  moustats(&mx,&my,&butstat);
  drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);

  do
    {
    if (bioskey(1))
      {
      inkey = bioskey (0);
      lowkey = inkey&255;
      if (lowkey==0)
        {
        upkey = inkey>>8;
        switch (upkey)
          {
          case 77:        // right arrow
            erasecursor();
            curitem++; if (curitem>=itemnum) curitem=0;
            setmoupos(lens[curitem+1]-1,y);
            drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);
            break;
          case 75:        // left arrow
            erasecursor();
            curitem--; if (curitem<0) curitem=itemnum-1;
            setmoupos(lens[curitem]+1,y);
            drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);
            break;
          case 72:        // up arrow
            lowkey = 27;  // escape is the same as up arrow
            break;
          case 80:        // down arrow
            lowkey = 13;  // return is the same as dn arrow
            break;
          }
        }
      if ((lowkey==13)||(lowkey==27)) selected = TRUE;
      }
    if (mouinstall)
      {
      int olditem=curitem;
      moustats(&mx,&my,&mbut);
      if (mx<lens[0])             curitem = 0;
      else if (mx>=lens[itemnum]) curitem = itemnum-1;
      else for (l=0;l<itemnum;l++) // Find which item it is.
            if ((mx>=lens[l])&&(mx<lens[l+1])) { curitem=l; l=itemnum+1; }

      if (olditem != curitem)
        {
        erasecursor();
        drawcursor(lens[curitem],y,lens[curitem+1]-1,CURATTR);
        }

      if (mbut != butstat)
        {
        butstat  = mbut;
        selected = TRUE;
        }
      }
    } while (!selected);
  if (lowkey == 27) curitem=0;
  else              curitem++;
  erasecursor();
  return(curitem);
  }

static char oldattr[10];
static char oldx[10];
static char oldy[10];
static char oldx1[10];
static char stackpos=0;

static void drawcursor(int x,int y,int x1,int attr)
  {
  int l=0;

  if (stackpos<10)
    {
    oldattr[stackpos]=((char) (readch(x,y)>>8));
    oldx[stackpos]   =x;
    oldy[stackpos]   =y;
    oldx1[stackpos]  =x1;
    for(l=x;l<=x1;l++) writech(l,y,attr,readch(l,y));
    stackpos++;
    }
  }

static void erasecursor(void)
  {
  int l=0;

  if (stackpos>0)
    {
    stackpos--;
    for(l=oldx[stackpos];l<=oldx1[stackpos];l++)
      writech(l,oldy[stackpos],oldattr[stackpos],readch(l,oldy[stackpos]));
    }
  }

#pragma loop_opt(on)

void closemenu(int x,int y, int len, int wid, char *buffer)
  {
  len++; wid++;
  x--; y--;
  moucur(0);
  if (buffer !=NULL) restorebox(x,y,x+len+2,y+wid+1,buffer);
//  moucurbox(0,0,79,24);
  windlvl--;
  }

int openmenu(int x,int y, int len, int wid, char *buffer)
  {
  len++;wid++;
  windlvl++;
  x--; y--;

  if (buffer !=NULL) savebox(x,y,x+len+2,y+wid+1,buffer);
  drawbox(x,y,x+len,y+wid,MENUATTR,MENUBOX,SHADATTR);
  clrbox(x+1,y+1,x-1+len,y-1+wid,MENUATTR);
//  setmoupos(x+2,y+2);
//  moucurbox(x+1,y+1,x+len-1,y+wid-1);
//  moucur(FALSE);
  return(MENUATTR-FOREATTR);
  }

int loadany(char *prompt,char *ext,char *path,unsigned int bytes, char *addr,int remember)
  {
  FILE *fp;
  char fname[81];
  char result;
  char srchfor[6];
  
  strcpy(fname,path);
  if (!getfname(5,7,prompt,ext,fname)) return(FALSE);
  if ( (fp=fopen(fname,"rb")) ==NULL)  // must exist because getfname searches for it
    {
    errorbox("Can't Find file!","(C)ontinue");
    return(FALSE);
    }
  fread(addr,bytes,1,fp);
  fclose(fp);
  if (remember) strcpy( (char *) curfile,(char *) fname);
  return(TRUE);
  }

#pragma loop_opt(off)

FILE *GetSaveFile(char *prompt,char *ext,char *path,char *curfile)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN];
  int  result=0;

  if (strcmp(curfile,"")==0) strcpy(fname,path);
  else strcpy(fname,curfile);

  do
    {
    *FileExt(fname)=0;
    if (!qwindow(2,7,30,prompt,fname)) return(NULL);
    strcat (fname,ext);

    if (strcmpi((char*)fname,(char*)curfile)!=0)
      {
      if ((fp=fopen(fname,"rb"))!=NULL)
        {  // file exists already, and is not current one
        fclose(fp);
        fp=NULL;
        result = errorbox("This file already exists! Saving now will destroy the old file!","(C)ontinue save    (D)o not save")&255;
        if ((result!='C')&(result!='c')) return(NULL);
        }
      }

    if ((fp=fopen(fname,"wb"))==NULL)
      {
      do
        {
        result=toupper(errorbox("Unable to Save!","(A)bort Save    (R)etry")&255);
        if (result=='A') return(False);
        } while (result!='R');
      }
    } while (fp == NULL);
  strcpy(curfile,fname);    // Save the new current filename.
  return(fp);
  }

#ifdef CRIPPLEWARE
void StdCrippleMsg(void)
  {
  errorbox("This DEMO version of Game-Maker 3.00 only allows Maps to be saved.","To REALLY create games please buy a copy of Game-Maker! (C)ontinue",10);
  }
#endif

int saveany(char *prompt,char *ext,char *path, unsigned int bytes, char *buffer)
  {
  FILE *fp=NULL;

#ifdef CRIPPLEWARE
  StdCrippleMsg();
#else
  if ( (fp=GetSaveFile(prompt,ext,path,curfile))==NULL) return(False);
  fwrite(buffer,bytes,1,fp);
  fclose(fp);
  return(True);
#endif
  }

#pragma loop_opt(on)


int delany(const char *prompt,const char *fspec,char *path)
  {
  int resp;
  char fname[MAXFILENAMELEN];
  char str[81];

  strcpy(fname,path); 
  if (!getfname(8,10,prompt,fspec,fname)) return(FALSE);
  sprintf(str,"Are you sure you want to delete %s",fname);
  resp=errorbox(str,"         (D)elete    (E)xit");
  resp&=0x0ff;
  if ((resp=='d')|(resp=='D')) return(del(fname,""));
  else                         return(FALSE);
  }

static int del(char *file,char *path)
  {
  char p[81];

  sprintf(p,"%s%s",path,file);
  if (remove(p)==0) return(TRUE);
  else              return(FALSE);
  }
