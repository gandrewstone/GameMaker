/* GameMaker v1.05 .map data file/structure decription                   */
/* map.h - header file for any program that uses mapmaker.exe to make    */
/*         arrays of blocks.                                             */
/* Copyright 1991 Gregory Stone                                          */
/*                                                                       */
/*************************************************************************/
#include "Coord2d.hpp"
#define LASTMM  200    /* Number of Monsters that can be placed on a map */
#define MLEN    100    /* Length of Map in Blocks */
#define MWID    100    /* Width of Map in Blocks */

extern int loadmap(void); /* Function to load a Map file from disk */
extern int savemap(void); /* Function to save a Map file to disk */
 
typedef struct          // Pointed to by the map index (Y,X)
  {
  unsigned char blk;    // Initial block in block array
  unsigned char mon;    // Current Index to monster in monmapstruct
  } mapstruct;          // Name the defined structure

typedef struct          // Monster Array, One Entry per Monster
  {
  unsigned char monnum; // Index to Monster in monstruct (0 to (LASTMON-1))
  int status;           // Filler, should never be used
  unsigned char xtra;   // Not Used
  int curx,cury;        // Index (Y,X) to Monster on Map
  int extra;            // Not Used
  } monmapstruct;       // Name the defined structure

typedef struct
  {
  Coord2d Size;
  int     NumLvls;
  } MapHeader;

typedef struct          // Pointed to by the map index (Y,X)
  {
  unsigned char blk[2]; // Initial block in block array
  } MapStruct;          // Name the defined structure

//--------------------------------------END------------------------------
