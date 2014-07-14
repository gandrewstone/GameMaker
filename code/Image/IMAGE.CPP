#pragma  inline
#define  GRAPHICS
#define  STAND_ALONE
#include <dos.h>
#include <bios.h>
#include <dir.h>
#define  TMODE 3
#include "gen.h"
#include "pal.h"
#include "coord2d.hpp"
#include "viddrive.hpp"
#include "fileclss.hpp"
#include "image.h"

#ifndef  STAND_ALONE
#include "geninput.hpp"
#endif


#define  IMAGESEPERATOR 0x2C
#define  BUFMAX 2000
static   uchar    FastBuf[BUFMAX];

static   LZWTable DefaultTable[MAXLZWINDEX];


#ifdef   GENINPUT
extern   InputClass Inp;
#endif

boolean GIFfamily::Startup (FileRead &gif,VideoMode *v,Palette *CurPal)
  {
  uchar Flag;
  int   j;
  GIFHeader           *head;
  GIFScreenDescriptor *pic;

  Data      = &gif;
  Vid       = v;
  PicStart  = 0;

  if (Data==NULL)      return(False);
  if (CurPal)          PalCvt=True;
  else                 PalCvt=False;
  head=(GIFHeader*)    Data->Peek(sizeof(GIFHeader));
  if (!Validate(head)) return(False);

  strncpy(GifVer,head->Ver,3);
  GifVer[3]    = 0;
  Size         = head->Size;
  BkCol        = head->BkCol;
  Flag         = head->Flag;
  Pal.ColsUsed = 2 << (Flag & 0x07);
  Data->Skip(sizeof(GIFHeader));

  if (Flag & 0x80) if (!gif.Get(Pal.Cols,sizeof(RGBdata)*Pal.ColsUsed)) return(False);

    {
    uchar temp;
    while ((temp=Data->Getc())!=IMAGESEPERATOR) // Does an image follow?
      {
      if (temp=='!') SkipExtension(Data);       // No, so skip the extension.
      else return(False);                      // No, so not a valid gif.
      }
    }                                        // Yes, so load its initial data.

  // Keep in mind that .GIFs can have multiple pictures in them.
  // This gets the info for the first.

  pic=(GIFScreenDescriptor*) Data->Peek(sizeof(GIFScreenDescriptor));
  Pos        = pic->Pos;
  Size       = pic->Size;
  Flag       = pic->Flag;
  Interlaced = Flag&0x40;
  Data->Skip(sizeof(GIFScreenDescriptor));

  // New Palette - specific to this picture

  if (Flag & 0x80)
    {
    Pal.ColsUsed = 2 << (Flag & 0x07);
    if (!gif.Get(Pal.Cols,sizeof(RGBdata)*Pal.ColsUsed)) return(False);
    }

  for (j=0;j<Pal.ColsUsed;j++)
    {
    Pal.Cols[j] >>= 2; // make values range to 63, not to 256
    if (CurPal==NULL) ColXlat[j] = j;
    else
      {
      if (CurPal->ColsUsed<256)
        {
        int   Error;
        ColXlat[j] = CurPal->Match(Pal(j),&Error);
        if (Error>0)
          {
          ColXlat[j]=CurPal->ColsUsed;
          CurPal->Cols[CurPal->ColsUsed] = Pal.Cols[j];
          CurPal->ColsUsed++;
          }
        }
      else ColXlat[j] = CurPal->Match(Pal(j));
      }
    }

  PixelSize = Data->Getc(); // Number of bits in a single pixel.
  PicStart  = Data->CurPos;
  return(True);
  }

void GIFfamily::SkipExtension(FileRead *gif)
  {
  char jumper=True;
  register int j;

  gif->Getc();           // throw away this byte
  while((jumper)&&(!gif->EOFflag))
    {
    jumper = gif->Getc();
    for (j=0;j<jumper;j++) gif->Getc();
    }
  }

uint GIFfamily::GetCode(void)
  {
  uint Retrn;
  asm {
  IDEAL
  push   si
  push   di
  push   ds
  push   es

  lds    si,this
  les    di,[ds:si.(class GIFfamily) Input]
  mov    bl,[ds:si.(class GIFfamily) BitsUsed]
  mov    bh,[ds:si.(class GIFfamily) CodeSize]

  mov    dx,[WORD PTR es:di]    ;// Load so dl is es:di and dh is es:di+1
  mov    ax,[WORD PTR es:di+1]
  mov    cl,bl
  shr    dx,cl
  shr    ax,cl
  mov    dh,al
                     ;// Check to see if > 15th bit is needed
  mov    ax,0FFFFh
  mov    cl,bh
  shl    ax,cl      ;// Now there are CodeSize+bitsused zeros in the bottom of ax
  not    ax         ;// Now codesizeBitsUsed ones.

  and    ax,dx       ;// ax is almost the # -- we need to shift it to 0 first.

  add    bl,bh       ;// BitsUsed += CodeSize

  mov    Retrn,ax
//  }
  TpLp:
//  asm {
  cmp    bl,8
  jl     SameByte
  sub    bl,8
  inc    [WORD PTR ds:si.(class GIFfamily) PktIdx]
  inc    di
  jmp    TpLp
//  }
  SameByte:
//  asm {
  mov    [BYTE PTR ds:si.(class GIFfamily) BitsUsed],bl
  mov    [WORD PTR ds:si.(class GIFfamily) Input],di
  pop    es
  pop    ds
  pop    di
  pop    si
  MASM
  }
  return(Retrn);
  }

boolean GIFfamily::ExpandCode(int coded)
  {
  uint i=BUFMAX;
  if (!FirstCode)
    {
    int properone;

    if (coded<lastcode) tab[lastcode].Pix = tab[coded].first;
    else tab[lastcode].Pix = tab[oldcode].first;
    tab[lastcode].code     = oldcode;
    tab[lastcode].first    = tab[oldcode].first;

    while(coded>NumCodes)
      {
      FastBuf[--i] = ColXlat[tab[coded].Pix];
      coded        = tab[coded].code;
      }

    lastcode++;                                // Advance the table pointer.
    if ( (lastcode==nextlimit)&&(CodeSize<12)) // inc. the # if bits in a code if needed.
      {
      CodeSize++;
      nextlimit<<=1;
      }
    }

  FastBuf[--i] = ColXlat[coded];
  if (FirstCode) FirstCode=False;
  return(DrawData(i,BUFMAX-i));
  }

boolean GIFfamily::DrawData(uint DataStart,uint Len)
  {
  uint i;
  uint count=0;
  uint End=DataStart+Len;

  for(i=DataStart;i<End;i+=count)
    {
    count = Min(End-i,Size.x-Pos.x);
#ifdef GRAPHICS
    Vid->RepCols(Pos,count,&(FastBuf[i]),Vid);
#endif
    Pos.x+=count;
    if (Pos.x==Size.x)
      {
      Pos.x=0;
      if (!Interlaced) { Pos.y++; if (Pos.y>=Size.y) return(True); }
      else
        {
        switch(Pos.y%8)
          {
          case 0:   Pos.y+=8; if (Pos.y>=Size.y) Pos.y=4; break;
          case 4:   Pos.y+=8; if (Pos.y>=Size.y) Pos.y=2; break;
          case 2:
          case 6:   Pos.y+=4; if (Pos.y>=Size.y) Pos.y=1; break;
          default:  Pos.y+=2;
          }
        }
      }
    }
  return(False);
  }

void GIFfamily::InitLZWTable(void)
  {
  if (tab==NULL) { tab = DefaultTable; MyTable=False; }
    // { tab = new LZWTable [MAXLZWINDEX]; MyTable=True; }

  NumCodes = 1 << PixelSize;

  for (register int j=0;j<NumCodes+2;j++)
    {
    tab[j].code   =0;
    tab[j].Pix    =j; //ColXlat[j];
    tab[j].first  =j;
    }

  FirstCode = True;
  CodeSize  = PixelSize + 1;
  lastcode  = NumCodes  + 2;
  nextlimit = NumCodes << 1;
  }

void GIFfamily::PalMatch(Palette *p)
  {
  for (int j=0;j<Pal.ColsUsed;j++) ColXlat[j]=Pal(j).Match(p->Cols);
  }



// ----------------------------- GIFimage ----------------------------------

boolean GIFImage::Draw(Coord2d Pos)
  {
  if ((!Data->SetPos(PicStart))||(Vid==NULL)) return(False);
#ifdef GRAPHICS
  if ((Pal.ColsUsed!=0)&&(!PalCvt)) Vid->SetAllPal(Pal.Cols,Vid);
#endif
#ifndef   GENINPUT
  decode();
#else
  Inp.Cursor->Off();
  decode();
  Inp.Cursor->On();
#endif
  return(True);
  }

void GIFImage::decode(void)
  {
  uint     PktLen  = 0;
  uint     Code    = 0;
  boolean  done    = False;
  uchar    work[4];          // char read, to be translated to code

  Pos.x   = Pos.y  = 0;
  PktIdx  = 0;
  BitsUsed= 0;
  oldcode = 0;
  InitLZWTable();

  while ((PktLen=Data->Getc())==0) if (Data->EOFflag) return; // Find blk len.
  Input=(uchar*)Data->Peek(PktLen);
  if (Data->EOFflag) return;

  while(!done)
    {
    for (PktIdx=0;PktIdx<PktLen-3;)  // Go until we're 3 bytes away.
      {
      Code=GetCode();
#ifndef GRAPHICS
      printf("%d ",Code);
#endif
      if (Code==NumCodes+1) { done=True; break; }  // End of Image!
      if (Code==NumCodes)   { InitLZWTable(); continue; }
      if (Code<=lastcode)
        {
        done=ExpandCode(Code);         // Draw pattern associated w/ code
        oldcode=Code;
        }
      else
        {
        done=True;
        printf("Impossible Code!\n");
        break;
        }
      }

    if (!done)
      {
      uint Xtra = PktLen-PktIdx;
      for (PktIdx=0;PktIdx<Xtra;PktIdx++) work[PktIdx] = Input[PktIdx];          // Save the leftovers.
      Data->Skip(PktLen-(Xtra-1));   // Now just Xtra-1 data bytes, then the new packet.

      Input=(uchar*)Data->Peek(Xtra);

      while((Input[Xtra-1]==0)&&(!Data->EOFflag))  // Skip all the zero length packets.
        { Data->Skip(1); Input=(uchar*)Data->Peek(Xtra); }
      PktLen=Input[Xtra-1]+Xtra;                // Set the Packet Size.
      Input =(uchar*)Data->Peek(PktLen);        // Get a pointer to the Data;
      for (PktIdx=0;PktIdx<Xtra;PktIdx++) Input[PktIdx] = work[PktIdx];      // restore the leftovers.
      if (Data->EOFflag) done=True;
      }
    }
  }


// ----------------------------- GISimage ----------------------------------

boolean GISImage::Draw(Coord2d Pos)
  {
  if ((!Data->SetPos(PicStart))||(Vid==NULL)) return(False);
  if ((Pal.ColsUsed>0)&&(!PalCvt)) Vid->SetAllPal(Pal.Cols,Vid);
#ifndef GENINPUT
  decode();
#else
  Inp.Cursor->Off();
  decode();
  Inp.Cursor->On();
#endif
  return(True);
  }


void GISImage::decode(void)
  {
  uint     Code    = 0;
  boolean  done    = False;
  uchar    work[4];          // char read, to be translated to code
  uint     PktLen;

  Pos.x   = Pos.y  = 0;
  PktIdx  = 0;
  BitsUsed= 0;
  oldcode = 0;
  InitLZWTable();

  while(!done)
    {
    PktLen=30005;
    Input=(uchar*)Data->Peek(PktLen);   // Remember the Pktlen is modified!
    if (Data->EOFflag) return;
    for (PktIdx=0;PktIdx<PktLen-3;)      // Go until we're 3 bytes away.
      {
      Code=GetCode();
      if (Code==NumCodes+1) { done=True; break; }  // End of Image!
      if (Code==NumCodes)   { InitLZWTable(); continue; }
      if (Code<=lastcode)
        {
        done=ExpandCode(Code);         // Draw pattern associated w/ code
        oldcode=Code;
        }
      else {  done=True; printf("Impossible Code!\n");  break; }
      }
    Data->Skip(PktIdx);
    }
  }

//---------------------------------------------------------------------------

#ifdef STAND_ALONE

void TextMode(void)
  {
  union REGS r;

  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }

FileRead     GifFile;
GIFImage     *Im=NULL;
VideoMode    *Vid=NULL;
VideoMode    *Modes[20];
int          LastMode      = 5;
ulongi       InterpreteCnt = 1;
ulongi       XtraCnt       = 1;
Coord2d      Zero(0,0);


boolean      ModeFit       = False;
boolean      Interactive   = True;
int dofindfile(char path[80],char *filespec,char *result);
static boolean CopyFile(const char *From,const char *To);

char         GifPath[121]="";
char         GifName[121]="";
char         Temp1[121]  ="";
char         Driver[121] ="";


void main(int argc,char *argv[])
  {
  int          ModeNum    = 0;
  VideoDriver *Drvr;
  if (argc>3)
    {
    if (strcmpi(argv[3],"fit")==0) { ModeNum=0; ModeFit=True; }
    else ModeNum=argv[3][0]-'0';
    }
  else { ModeNum=0; ModeFit=False; }

  strcpy(Driver,argv[0]);
  for (int loop=strlen(Driver);((loop>=0)&&(Driver[loop]!='\\'));loop--);
  loop++; Driver[loop]=0;

  if (argc>2)
    {
    if (strcmp(argv[2],"-NonInteractive") == 0)
      {
      Interactive=False;
      strcat(Driver,"vga.drv");
      }
    else
      {
      strcat(Driver,argv[2]); strcat(Driver,".drv");
      printf("Driver: %s",Driver);
      bioskey(0);
      }
    }
  else        strcat(Driver,"vga.drv");

  if ((argc<2)&&(Interactive))
    {
    printf("Useage : seegif <directory\\ or file> [driver] [0...5,fit]\n");
    printf("Example: seegif e:\\picture\\animals\\ fit\n");
    printf("\n\nIf a directory and no file is specified, seegif will show every file in\nthe specified directory, and every file in its subdirectories.\n");

    printf("\nFor the 'driver' argument, choose:\n   vga      : Standard vga driver.\n   vesa     : Driver for any VESA compatible card.\n   stlth64  : Diamond Stealth 64 card.\n");
    printf("The third argument is what video mode to use.  0 is 320x200.  Higher numbers\nare super vga modes.  Your card may not have all 5 modes.\n");
    printf("\n'fit' will check the GIF file's size, and choose the video mode\nwhich is closest to (but larger than) the GIF.\n");
    printf("Many images may still be bigger then the card's biggest video mode.\n");
    printf("If this occurs you will see the upper left corner of the GIF.\n");
    exit(1);
    }

  if ((Drvr=LoadVideoDriver(Driver))==NULL) { printf("Error Loading Driver %s.\n",Driver); return; }

  strcpy(GifPath,argv[1]);
  for (loop=strlen(GifPath);((loop>=0)&&(GifPath[loop]!='\\'));loop--);
  loop++;
  strcpy(GifName,&GifPath[loop]);
  GifPath[loop]=0;
  if (GifName[0]==0) strcpy(GifName,"*.gif");

  LastMode=Drvr->NumModes;
  if ((ModeNum>LastMode)||(ModeNum<0))
    { printf("This Mode not Supported!\n"); exit(1); }

  if (Interactive)
    {
    if (!ModeFit) printf("\n\n\nUsing Driver Mode #%d\n",ModeNum);
    else          printf("\n\n\nScreen Fitting enabled.\n");
    printf("\nLooking at:%s%s\n\n\n",GifPath,GifName);
    printf("Q     : Skip to the next directory.\n<ESC> : Quit.\nG     : Copy this picture to the current directory (G is for 'Get').\n\n  <Space> or <Return> will go to the next image.\n");
    printf("\nHit a Key!");
    bioskey(0);
    }

  Im=new GIFImage;
  if (Im==NULL) { printf("Out of memory!\n"); exit(1); }

/*  if (ModeFit)
    {
    for (uint i=0;i<=LastMode;i++) Modes[i]=Drvr->GetMode(i);
    Vid=Modes[LastMode];
    }
  else */

  Vid=Drvr->GetMode(ModeNum);

#ifdef GRAPHICS
  Vid->SetMode(Vid);
#endif
  dofindfile(GifPath,GifName,Temp1);
  TextMode();
  }

void QuitProg(void)
  {
  TextMode();
  printf("Done!  See images.ok to see names of the stuff U looked at.\n");
  exit(1);
  }


char str[81];
char str1[81];
boolean Copied  = False;
int     NoPause = 1;

int dofindfile(char path[80],char *filespec,char *result)
  {
  static int   addon = 0;
  int          ctr   = 0;
  int          ret   = 0;
  struct ffblk f;
  char         pf[81];

  strcpy (pf,path);
  strcat (pf,filespec);

     // find all files in this directory
  ret = findfirst(pf,&f,15);
  while(ret==0)
    {
    sprintf(str,"%s%s\0",path,f.ff_name);

    GifFile.Open(str);
    if (Im->Startup(GifFile,Vid))
      {
#ifdef GRAPHICS
      if (ModeFit)
        {
        uint UseMode=0;
        while ((UseMode<LastMode)&&((Im->Size.x>Modes[UseMode]->Size.x)||(Im->Size.y>Modes[UseMode]->Size.y))) UseMode++;
        if (UseMode==1) UseMode++;
        if (Modes[UseMode] != Vid) { Vid = Modes[UseMode]; Vid->SetMode(Vid); }
        Im->Vid = Vid;
        }
      Vid->Clear(Im->BkCol,Vid);
#endif
      Im->Draw(Zero);
      Copied = False;
      char key;
      uchar Done=!NoPause;
      while(!Done)
        {
        if ( (key=bioskey(NoPause))>0)
          {
          if ((NoPause==1)&&(key)) bioskey(0);  // Get the key iff we did a KeyPending bioskey
          key &=255;
          switch(key)
            {
            case 'P':
            case 'p':
              if (NoPause==1) NoPause=0; else NoPause=1;
              Done=True;
              break;
            case 'G':
            case 'g':
              Done=True;
              CopyFile(str,f.ff_name);
              Copied=True;
              break;
            case 13:
            case ' ': Done=True; break;
            case  27: QuitProg();
            case 'Q':
            case 'q': return(0);
            }
          }
        }
      }


/*      {
      FILE *fp=fopen("Images.ok","at+");
      if (fp!=NULL)
        {
        fputs(str,fp);
        sprintf(str1," Size(%d,%d). Colors %d. Ver:%s.",Im->Size.x,Im->Size.y,Im->Pal.ColsUsed,Im->GifVer);
        fputs(str1,fp);
        if (Copied) fputs("  Gotten!",fp);
        fputs("\n",fp);
        fclose(fp);
        }
      }
*/

    addon += strlen(result+addon)+1;
    ctr++;                 // ctr is the # of files found total.
    ret=findnext(&f);
    }
  // Find all other directories
  strcpy (pf,path);
  strcat (pf,"*.");
  f.ff_attrib=FA_DIREC;

  Vid->Clear(0,Vid);

//  ret = findfirst(pf,&f,16);
  ret = 1;
  while (ret==0)
    {
    if ((f.ff_name[0]!='.')&(f.ff_attrib==FA_DIREC))
      {
      sprintf(pf,"%s%s\\",path,f.ff_name);
      if (Interactive) printf("New Directory:%s.\n",pf);
      ctr+=dofindfile(pf,filespec,result);
      }
    ret = findnext(&f);
    }
  return(ctr);  // Fn returns # of file names found
  }

static boolean CopyFile(const char *From,const char *To)
  {
  char str[81];
  char worked=True;
  char far *mem=NULL;
  unsigned int startat=64512;
  unsigned int BytesRead=0;
  FILE *fp_in,*fp_out;
  int attr=0;
  char w[1000];
  char size;

  if ((fp_in=fopen(From,"rb")) == NULL)   { return(False); }
  else if ((fp_out=fopen(To,"wb"))==NULL) { fclose(fp_in); return(False); }
  else
    {  
    while((mem==NULL)&&(worked))
      { 
      startat-=512; 
      if (startat>1024) mem=(char far *) farmalloc(startat); 
      else worked=False;
      }
    if (worked) do
      {
      BytesRead=fread(mem,sizeof(char),(size_t) startat,fp_in);
      if (fwrite(mem,sizeof(char),(size_t) BytesRead,fp_out)<BytesRead) worked=False;
      } while (BytesRead==startat); // if not equal, an EOF has occured. 
    fclose(fp_in);
    fclose(fp_out);
    farfree(mem);
    }
  return(worked);
  }

#endif
