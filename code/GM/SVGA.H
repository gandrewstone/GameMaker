/*--------------------------------------------------------------------------*/
/* Super VGA header file  ( for svgac.c svgaa.asm)                          */

#ifndef SVGA
#define SVGA

#include "gen.h"

typedef enum 
   { Unknown, ATI, ChipTech, Genoa, Paradise, Trident8800,Trident8900, Tseng3000, Tseng4000, Video7, VESA, ATIWonder, SoftwareSim, NoType} VideoCards;

extern int Identify_VGA(void);
extern int Force_VGA(char *vgaid);
extern int Force_VGA(VideoCards v);
extern int MoveWindow(unsigned char offset64k);
extern int ViewWindow(unsigned long int offset4);

extern int WaitVertRetrace(void);


extern "C"
  {
  extern int MoveViewScreen(unsigned long int offset4,unsigned int LittleShiftx);  // move w/o updating draw varibles

  void BufToScrn(unsigned int len,unsigned int offset);
  void SvgaBufToScrn(unsigned int len,unsigned int offset);
  void SoftwareScroll(void);
  extern void SetPage(unsigned char pgnum); 
  extern void SetTrident(void); 
  extern void SetTridentStart(int highstart);  /* Bit #16 of start address */
//  extern void drawblk(int x,int y,unsigned char *blk);
//  extern void getblk (int x,int y,unsigned char *blk);
  extern void Point(int x,int y,unsigned char col);
  extern uchar GetCol(int x,int y);
  extern void interrupt NewTimerAsm(...);
  extern void far Gwritestr(int x,int y,int col,char *str,int len); /* Faster */
  }
#endif
/* ---------------------------------END-------------------------------------*/
