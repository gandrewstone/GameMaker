/* Header file for any program that uses bit-blk animation */
#include <alloc.h>
#include "gen.h"
#include "Coord2d.hpp"
typedef uchar Pixel;

#ifndef BLEN
#define BLEN    20   //  Length and width of each block
#define BWID    20   
#endif
#define BACKBL 150   //  Number of blocks in background block file
#define MONBL  100   //  Number of blocks in a monster file
#define CHARBL  50   //  Number of blocks in the character file
#define BACKBLK  0   //  Background block number

#define NOMONSTER 0xFF

#define SOLTOP 1
#define SOLBOT 4
#define SOLRIG 2
#define SOLLEF 8  
#define AVEGRAV 30

#define MAXEFFECTS 5


extern int getblktype(void);
extern int loadblks(int btype,char *buffer);
extern int saveblks(int btype,char *buffer);
extern void freadfar(unsigned char far *address,int numbytes,int numtimes,FILE *fp);
extern void fwritefar(unsigned char far *address,int numbytes,int numtimes,FILE *fp);
extern void drawblocpiece(int x,int y,int bx,int by,int len,int wid,int bloc);
extern void scroll_list(int dir,int x,int y,int listlen,int totalen,int *current,int (*dra)(int));
extern void drawlist(int x,int y,int listlen,int totalen,int *current,int (*dra)(int));
extern void vdrawlist(int x,int y,int listlen,int totalen,int *current,int (*dra)(int));
extern void vscroll_list(int dir,int x,int y,int listlen,int totalen,int *current,int (*dra)(int));

extern "C"
  {
  extern int drawblk(int x,int y,unsigned char  *bloc);
//  extern int drawcbloc(int x,int y,unsigned char *bloc);
  extern int getblk(int x,int y,unsigned char   *bloc);
  }

typedef struct
  {
  unsigned char p[BLEN][BLEN]; // Block picture array (Y,X)
  unsigned char Birthmon;   // Monster to appear (2B Imp)
  int Birthtime;            // Time for monster to appear (high bit used as
                            //    flag by playgame).
  unsigned char Xtra[2];    //  X-tras
  unsigned char solid;      // 1-top 2-right 4-bottom 8-left
  unsigned char hitpt;      // Same as above, but can hurt the character
  int scorepts;             // If touched accumlate points
  int chhitpt;              // Amount to change character points
  unsigned char nextbl;     // Change block after time, nextbl=index self
  int ntime;                // Change block time value, 0=never change
  unsigned char touchbl;    // Change block when touched, touchbl=index self
  char grav;                // bit 1 suck up, 2 right, 4 bottom, 8 left
  unsigned char obj;        // if character can pick it up
  unsigned char effect;     // a # showing what repetition counter affected
  int eamt;                 // how much effect the repetition counter has
  } OLDblkstruct;

class BitMap
  {
  public:
  Coord2dchar Size;
  Pixel P[1];

  void* operator new (size_t Type_Size,int x,int y)
    {
    void *This;
    This = ALLOC(sizeof(BitMap)+(sizeof(Pixel)*x*y));
    if (This==NULL) return(NULL);
    ((BitMap*)This)->Size.x=x;
    ((BitMap*)This)->Size.y=y;
    return(This);
    }


  Pixel& operator() (int x,int y)
    {
    return(*(P+(x+(y*Size.x))));
    }
  Pixel& operator() (Coord2dchar Pos)
    {
    return( *(P+(Pos.x+(Pos.y*Size.x))));
    }
  Pixel& operator() (Coord2d Pos)
    {
    return( *(P+(Pos.x+(Pos.y*Size.x))));
    }

  void Draw(Coord2d Pos) { drawblk(Pos.x,Pos.y,P);}
  void Get (Coord2d Pos) { drawblk(Pos.x,Pos.y,P);}
  void Draw(const int x,const int y) { drawblk(x,y,P); }
  void Get (const int x,const int y) { drawblk(x,y,P); }

  void Clear(const Pixel &p)
    {
    if (this!=NULL) for (uint i=0;i<((uint) Size.x)*((uint) Size.y);i++) P[i]=p;
    }

  };


class Effects // Size 16
  {
  public:
  uchar   ChangeTo;         // 0xFF = none;
  int     Hitpts;           // hurt/help character when touched.
  int     Score;            // If accumulate points.
  uchar   Counter;          // a # showing what repetition counter affected
  int     CntrAmt;          // how much effect the repetition counter has
  uint    Text;             // Ptr to text. 0xFFFF = notext.
  uint    Propagate;        // do same to block pointed to in here. 0xFFFF= no block.
  uint    Commands;         // index into commandlist 0xFFFF = none.
  uchar   BirthMon;         // Monster to appear (2B Imp)
  private:
  uchar   Xtra;
  public:

  void    Clear(void)
    {
    ChangeTo  = 0xFF;
    Hitpts    = 0;
    Score     = 0;
    Counter   = 1;
    CntrAmt   = 0;
    Text      = 0xFFFF;
    Propagate = 0xFFFF;
    Commands  = 0xFFFF;
    BirthMon  = 0xFF;
    Xtra      = 0;
    }
  };


class BlockClass              // Size 128.
  {
  public:
  uchar   PwrLvl;             // Monsters/characters < this do not touch.
  uchar   HitPts;             // Block strength.
  uchar   Solid;              // 1-top 2-right 4-bottom 8-left.
  uchar   obj;                // if character can pick it up
  Coord2dchar Grav;         // Gravity vector.
  uint    Time[2];            // Change block after time value, 0=never change
  BitMap  *p;
  char    Resist;             // How much character slows down going thru.
  char    grav;               // REMOVE THIS - TEMP COMPILING PURPOSES.
  //uchar   Xtra[1];            // make this a 16 byte record.
  Effects Effect[MAXEFFECTS]; // 0=Time0 1=Time1 2=touch 3=destroyed 4=Monster Touch.
  private:
  uchar   Extra[(16*2)];
  public:

  BlockClass& operator = (BlockClass &b)
    {
    // Copy all except the BitMap pointer;
    return(*this);
    }


  BlockClass& operator = (OLDblkstruct &b)
    {
    unsigned char p[BLEN][BLEN]; // Block picture array (Y,X)

    Clear();
    Effect[1].BirthMon = b.Birthmon;
    Time[1]            = b.Birthtime;
    Time[0]            = b.ntime;
    Effect[0].ChangeTo = b.nextbl;
    Effect[2].ChangeTo = b.touchbl;
    Effect[2].Hitpts   = b.chhitpt;
    Effect[2].Score    = b.scorepts;
    Effect[2].Counter  = b.effect;
    Effect[2].CntrAmt  = b.eamt;
    Solid              = b.solid;
    obj                = b.obj;
    if (b.grav) Resist = 0;
    if (b.grav|SOLTOP)
      {
      if (b.grav|SOLBOT) Grav.y=0;
      else Grav.y= -AVEGRAV;
      }
    if (b.grav|SOLBOT) Grav.y = AVEGRAV;

    if (b.grav|SOLLEF)
      {
      if (b.grav|SOLRIG) Grav.x=0;
      else Grav.x= -AVEGRAV;
      }
    if (b.grav|SOLRIG) Grav.x = AVEGRAV;
    return(*this);
    }

  void Clear(void)
    {
    ClearData();
    p        = NULL;
    }
  void ClearData(void)
    {
    PwrLvl   = 0xFF;
    HitPts   = 0xFF;
    Solid    = (SOLTOP|SOLLEF|SOLRIG|SOLBOT);
    Time[0]  = 0;
    Time[1]  = 0;
    obj      = FALSE;      // if character can pick it up
    Grav.x   = 0;
    Grav.y   = 0;
    Resist   = 50;
    for (int i=0;i<MAXEFFECTS;i++) Effect[i].Clear();
    for (i=0;i<(16*2);i++)         Extra[i]=0;
    }

  };
