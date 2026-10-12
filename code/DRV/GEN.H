#ifndef GENDECLARES
#define GENDECLARES
#ifndef GM_ENUM16
#define GM_ENUM16          // Borland enums are 16 bits; the port shim defines this as ": short"
#endif
//#pragma comment(exestr,"Copyright 1995 G. Andrew Stone")
#include <alloc.h>
enum { FALSE=0,TRUE=1,False=0,True=1 };
typedef int boolean;
typedef unsigned int       uint;
#ifndef GM_PORT
typedef long           gm_long;    // 32 bits here; the port's shim defines these two as int32_t/uint32_t
typedef unsigned long  gm_ulong;   // because "long" is 64 bits on Linux and macOS
#endif
typedef gm_ulong       ulongi;
typedef gm_long        longi;
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