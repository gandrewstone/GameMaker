
#include "bitmap.h"

void BitMap::Draw (Coord2d Pos,Box2d Mask,VideoMode *v)
  {
  if (Address!=NULL)
    {
    Coord2d End;
    Mask.lr = Min(Mask.lr,Size);

    End.x   = Min(Mask.lr.x-Mask.ul.x,(v->Size.x-1)-(Pos.x+Mask.ul.x));
    End.y   = Min(Mask.lr.y+Pos.y,v->Size.y-1);

    Pos+=Mask.ul;
    if ((Pos<v->Size)&&(End.x>0))
      {
      Pixel *CurAddr=Address+(Mask.ul.x+(Mask.ul.y*LineSep));

      for (;Pos.y<=End.y;Pos.y++,CurAddr+=LineSep)
        v->RepCols(Pos,End.x,CurAddr,v);

      }
    }
  }

void BitMap::Draw (Coord2d Pos,VideoMode *v)
    {
    if (Pos<v->Size)
      {
      Coord2d End;
      Pixel *CurAddr;
      if (Pos.y+Size.y > v->Size.y) End.y=v->Size.y;       // The y end is WRT
      else                          End.y=Pos.y+Size.y;    // the VideoMode
      if (Pos.x+Size.x > v->Size.x) End.x=v->Size.x-Pos.x; // The x end is WRT
      else                          End.x=Size.x;          // this bitmap.

      for (CurAddr=Address;Pos.y<End.y;Pos.y++,CurAddr+=LineSep)
        v->RepCols(Pos,End.x,CurAddr,v);
      }
    }

void BitMap::Get  (Coord2d Pos,VideoMode *v)
    {
    if (Pos<v->Size)
      {
      Coord2d End;
      Pixel *CurAddr;
      if (Pos.y+Size.y > v->Size.y) End.y=v->Size.y;       // The y end is WRT
      else                          End.y=Pos.y+Size.y;    // the VideoMode
      if (End.x+Size.x > v->Size.x) End.x=v->Size.x-Pos.x; // The x end is WRT
      else                          End.x=Size.x;          // this bitmap.

      for (CurAddr=Address;Pos.y<End.y;Pos.y++,CurAddr+=LineSep)
        v->GetCols(Pos,End.x,CurAddr,v);
      }
    }

