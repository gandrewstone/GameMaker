#ifndef GENDECLARES
#define GENDECLARES
//#pragma comment(exestr,"Copyright 1995 G. Andrew Stone")
#include <alloc.h>
typedef enum { FALSE=0,TRUE=1,False=0,True=1 } boolean;
typedef unsigned int       uint;
typedef unsigned long int  ulongi;
typedef long int           longi;
typedef unsigned char      uchar;
typedef int                Block;

#ifndef Sign(x)
  #define Sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))
#endif

#define InBox(x,y,lx,ly,mx,my) ((x>=lx)&&(y>=ly)&&(x<=mx)&&(y<=my))

#define NAMELEN 81
#define MAXLENGTH 64
#define ALLOC farmalloc
#define FREE  farfree
#endif
