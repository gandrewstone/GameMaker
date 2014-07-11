#include "gen.h"
#include "FileClss.hpp"
#include "pal.h"


Palette P;

void main(void)
  {
  FileRead f("mars.pal");
  FileWrite fw;
  f.Get((uchar *) &P.Cols,768);
  f.Close();

  for (uint i=0;i<256;i++)
    {
    uchar t=P(i).red;
    P(i).red=P(i).green;
    P(i).green = t;
    }

  fw.Open("green.pal");
  fw.Write((uchar*)&P.Cols,768);
  fw.Close();
  }

