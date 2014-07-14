// A class of images and their derived formats
// July 15,94                             Copyright(C) 1994 G. Andrew Stone

#include "gen.h"
#include "coord2d.hpp"
#include "viddrive.hpp"
#include "fileclss.hpp"

typedef enum
  {
  NoIm   = 0,
  RawIm,
  RawRGBIm,
  GIFIm,
  GISIm,
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
  virtual boolean Draw    (Coord2d Pos) = 0;
  virtual boolean Get     (Box2d Scrn) {return(False);};
  virtual boolean Startup (FileRead &gif, VideoMode *v,Palette *CurPal=NULL) = 0;
  virtual boolean Cleanup (void)        = 0;
  };


#define MAXLZWINDEX 4097
class LZWTable
  {
  public:
  int   code;     // Position of next entry in chain
  Pixel Pix;      // Character to add to string
  uchar first;    // Character used to get tab.next for next code
  };

struct GIFScreenDescriptor
  {
  public:
  Coord2d Pos;
  Coord2d Size;
  uchar   Flag;
  };


typedef struct
  {
  char Sig[3];
  char Ver[3];
  Coord2d Size;
  uchar   Flag;
  uchar   BkCol;
  uchar   AspectRatio;
  int Validate(void)
    {
    if ((Sig[0]=='G')&&(Sig[1]=='I')&&(Sig[2]=='F')) return(True);
    return(False);
    }
  } GIFHeader;


class GIFfamily:public Image
  {
  protected:
  uchar    Interlaced;

  uchar    PixelSize;     // Bits in a single pixel.


  int      nextlimit;     // Max # the CodeSize bits can encode.
  uchar    CodeSize;      // Current # of bits in a code.
  uchar    BitsUsed;      // # of bits in cur byte that were used in the prev. code.
  int      NumCodes;
  int      lastcode;      // first unused entry in the LZW table.
  int      oldcode;       // place holder for tab[],int for next tab[] entry
  boolean  FirstCode;     // is this the first code since an InitLZWtable?

  Coord2d  Pos;
  uchar   *Input;

  uint     GetCode(void);
  void     SkipExtension(FileRead *gif);
  boolean  ExpandCode(int coded);
  boolean  DrawData(uint size,uint len);   // draw one every 'skip' pixels when sent
  void     InitLZWTable(void);

  public:
  char      GifVer[4];
  longi     PicStart;

  LZWTable *tab;
  FileRead *Data;
  Pixel     BkCol;
  uint      PktIdx;

  boolean   PalCvt;
  boolean   MyTable;    // Did I alloc the LZWtable?

  Pixel     ColXlat[256];
  Palette   Pal;

  void      PalMatch(Palette *p);
  void      PrintStats(void)
    { printf(" Size(%d,%d). Start Off %lu.  Num Colors %d. PixelSize %d. Ver:%s",Size.x,Size.y,PicStart,Pal.ColsUsed,PixelSize,GifVer);    }

  GIFfamily()    { MyTable=False; tab=NULL; Pal.ColsUsed=0; Data=NULL;  }
  ~GIFfamily()   { if ((MyTable)&&(tab!=NULL)) delete tab; tab=NULL; }
  virtual boolean Validate(GIFHeader *h) = 0;
  boolean Startup (FileRead &gif,VideoMode *v,Palette *CurPal=NULL);
  };

class GIFImage:public GIFfamily
  {
  void       decode(void);

  public:
  virtual boolean Draw    (Coord2d Pos);
  virtual boolean Cleanup (void) {return(True);};
  virtual boolean Validate(GIFHeader *h)
    {
    if ((h->Sig[0]=='G')&&(h->Sig[1]=='I')&&(h->Sig[2]=='F')) return(True);
    return(False);
    }
  };

class GISImage:public GIFfamily
  {
  void       decode(void);

  public:
  virtual boolean Draw    (Coord2d Pos);
  virtual boolean Cleanup (void) {return(True);};
  virtual boolean Validate(GIFHeader *h)
    {
    if ((h->Sig[0]=='G')&&(h->Sig[1]=='I')&&(h->Sig[2]=='S')) return(True);
    return(False);
    }
  };



