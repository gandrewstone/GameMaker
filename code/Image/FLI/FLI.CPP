// --------------------------------
//  Copyright 1994 G. Andrew Stone
//  Not public domain!
// --------------------------------

#include <bios.h>
#include <dos.h>
#include <conio.h>
#include "gen.h"
#include "debug.hpp"
#include "fileclss.hpp"
#include "viddrive.hpp"

//#define STANDALONE

Debug Debugging;
char  LogStr[101];

#include "Fli.h"

boolean FrameHeader::Validate(void)
    {
    if (ID==0xF1FA) return(True);
    return(False);
    }

/*     The compression schemes are all byte-oriented.  If the
compressed data ends up being an odd length a single pad byte is
inserted so that the FLI_COPY's always start at an even address
for faster DMA.
*/


typedef struct
  {
  long int   Size;
  ChunkTypes Type;
  } ChunkHeader;

typedef struct
  {
  int NumPackets;
  } ColorMapHeader;


typedef struct
  {
  unsigned char SkipCols;
  unsigned char NumCols;
  } ColorPacket;


//typedef RGBdata ColorData;

typedef struct
  {
  int TopLines;
  int NumChanged;
  } LineCmpHeader;

typedef struct
  {
  char Num;
  } RunLengthPacket;

void DoFLI(FileRead &FliFile);

boolean FLI::StepInit(void)
  {
  File.Get(&Head,sizeof(FLIHeader));
  if (!Head.Validate()) { DEBUG("Invalid FLI.\n"); return(False); }
  FrameIdx=0;
  return(True);
  }


boolean FLI::Step(void)
  {
  if (FrameIdx>=Head.NumFrames) return(False);
  File.Get(&CurFrame,sizeof(FrameHeader));
  FrameIdx++;
  return(CurFrame.View(Head,File,Vid,Pos));
  }


boolean FLI::View(void)
  {
  boolean result=True;
  char    s[51];

  File.Get(&Head,sizeof(FLIHeader));
  if (!Head.Validate()) { DEBUG("Invalid FLI.\n"); return(False); }
  for(uint Frame=0;((Frame<Head.NumFrames)&&(result));Frame++)
    {
    File.Get(&CurFrame,sizeof(FrameHeader));
    result=CurFrame.View(Head,File,Vid,Pos);
    //gotoxy(1,10);
    sprintf(LogStr,"Frame: %d Out of %d\n",Frame,Head.NumFrames);
    LOG(LogStr);
    for (uint Delay=0;Delay<Head.FrameSpd;Delay++)
      {
      while((inportb(0x3da)&8)>0);  // Wait out retrace
      while((inportb(0x3da)&8)==0);  // Wait for retrace
      }
    }
  return(result);
  }

void DoColorChange(uchar *Data,VideoMode *Vid)
  {
  uint NumPackets=0;
  char PacketLen;

  if (Data==NULL) return;

  NumPackets = *((int *) Data);
  Data+=2;

  sprintf(LogStr,"Col: Numpackets %d",NumPackets);
  LOG(LogStr);

  uchar SkipCols=0;
  for (int Packets=0;Packets<NumPackets;Packets++)
    {
    uint  NumCols;

    SkipCols+= *Data;
    NumCols  = (*(Data+1));
    Data+=2;
    if (NumCols==0) NumCols=256;
    for (int cnt=0;cnt<NumCols;cnt++,Data+=sizeof(RGBdata))
      Vid->SetPal(cnt+SkipCols,*((RGBdata*)Data),Vid);
    SkipCols+=NumCols;
    }
  }


void DoRunLength(char *Data,VideoMode *Vid,Coord2d Start)
  {
  Coord2d Pos(0,0);
  uchar NumPackets=0;
  char PacketLen;

  if (Data==NULL) return;

  for (uint Lines=0; Lines<200; Lines++)
    {

    NumPackets=*Data;
    Data++;

    for(uchar Packets=0; Packets<NumPackets;Packets++)
      {
      PacketLen=*Data; Data++;

      if (PacketLen<=0)
        {
        PacketLen*=-1;
        Vid->RepCols (Start+Pos,PacketLen,Data,Vid);
        sprintf(LogStr,"RepCols((%d,%d),%d)\n",(Start+Pos).x,(Start+Pos).y,PacketLen);
        LOG(LogStr);
        Data+=PacketLen;
        Pos.x+=PacketLen;
        }
      else
        {
        Vid->Rep1Col(Start+Pos,PacketLen,*Data,Vid);
        sprintf(LogStr,"Rep1Col((%d,%d),%d,%d)\n",(Start+Pos).x,(Start+Pos).y,PacketLen,*Data);
        LOG(LogStr);
        Data++;
        Pos.x+=PacketLen;
        }
      }
    Pos.y++;
    Pos.x=0;
    }
  }

typedef struct
  {
  uint SkipNum;
  uint ChangeNum;
  } LineCmpPacket;

int Dummy(void);

static void DoLineCompress(char *Data,VideoMode *Vid,Coord2d Pos)
  {
  uint x=0;
  LineCmpPacket *P;
  uint NumPackets=0;

  if (Data==NULL) { DEBUG("Data=NULL!"); return; }

  P = (LineCmpPacket*) Data;
  Data+=sizeof(LineCmpPacket);

  for (uint Lines=P->SkipNum; Lines<P->ChangeNum+P->SkipNum; Lines++,x=0)
    {
    NumPackets= (int) (*Data); Data++;

    for(uint Packets=0; Packets<NumPackets;Packets++)
      {
      int PacketLen=*(Data+1);
      x+=*((uchar*) Data);
      Data+=2;
      if (PacketLen>=0)
        {
        Vid->RepCols(Pos+MakeCoord2d(x,Lines),PacketLen,Data,Vid);
        Data+=PacketLen;
        x+=PacketLen;
        }
      else
        {
        PacketLen*=-1;
        Vid->Rep1Col(Pos+MakeCoord2d(x,Lines),PacketLen,*Data,Vid);
        Data++;
        x+=PacketLen;
        }
      }
    }
  }


boolean FrameHeader::View(FLIHeader &H,FileRead &f,VideoMode *Vid,Coord2d Pos)
  {
  static int Frame=0;
  ChunkHeader c;
  int Chunk;
  if (Validate()==False) { DEBUG("Invalid Frame.\n"); return(False); }
  uint MemAmt;
  char s[61];
  sprintf(s,"New Frame: %d Size:%5lu Chunks:%u\n",Frame++,Size,NumChunks);
  DEBUG(s);

  for (Chunk=0;Chunk<NumChunks;Chunk++)
    {
    f.Get(&c,sizeof(ChunkHeader));

    sprintf(s,"  New Chunk: Size:%5lu ",c.Size,c.Type);
    DEBUG(s);

    MemAmt=c.Size-sizeof(ChunkHeader);
    switch (c.Type)
      {
      case ColorMap:
           DEBUG("Color Map\n");

           DoColorChange((uchar *) f.Peek(MemAmt),Vid);
           f.Skip(c.Size-sizeof(ChunkHeader));
           break;
      case LineCompress:
           DEBUG("Line Compress\n");
           DoLineCompress((char *) f.Peek(MemAmt),Vid,Pos);
           f.Skip(c.Size-sizeof(ChunkHeader));
           break;
      case AllBlack:
           DEBUG("All Black\n");
           Vid->BoxFill(Pos,Pos+MakeCoord2d(319,199),0,Vid);
           break;
      case RunLength:
           DEBUG("Run Length\n");
           DoRunLength((char *) f.Peek(MemAmt),Vid,Pos);
           f.Skip(c.Size-sizeof(ChunkHeader));
           break;
      case Copy:
           DEBUG("Copy\n");
           {
//           f.Get(((void *)0xA0000000),64000);
           uchar *Pixels = f.Peek(64000);
           memcpy(((void *)0xA0000000),Pixels,64000);
           f.Skip(64000);
           }
           break;
      default:
           DEBUG("Invalid chunk type\n");
           f.Skip(c.Size-sizeof(ChunkHeader));
           exit(1);
           return(False);
      }
    }
  return(True);
  }


#ifdef STANDALONE

#define TMODE 3
void TextMode(void)
  {
  union REGS r;
  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }

FLI          f;
void main(int argc,char *argv[])
  {
  FLIHeader   *Head;
  int          ModeNum=0;
  char         Driver[51]="vga.drv";
  VideoDriver *Drvr;
  uint         FLIHdrSize = sizeof(FLIHeader);

  if (argc>2) ModeNum=argv[2][0]-'0';
  if (!f.File.Open(argv[1])) { printf("Cannot open fli file %s.\n",argv[1]); return; }
  if ((Drvr=LoadVideoDriver(Driver))==NULL) { printf("Error Loading Driver vga.drv.\n"); return; }
  f.Vid=Drvr->GetMode(ModeNum);

  Head = (FLIHeader*) f.File.Peek(FLIHdrSize);
  Head->Print();
  if (Head->Validate()==False)
    {
    printf("Not a valid 256-color FLI\n");
    return;
    }
  f.Vid->SetMode(f.Vid);
  f.Pos.Set(0,0);
  if (f.View()) bioskey(0);
  TextMode();
  }
#endif

