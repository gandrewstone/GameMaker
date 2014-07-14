#pragma  inline
#define  GRAPHICS
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

#define  BUFMAX 2000
static   uchar    FastBuf[BUFMAX];


void main(int argc,char *argv[])
  {
  uchar *Data=NULL;
  if (argc<3)
    {
    printf("Syntax: GifCvt File.in file.gis\n");
    exit(1);
    }

  GIFImage   G;
  FileRead  In (argv[1]);
  FileWrite Out(argv[2]);

  printf("%s -> %s\n",In.Name,Out.Name);

  if (!G.Startup(In,NULL))
    {
    printf("Invalid GIF file!\n");
    return;
    }
  if (!In.SetPos(0)) return;
  if ( (Data    = In.Peek(G.PicStart))==NULL)
    {
    printf("Can't load the whole header in!\n");
    return;
    }
  Data[2] = 'S';

  Out.Write(Data,G.PicStart);    // Write the header.
  In.Skip  (G.PicStart);

  uint PktLen;
  uchar PktOut;
  while(!In.EOFflag)             // Remove all of the Length fields
    {
    while ((PktLen=(uint) In.Getc())==0) if (In.EOFflag) break; // Find packet len.
    printf("InPos:%lu OutPos:%lu  PktLen:%d\n",In.CurPos,Out.CurPos,PktLen);
    Data=(uchar*)In.Peek(PktLen);
//    PktOut=PktLen;
//    Out.Write(&PktOut,sizeof(uchar));
    Out.Write(Data,PktLen);
    In.Skip(PktLen);
    }

  In .Close();
  Out.Close();
  printf("Conversion completed!\n");
  }

