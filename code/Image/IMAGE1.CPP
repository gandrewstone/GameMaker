#define NEWCODE
#define GRAPHICS
#ifdef  NEWCODE
#pragma inline
#endif
#include <dos.h>
#include <bios.h>
#include <dir.h>
#define TMODE 3
#include "gen.h"
#include "pal.h"
#include "lztimer.h"
unsigned long int TimeElapsed;
#include "coord2d.hpp"
#include "viddrive.hpp"
#include "fileclss.hpp"

#define OUTBUFLEN 1000

extern uint _stklen = 8000U;

class Box2d
  {
  Coord2d ul,lr;
  };

typedef enum
  {
  NoIm   = 0,
  RawIm,
  RawRGBIm,
  GIFIm,
  TargaIm,
  PCXIm,
  GSRIm,
  FLIIm,
  } ImageTypes;


class Image
  {
  public:
  Coord2d      Size;
  char         ColDepth;
  ImageTypes   Type;
  VideoMode   *Vid;
  virtual boolean Draw    (Box2d Scrn,Box2d Imag) =0;
  virtual boolean Get     (Box2d Scrn,Box2d Imag) {return(False);};
  virtual boolean Startup (FileRead *gif,VideoMode *v) = 0;
  virtual boolean Cleanup (void) = 0;
  };

class LZWTable
  {
  public:
  int code;               // Position of next entry in chain
  unsigned char next;     // Character to add to string
  unsigned char first;    // Character used to get tab.next for next code
  };

class GIFHeader;

class GIFImage:public Image,public Palette
  {
  uint     NumCols;
  uchar    PixelSize;
  uchar    Interlaced;
  uchar    Flag;

  uchar    BitsUsed;
  uchar    CodeSize;
  uchar    outcol[OUTBUFLEN];   // contains string of output characters
  uchar   *Input;
  uint     PktIdx;
  Coord2d  Pos;

  boolean  FirstCode;
  FileRead *Data;
  LZWTable tab[4097];           // LZW string table.
  int      nextlimit;           // greater. codestart--# of bits per code
  int      lastcode,oldcode;    // place holder for tab[],int for next tab[] entry

  GIFHeader& operator = (GIFHeader &h);
  void       SkipExtension(FileRead *gif);
  boolean    ExpandCode(int coded);
  inline int CreateCode(unsigned char from[]);
  uint       GetCode(void);
  boolean    DrawData(int size);   // draw one every 'skip' pixels when sent
  void       InitLZWTable(void);
  void       decode(void);
  public:
  uchar    BkCol;
  virtual boolean Startup (FileRead *file,VideoMode *v);
  virtual boolean Draw    (Box2d Scrn,Box2d Imag);
  virtual boolean Cleanup (void) {return(True);};
  };

class GIFHeader
  {
  char Sig[3];
  char Ver[3];
  public:
  Coord2d Size;
  uchar   Flag;
  uchar   BkCol;
  uchar   AspectRatio;
  int Validate(void)
    {
    if ((Sig[0]=='G')&&(Sig[1]=='I')&&(Sig[2]=='F')) return(True);
    return(False);
    }
  };

GIFHeader& GIFImage::operator = (GIFHeader &h)
    {
    Size = h.Size;
    BkCol= h.BkCol;
    Flag = h.Flag;
    return(h);
    }

void TextMode(void)
  {
  union REGS r;

  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }

FileRead     GifFile;
GIFImage     *Im;
VideoMode    *Vid;
Box2d        a,b;
unsigned long int     InterpreteCnt = 1;
unsigned long int     XtraCnt       = 1;

int dofindfile(char path[80],char *filespec,char *result);

void main(int argc,char *argv[])
  {
  char         GifName[81]="e:\\";
  int          ModeNum=0;
  char         Driver[51]="vesa.drv";
  VideoDriver *Drvr;
  char         Temp[81]="*.gif",Temp1[81];

  if (argc>2) ModeNum=argv[2][0]-'0';
  if (argc>1) strcpy(GifName,argv[1]);
  printf("Using Driver Mode #%d\n",ModeNum);
  printf("Image:%s\n",GifName);
  bioskey(0);

  if ((Drvr=LoadVideoDriver(Driver))==NULL) { printf("Error Loading Driver.\n"); return; }
  Vid=Drvr->GetMode(ModeNum);

#ifdef GRAPHICS
  Vid->SetMode(Vid);
  Vid->Clear(20,Vid);
#endif
  Im = new GIFImage;

//  ZTimerOn();
  dofindfile(GifName,Temp,Temp1);
//  ZTimerOff();
//  TimeElapsed+=ZGetTime();
  TextMode();
  printf("Time!: %lu\n",TimeElapsed);
  printf("XtraCnt/Interprete = %lu/%lu = %lu\n",XtraCnt,InterpreteCnt,XtraCnt/InterpreteCnt);
  }

boolean GIFImage::Startup (FileRead *gif,VideoMode *v)
  {
  uchar temp;
  int   j;
  GIFHeader *head;
  Data=NULL; Vid=NULL;

  if (gif==NULL) return(False);

  head=(GIFHeader*) gif->Peek(sizeof(GIFHeader));
  if (!head->Validate()) return(False);
  *this = *head;
  gif->Skip(sizeof(GIFHeader));

  NumCols = 2 << (Flag & 0x07);

  if (Flag & 0x80)  // Load a palette->color number mapping.
    {
    if (!gif->Get(Pal,sizeof(RGBdata)*NumCols)) return(False);
    for (j=0;j<NumCols;j++) Pal[j]>>=2; // make values range to 63, not to 256
    }

  temp=gif->Getc();
  while (temp!=',')
    {
    if (temp=='!') SkipExtension(gif);
    else return(False);
    }

  // Keep in mind that .GIFs can have multiple pictures in them.
  // This gets the info for the first.

  gif->Get(&Pos,sizeof(Coord2d));   // image upper left corner in screen.
  gif->Get(&Size,sizeof(Coord2d));  // image size.

  Flag = gif->Getc();
  Interlaced = (Flag & 0x40);

  if (Flag&0x80)  // New Palette - specific to this picture
    {
    NumCols = 2 << (Flag & 0x07);
    if (!gif->Get(Pal,sizeof(RGBdata)*NumCols)) return(False);
    for (j=0;j<NumCols;j++) Pal[j]>>=2; // make values range to 63, not to 256
    }
  PixelSize = gif->Getc(); // Number of bits to read for _trans after initing table
  InitLZWTable();
  Data = gif;
  Vid  = v;
  return(True);
  }

void GIFImage::SkipExtension(FileRead *gif)
  {
  char jumper=True;
  register int j;

  gif->Getc();           // throw away this byte
  while(jumper)
    {
    jumper = gif->Getc();
    for (j=0;j<jumper;j++) gif->Getc();
    }
  }

uint GIFImage::GetCode(void)
  {
  uint Retrn;
#ifdef NEWCODE
  asm {
  IDEAL
  push   si
  push   di
  push   ds
  push   es

  lds    si,this
  les    di,[ds:si.(class GIFImage) Input]
  mov    bl,[ds:si.(class GIFImage) BitsUsed]
  mov    bh,[ds:si.(class GIFImage) CodeSize]

  mov    dx,[WORD PTR es:di]    ;// Load so dl is es:di and dh is es:di+1
  mov    ax,[WORD PTR es:di+1]
  mov    cl,bl
  shr    dx,cl
  shr    ax,cl
  mov    dh,al
                     ;// Check to see if > 15th bit is needed
  mov    ax,0FFFFFFFFh
  mov    cl,bh
  shl    ax,cl      ;// Now there are CodeSize+bitsused zeros in the bottom of ax
  not    ax         ;// Now codesizeBitsUsed ones.

  and    ax,dx       ;// ax is almost the # -- we need to shift it to 0 first.

  add    bl,bh       ;// BitsUsed += CodeSize

  mov    Retrn,ax
  @@TpLp:
  cmp    bl,8
  jl     @@SameByte
  sub    bl,8
  inc    [WORD PTR ds:si.(class GIFImage) PktIdx]
  inc    di
  jmp    @@TpLp
  @@SameByte:
  mov    [BYTE PTR ds:si.(class GIFImage) BitsUsed],bl
  mov    [WORD PTR ds:si.(class GIFImage) Input],di
  pop    es
  pop    ds
  pop    di
  pop    si
  MASM
  }
#endif
  return(Retrn);
  }

void GIFImage::decode(void)
  {
  uint     PktLen  = 0;
  uint     Code    = 0;
  boolean  done    = False;
  uchar    work[4];           // char read, to be translated to code

  Pos.x=0; Pos.y=0;
  PktIdx  = 0;
  BitsUsed= 0;

  while ((PktLen=Data->Getc())==0) if (Data->EOFflag) return; // Find blk len.
  Input=(uchar*)Data->Peek(PktLen);
  if (Data->EOFflag) return;

  while(!done)
    {
    for (PktIdx=0;PktIdx<PktLen-3;)  // Go until we're 3 bytes away.
      {
      Code=GetCode();
      if (Code==NumCols+1) { done=True; break; }  // End of Image!
      if (Code==NumCols)   { InitLZWTable(); continue; }
      done=ExpandCode(Code);         // Draw pattern associated w/ code
      oldcode=Code;
      }

    if (!done)
      {
      uint Xtra = PktLen-PktIdx;
      for (PktIdx=0;PktIdx<Xtra;PktIdx++) work[PktIdx] = Input[PktIdx];          // Save the leftovers.
      Data->Skip(PktLen-(Xtra-1));             // Now just Xtra-1 data bytes, then the new packet.

      Input=Data->Peek(Xtra);

      while((Input[Xtra-1]==0)&&(!Data->EOFflag))  // Skip all the zero length packets.
        { Data->Skip(1); Input=(uchar*)Data->Peek(Xtra); }
      PktLen=Input[Xtra-1]+Xtra;               // Set the Packet Size.
      Input = Data->Peek(PktLen);              // Get a pointer to the Data;
      for (PktIdx=0;PktIdx<Xtra;PktIdx++) Input[PktIdx] = work[PktIdx];      // restore the leftovers.
      if (Data->EOFflag) done=True;
      }
    }
  }


boolean GIFImage::ExpandCode(int coded)
  {
  int i=OUTBUFLEN;
  int properone;

  if (!FirstCode)
    {
    if (tab[coded].code==4096) properone = oldcode;
    else                       properone = coded;
    tab[lastcode].next=tab[properone].first;
    tab[lastcode].code=oldcode;
    tab[lastcode].first=tab[oldcode].first;
    lastcode++;
    if ( (lastcode==nextlimit)&&(CodeSize<12) )
      {
      CodeSize++;
      nextlimit<<=1;
      }

    while(coded>NumCols)
      {
      outcol[--i]=tab[coded].next;
      coded=tab[coded].code;
      }
    }
  else FirstCode=False;

  outcol[--i]=coded;
  XtraCnt=1234;
  boolean r=DrawData(i);
  XtraCnt=1;
  return(r);
//  return(False);
  }

boolean GIFImage::DrawData(int DataStart)
  {
  uint i;
  uint count=0;

  if (XtraCnt==1234)
    {
  for(i=DataStart;i<OUTBUFLEN;i+=count)
    {
    count = Min(OUTBUFLEN-i,Size.x-Pos.x);
#ifdef GRAPHICS
    Vid->RepCols(Pos,count,&(outcol[i]),Vid);
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
  }
  else printf("OH NO!\n");
  return(False);
  }

void GIFImage::InitLZWTable(void)
  {
  register int j;

  for (j=NumCols;j<4096;j++)
    {
    tab[j].code=4096;          // a code of LASTCOL signifies
    tab[j].next=0;             // a reinitialization of the
    tab[j].first=0;            // table (clear code)
    }                          // code LASTCOL+1 is an end of image code.

  for (j=0;j<NumCols;j++)
    {
    tab[j].code=0;
    tab[j].next=j;
    tab[j].first=j;
    }
  FirstCode=True;
  lastcode=NumCols+2;
  CodeSize=PixelSize+1;
  nextlimit=NumCols<<1;
  }


boolean GIFImage::Draw(Box2d Scrn,Box2d Imag)
  {
#ifdef GRAPHICS
  Vid->SetAllPal(Pal,Vid);
#endif
  decode();
  return(True);
  }

int dofindfile(char path[80],char *filespec,char *result)
  {
  static int addon=0;
  int ctr=0;
  struct ffblk f;
  int ret=0;
  char pf[80];

  strcpy (pf,path);
  strcat (pf,filespec);

     // find all files in this directory
  ret = findfirst(pf,&f,15);
  while(ret==0)
    {
    char str[81];
    sprintf(str,"%s%s\0",path,f.ff_name);
    GifFile.Open(str);
    if (Im->Startup(&GifFile,Vid))
      {
#ifdef GRAPHICS
      Vid->Clear(Im->BkCol,Vid);
#endif
      Im->Draw(a,b);
      }
    if (bioskey(1))
      {
      char key=bioskey(0)&255;
      switch(key)
        {
        case ' ': bioskey(0); break;
        case 'q': return(0);
        }
      }
    addon += strlen(result+addon)+1;
    ctr++;                 // ctr is the # of files found total.
    ret=findnext(&f);
    }
  // Find all other directories
  strcpy (pf,path);
  strcat (pf,"*.");
  f.ff_attrib=FA_DIREC;

  ret = findfirst(pf,&f,16);

  while (ret==0)
    {
    if ((f.ff_name[0]!='.')&(f.ff_attrib==FA_DIREC))
      {
      sprintf(pf,"%s%s\\",path,f.ff_name);
      ctr+=dofindfile(pf,filespec,result);
      }
    ret = findnext(&f);
    }
  return(ctr);  // Fn returns # of file names found
  }



