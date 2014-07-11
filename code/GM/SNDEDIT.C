/*---------------------------------------------------------*/
/*   Sound Editor    by :  Oliver Stone                    */
/*   Digitized speech capabilities Andy Stone, Pete Savage */
/*   last edited 12-27-92                                  */
/*---------------------------------------------------------*/
#include <process.h>
#include <stdio.h>
#include <dos.h>
#include <bios.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gen.h"
#include "gmgen.h"
#include "windio.h"
#include "scrnrout.h"
#include "palette.h"
#include "gmgen.h"
#include "mousefn.h"
#include "sound.h"
#include "jstick.h"
#include "graph.h"
#include "facelift.h"
#include "tranmous.hpp"

#define IN(x,y,lx,ly,mx,my) ((x>=lx)&&(y>=ly)&&(x<=mx)&&(y<=my))

#define ROOTN    25
#define RANGE    (FREQH-FREQL)
#define LX       19
#define TY       5
#define RX       (LX+(4*NUMFREQ))
#define BY       (TY+(RANGE*ROOTN)-1)
#define MX       (RX+95)
#define MY       (PY+6)
#define HY       (MY+12)
#define PX       (RX+30)
#define PY       100
#define P2       (PY+12)
#define P3       (PY+24)
#define HELPFILE "sound.hlp"
#define TEXX     178
#define TEXY     10
#define TEXY2    (TEXY+12)
#define VOCX     (TEXX+4)
#define VOCY     60
#define VOCX1    (VOCX+26*8)
#define VOCY1    (VOCY+21)

static void entersnd(int i);
static void PlaySound(int i);
static int freq2index(unsigned int fre);
static void drwscrn(void);
static void Announce(int num);
static void Drawmarks(int i);
static void initallData(void);
static void playb(int x,int y, unsigned char col[4]);
static void nextb(int x,int y, unsigned char col[4]);
static void helpb(int x,int y,unsigned char *col);
static void menub(int x,int y,unsigned char *col);
static void prevb(int x,int y, unsigned char col[4]);
static void cleanup(void);
static void convcmf(void);
static long int fgetvarlen(FILE *fp);
static int fputvarlen(unsigned long int value,FILE *fp);
extern unsigned char  FaceCols[MAXFACECOL];      //Screen colors
extern char           *FontPtr;
extern unsigned char  HiCols[4];

RGBdata      colors[256];
sndstruct    s[LASTSND];
char         DigiSnd[LASTSND][9];
unsigned int roottwo[RANGE*ROOTN];
char         saved=1;
extern char  curfile[31];
ConfigStruct cs;

QuitCodes main(int argc,char *argv[])
  {
  char w[1000];
  int choice=1;
  int attr=0;
  int temp;

#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif

  atexit(cleanup);
  FontPtr=GetROMFont();

  /*---------------------Set up Mouse  */
  if (initmouse()==FALSE)
    {
    printf("This application requires a Microsoft Compatible Mouse.");
    return(quit);
    }
  LoadConfigData(&cs);
  if (strcmpi(cs.SndDrvr,"NotInit")==0)
    errorbox("Run PLAYGAME to initialize","sound card information.",5);
  else if (cs.ForceSnd==SndBlaster)
    {
    int temp;
    if ( (temp=InitSbVocDriver(&cs))!=1)
      {
      if (temp==0) errorbox("Sound card voice driver cannot be loaded!",".VOC file sounds cannot be played.",5);
      if (temp==-1) errorbox("Sound Blaster interrupt or port is set up incorrectly",".VOC file sounds cannot be played.",5);
      }
    }
 // else errorbox("No sound card specified",".VOC file sounds cannot be played.",5);
  initallData();
  do
    {
    SetTextColors();
    attr = openmenu(50,3,20,12,w);
    moucur(0);
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    writestr(50,3,PGMTITLECOL,"   SOUND DESIGNER   ");
    writestr(50,4,PGMTITLECOL,"    Version "GMVER"    ");
    writestr(50,5,PGMTITLECOL,"  By  Oliver Stone  ");
    writestr(50,6,PGMTITLECOL," Copyright (C) 1994 ");
    writestr(50,8,attr+14, " Choose a Sound Set ");
    writestr(50,9,attr+14, " New Sound Set");
    writestr(50,10,attr+14," Edit Sounds");
    writestr(50,11,attr+14," Save a Sound Set");
    writestr(50,12,attr+14," Delete a Sound Set");
    writestr(50,13,attr+14," Convert a CMF File");
    writestr(50,14,attr+14," Quit");
    choice=vertmenu(50,8,20,7,choice);
    closemenu(50,3,20,12,w);
    switch (choice) {
      case 1: if (!saved) {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice = 3; break; }
         }
        LoadSnd("Enter name of sound set to load: ","*.snd\0",WorkDir,(unsigned char *)&s,&DigiSnd[0][0],1);
        choice = 2; saved=1; break;
      case 2: 
        if (!saved) 
          {
          choice = (errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice = 3; break; }
          }
        initallData(); choice = 2; saved=True;
        errorbox("New sound set created!","(C)ontinue",2);
        break;
      case 3: entersnd(0);  break;
      case 4: if (SaveSnd("Enter name of sound set to save:",".snd",WorkDir,(char *)&s,&DigiSnd[0][0]))
                { saved = TRUE; choice=5; }
              else choice = 3;
              break;
      case 5: delany("Delete which sound set: ","*.snd\0",WorkDir); choice=4; break;
      case 6: convcmf(); break;
      case 0:
      case 7: if (!saved)
          {
          choice = (errorbox("Unsaved changes--Really Quit?","  (Y)es  (N)o")&0x00FF);
          if ( (choice!='Y') && (choice!='y') ) { choice = 3; break; }
          }
        choice=7; break;
     }
    choice++;
    } while (choice != 8);
  return(menu);
  }

static void cleanup(void)
  {
  Time.TurnOff();
  ShutSbVocDriver();
  }

void entersnd(int i)
 {
  register int j;
  int x=0,y=0,but=0,grabbed;
  int oldx,oldy,oldbut;
  int inkey;
  char str[9];

  delay(0);
  GraphMode();
  InitStartCols(colors);
  SetCols(colors,FaceCols);
//  colors[4].red=63; colors[4].green=8; colors[4].blue=8;
//  colors[1].red=4; colors[1].green=4; colors[1].blue=55;
  drwscrn();
  Announce(i);
  moucur(1);
  setmoupos(MX+18,MY+5);
  FadeTo(colors);
  do { moustats(&x,&y,&but); } while (but != 0);
  while(1)
    {
    oldx=x;   oldy=y;  oldbut=but;
    moustats(&x,&y,&but);
    if ((but)&&IN(x,y,VOCX,VOCY,VOCX1,VOCY1)) // Edit the .voc file name.
      {
      strcpy(str,DigiSnd[i]);
      moucur(FALSE);
      BoxWithText(VOCX+24,VOCY+12,"          ",FaceCols);
      if (GGet(VOCX+33,VOCY+13,FaceCols[BLACK],FaceCols[GREEN],str,8)>=0)
        {
        if (strcmpi(DigiSnd[i],str)!=0)  // Check to see if its changed for
          {                              // the saved flag.
          strcpy(DigiSnd[i],str);
          saved=FALSE;
          }
        PlaySound(i);
        }
      if ( (DigiSnd[i][0]==0)||(DigiSnd[i][0]==' ') )
        BoxFill(VOCX+24,VOCY+12,VOCX+104,VOCY+20,FaceCols[GREY2]);
      else
        {
        sprintf(str," %-8s ",DigiSnd[i]);
        BoxWithText(VOCX+24,VOCY+12,str,FaceCols);
        }
      moucur(TRUE);
      }
    if ( (but)&&( (oldx!=x)||(oldy!=y)||(oldbut!=but) ) )
      {
      if ( (x>MX)&(x<MX+36) ) {
        if ( (y>HY)&(y<HY+9) ) {
          while(but) moustats(&x,&y,&but);
          moucur(0);
          TextMode();
          DisplayHelpFile(HELPFILE);
          mouclearbut(); while (bioskey(1)) bioskey(0);
          GraphMode(); setmoupos(MX+16,HY+4); drwscrn();
          Announce(i);  FadeTo(colors);
          do { moustats(&x,&y,&but); } while (but != 0);
          moucur(1);
          }
        if ( (y>MY)&(y<MY+9) ) { TextMode(); return; }
        }
      if ( (x>=LX)&(x<RX)&(y>=BY+2)&(y<=BY+4) )
        {
        grabbed = (x-LX)/4;
        moucur(0);
        BoxFill(LX+4*grabbed,TY,LX+3+4*grabbed,BY,FaceCols[GREY2]);
        moucur(1);
        s[i].freq[grabbed]= 0;
        saved=0;
        }
      if ( (x>=LX)&(x<RX)&(y>=TY)&(y<=BY) ) {
        grabbed = (x-LX)/4;
        moucur(0);
        s[i].freq[grabbed]= roottwo[BY-y];
        BoxFill(LX+4*grabbed,TY,LX+3+4*grabbed,y,FaceCols[GREY2]);
        BoxFill(LX+4*grabbed,y,LX+2+4*grabbed,BY,FaceCols[GREY1]);
        Line(LX+3+4*grabbed,y+1,LX+3+4*grabbed,BY,FaceCols[GREY3]);
        moucur(1);
        saved=0;
       }
      if ( (x>PX)&(x<PX+36) )
        {
        if ( (y>PY)&(y<PY+10) )
          {
          PlaySound(i);
          }
        if ( (y>P3)&(y<P3+10) )
          {
          if ( (++i)>=LASTSND ) i=0;
          Announce(i);
          }
        if ( (y>P2)&(y<P2+10) )
          {
          if ( (--i)<0 ) i=LASTSND-1;
          Announce(i);
          }
        }
      }
    if ( bioskey(1) )
      {
      inkey = bioskey(0);
      if ( (inkey&0x00FF) == 0)
        { 
        inkey >>= 8;
        switch (inkey) {
          case 75:   // left arrow
          case 80:   // down arrow
            inkey='p'; break;
          case 77:   // right arrow
          case 72:   // up arrow
            inkey='n'; break;
          }
        }
      inkey &= 0x00FF;
      switch (inkey) {
        case 'n': case 'N': case '=': case '+':
          i++; if ( (++i)>=LASTSND) i%=LASTSND;        // add two then -1
        case 'p': case 'P': case '-': case '_':        // then play
          if ( (--i)<0 ) i=LASTSND-1; Announce(i);
        case 13: PlaySound(i); break;
        case 27: TextMode(); return;
        }
      }
    }
  }

static void PlaySound(int i)
  {
  register int j;

  if ((DigiSnd[i][0]!=0) && (DigiSnd[i][0]!=' '))
    if (PlaySbVocFile((char far *) DigiSnd[i])==1) return;
  if (s[i].freq[0]) sound(s[i].freq[0]);
  delay(DELAY);
  for (j=1;j<NUMFREQ;j++)
    {
    if (s[i].freq[j])
      {
      if (s[i].freq[j]!=s[i].freq[j-1]) sound(s[i].freq[j]);
      }
    else nosound();
    delay(DELAY);
    }
  nosound();
  }

int freq2index(unsigned int fre)
  {
  int j=-1;

  while (fre>roottwo[(++j)]);
  return(j);
  }

static void drwscrn(void)
  {
  register int j;

  SetAllPalTo(&colors[BKGCOL]);
  drawsurface(LX-17,TY-3,RX-LX+19,BY-TY+21,FaceCols);
  BoxFill(LX-1,TY,RX-1,BY,FaceCols[GREY2]);
  Line(LX-2,TY,LX-1,BY+3,FaceCols[GREY3]);
  Line(LX-2,TY-1,RX-1,TY-1,FaceCols[GREY4]);
  Line(LX-2,BY+2,RX-1,BY+2,FaceCols[RED2]);
  Line(LX-2,BY+3,RX-1,BY+3,FaceCols[RED2]);
  for (j=1;j<NUMFREQ+1;j++)
    Point(LX-1+j*4,BY+1,FaceCols[RED2]);
  Point(LX-1,BY+1,FaceCols[GREY2]);
  BoxFill(LX-14,(TY+BY-72)/2-1,LX-5,(TY+BY+72)/2,FaceCols[GREEN]);
  Line(LX-15,(TY+BY-72)/2-2,LX-15,(TY+BY+72)/2,FaceCols[GREY3]);
  Line(LX-15,(TY+BY-72)/2-2,LX-5,(TY+BY-72)/2-2,FaceCols[GREY4]);
  GWrite(LX-13,(TY+BY-72)/2,FaceCols[BLACK],"F\nr\ne\nq\nu\ne\nn\nc\ny");
  BoxWithText((LX+RX-32)/2,BY+7,"Time",FaceCols);
  drawsurface(MX-3,MY-2,42,28,FaceCols);
  menub(MX,MY,FaceCols);
  helpb(MX,HY,FaceCols);
  drawsurface(PX-3,PY-2,42,40,FaceCols);
  playb(PX,PY,FaceCols);
  prevb(PX,P2,FaceCols);
  nextb(PX,P3,FaceCols);
  drawsurface(TEXX-3,TEXY-3,17*8+6,TEXY2-TEXY+22,FaceCols);
  drawsurface(VOCX-3,VOCY-3,16*8+6,TEXY2-TEXY+14,FaceCols);
  Line(TEXX-1,TEXY2-1,TEXX-1,TEXY2+16,FaceCols[GREY3]);
  Line(TEXX-1,TEXY2-1,TEXX+17*8,TEXY2-1,FaceCols[GREY4]);
  BoxWithText(VOCX+24,VOCY+12,"          ",FaceCols);
  }

static void Announce(int num)
  {
  char str[25];

  sprintf(str,"Sound number: %2d",num+1);
  BoxWithText(TEXX,TEXY,str,FaceCols);
  switch(num)
    {
    case  0: sprintf(str,"Increase of\n hit points"); break;
    case  1: sprintf(str,"Monster killed"); break;
    case  2: sprintf(str,"Increase of money"); break;
    case  3: sprintf(str,"Decrease of money"); break;
    case  4: sprintf(str,"Gain of a life"); break;
    case  5: sprintf(str,"Increase of\n the score"); break;
    case  6: sprintf(str,"Decrease of\n the score"); break;
    case  7: sprintf(str,"Increase of\n any counter"); break;
    case  8: sprintf(str,"Decrease of\n any counter"); break;
    default: str[0]=0; break;                //Null string default
    }
  if (str[0]==0) BoxFill(TEXX,TEXY2,TEXX+17*8,TEXY2+16,FaceCols[GREY2]);
  else
    {
    BoxFill(TEXX,TEXY2,TEXX+17*8,TEXY2+16,FaceCols[GREEN]);
    GWrite(TEXX+1,TEXY2+1,FaceCols[BLACK],str);
    }
  Drawmarks(num);
  BoxWithText(VOCX,VOCY,".VOC Speech File",FaceCols);
  if ( (DigiSnd[num][0]==0)||(DigiSnd[num][0]==' ') )
    BoxFill(VOCX+24,VOCY+12,VOCX+104,VOCY+20,FaceCols[GREY2]);
  else
    {
    sprintf(str," %-8s ",DigiSnd[num]);
    BoxWithText(VOCX+24,VOCY+12,str,FaceCols);
    }
  }

static void Drawmarks(int i)
  {
  register int j;

  moucur(0);
  BoxFill(LX,TY,RX-1,BY,FaceCols[GREY2]);
  for (j=0;j<NUMFREQ;j++)
    if (s[i].freq[j])
      {
      BoxFill(LX+4*j,BY-freq2index(s[i].freq[j]),LX+2+4*j,BY,FaceCols[GREY1]);
      Line(LX+3+4*j,BY-freq2index(s[i].freq[j])+1,LX+3+4*j,BY,FaceCols[GREY3]);
      }
  moucur(1);
 }

void initallData(void)
  {
  register int j,k;
  double temp;
  float  temp2;

  for (j=0;j<RANGE*ROOTN;j++)
    {
    temp=((double)FREQL+(double)j/(double)ROOTN);
    temp2 = (float) pow( 2.0000, temp );
    roottwo[j] = (unsigned int) temp2;
    }
  for (k=0;k<30;k++)
    for (j=0;j<NUMFREQ;j++)
      {
      s[k].freq[j]=roottwo[ ((int) 4.5*ROOTN)+1];
      }
  }


static void convcmf(void)
  {
  char     fileIN[MAXFILENAMELEN]="",fileOUT[MAXFILENAMELEN]="";
  FILE     *fpin, *fpout;
  int      perbeat, origin, count;
  char     error     = 0;
  char     fileID[5] = {0,0,0,0,0};
  int      buff1[20];
  char     buff[70];
  unsigned long int paws;
  
  if (!getfname(10,10,"Enter the file to convert: ","*.cmf\0",fileIN)) return;
  if ((fpin=fopen(fileIN,"rb")) == NULL)
    errorbox("Trouble opening CMF file","Return to (M)enu");
  else if ( fread(fileID,1,4,fpin) < 4 )
    errorbox("End of File Error!","Return to (M)enu");
  else if (strcmp(fileID,"CTMF") != 0)
    errorbox("This is not a cmf file!","Return to (M)enu");
  else if ( fread(buff1,2,18,fpin) < 18 )
    errorbox("End of File Error!","Return to (M)enu");
  else if (buff1[4] == HDRCLK)
    errorbox("File does not need conversion","Return to (M)enu");
  else 
    {
    do
      {
      error=0;
      strcpy(buff,fileIN);
      for(count=strlen(buff)+2;count>1;count--)
        {
        buff[count]=buff[count-2];
        if (buff[count]=='\\') break;
        }
      buff[count]  ='M';
      buff[count-1]='G';
      strcpy(&fileOUT[4],&buff[count-1]);
      if (!qwindow(10,10,25,"Enter the file to output: ",fileOUT)) return;

      strcpy(FileExt(fileOUT),".cmf");
      if (strcmpi(fileIN,fileOUT)==0)
        error=errorbox("Duplicate file name error","(R)ename File    Return to (M)enu");
      if ( (error=='M') || (error=='m') )  { fclose(fpin); return; }
      } while (error!=0);
    if ((fpout=fopen(fileOUT,"wb")) == NULL)  
      {
      errorbox("Trouble creating new CMF file","Return to (M)enu");
      fclose(fpin);
      return;
      }
    fwrite(fileID,1,4,fpout);
    fwrite(buff1,2,3,fpout);
    origin=buff1[4];
    perbeat=buff1[3];
    perbeat = (int) (((long)perbeat * (long)HDRCLK) / (long)origin);
    fwrite(&perbeat,2,1,fpout);
    perbeat = HDRCLK;
    fwrite(&perbeat,2,1,fpout);
    fwrite(&buff1[5],2,13,fpout);
    count=buff1[2];
    while (count>40)
      {
      fputc(fgetc(fpin),fpout);
      count--;
      }
    perbeat=0;
    do
      {
      paws=fgetvarlen(fpin);
      paws = ((unsigned long int)HDRCLK * paws)/(unsigned long int)origin;
      if (!fputvarlen(paws,fpout)) error |= 2;  // number too big
      if (feof(fpin))
        {
        error|=4;                        // EOF error
        break;
        }
      switch((count=fputc(fgetc(fpin),fpout)) & 0xF0)
        {
        case 0xF0: perbeat=1;    // two more bytes until end of file
        case 0xB0:          // code for special function -- two bytes follow
        case 0x90:     // code for channel on -- two more bytes before next paws
        case 0x80: fputc(fgetc(fpin),fpout);  // two more bytes before next paws
        case 0x70:
        case 0x60:
        case 0x50:                   // this section are codes for MIFI notes
        case 0x40:
        case 0x30:
        case 0x20:
        case 0x10:
        case 0x00:
        case 0xC0: fputc(fgetc(fpin),fpout);  // one more byte before next paws
                   break;                     // code to assign instr. to chan.
        default: error|=1;    // unknown code
                 fputc(fgetc(fpin),fpout);  // one more byte before next paws
                 break;     
        }
      } while ((!perbeat) && (!error));
      
    if (error) 
      {
      sprintf(buff,"WARNING: ERROR number %d!",error);
      errorbox(buff,"Song may not play correctly");
      }
    fclose(fpout);
   }
  fclose(fpin);
 }

static long int fgetvarlen(FILE *fp)
  {
  long int value=0;
  char c;
  
  if ((value = (long int) fgetc(fp)) & 0x80)
    {
    value &= 0x7F;
    do
      {
      value = (value << 7) + ((c=fgetc(fp)) & 0x07F);
      } while (c & 0x80);
    }
  return(value);
  }
  
static int fputvarlen(unsigned long int value,FILE *fp)
  {
  long int buffer;
  
  if (value > 0x0FFFFFFF) return(0);
  buffer = value & 0x7F;
  while ((value >>= 7) > 0)
    {
    buffer <<=8;
    buffer |= 0x80;
    buffer += (value & 0x7F);
    }
  while (buffer & 0x80)
    {
    fputc( (char) (buffer&0xFF), fp );
    buffer >>= 8;
    }
  fputc( (char) (buffer&0xFF), fp );
  return(1);
  }

static void playb(int x,int y, unsigned char col[4])
  {
  char playar[] = "         UUUUUUU@UUUUUUUTUýuW×WUTUU×u]wWUUUUýu_õýUUUUÕu]uuUU•UÕÝuuUVUUUUUUUT)UUUUUUUhªªªªªªª€";
  draw4dat(x,y,playar,35,10,col);
  }

static void prevb(int x,int y, unsigned char col[4])
  {
  char prevar[] = "         UUUUUUU@UUUUUUUTUý_÷WUTUU×uÝWWUUUUý_×WUUUUÕuÝUÝUU•UÕußõuUVUUUUUUUT)UUUUUUUhªªªªªªª€";
  draw4dat(x,y,prevar,35,10,col);
  }

static void nextb(int x,int y, unsigned char col[4])
  {
  char nextar[] = "         UUUUUUU@UUUUUUUTU×ÝwÿUTUU÷u]uuUUUUÿWÕuUUUUßu]uuUU•U×ÝuuUVUUUUUUUT)UUUUUUUhªªªªªªª€";
  draw4dat(x,y,nextar,35,10,col);
  }
