/* Borland <dir.h>: findfirst/findnext over the platform layer. */
#include "gmcompat.h"
#define FA_RDONLY 0x01
#define FA_HIDDEN 0x02
#define FA_SYSTEM 0x04
#define FA_LABEL  0x08
#define FA_DIREC  0x10
#define FA_ARCH   0x20
#define findfirst(path, blk, attr) gm_findfirst((path), (blk), (int16_t)(attr))
#define findnext(blk) gm_findnext(blk)
