/* Borland <conio.h>: console services the port provides itself (see gmcompat.h / dosplat.h). */
#include "gmcompat.h"
#define putch(c) ((void)(c))   /* the bell: no console here */
