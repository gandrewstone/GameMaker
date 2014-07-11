/*----------------------------------------------------------------------*/
/* Palc.c  - Graphics routines that manipulate the palette              */
/* Created: Feb. 14,1991   Programmer: Andy Stone                       */
/* Last Edited: Mar 3 1991                                              */
/*----------------------------------------------------------------------*/

#include <dos.h>
#include <alloc.h>
#include <stdlib.h>
#include "gen.h"
#include "gmgen.h"
#include "palette.h"
 
#pragma loop_opt(off)

Pixel RGBdata::Match(RGBdata *colors)
  {
  int        min=32000;  // counter to keep track of best fit
  int        sum;        // number to compare to min
  int        r,g,b;      // red green and blue components of the color
  int        newcolor;   // where the number for the closest color is stored.

  for (int k=0;k<256;k++)
    {
    r  =  ((int)(red&63))  - ((int) (colors[k].red&63));
    g  =  ((int)(green&63))- ((int) (colors[k].green&63));
    b  =  ((int)(blue&63)) - ((int) (colors[k].blue&63));
    r *= r;
    g *= g;
    b *= b;
    sum= r+g+b;
    if (sum<min)  { min=sum; newcolor=k; }
    }
  return(newcolor);
  }


unsigned char PalFindCol(unsigned char red, unsigned char green, unsigned char blue,RGBdata *colors)
  {
  int          min=12288;  /* counter to keep track of best fit   */
  int          sum;        /* number to compare to min   */
  int          r,g,b;      /* red green and blue components of the color */
  register int k;          /* looping variable  */
  int          newcolor;   /* where the number for color yellow is stored */

  for (k=0;k<255;k++) 
    {
    r =  ( (red-colors[k].red)*(red-colors[k].red) );
    g =  ( (green-colors[k].green)*(green-colors[k].green) );
    b =  ( (blue-colors[k].blue)*(blue-colors[k].blue) );
    sum = r+g+b;
    if (sum<min)
      {
      min=sum;
      newcolor=k;
      if (sum==0) break;
      }
    }
  return(newcolor);
  }

void FadeTo(RGBdata *dest,int tim)
  {
  register int l,l1,l2;
  struct palcols
    {
    char y[3],dy[3],absdy[3];
    char sdy[3];
    };

  struct palcols *t;
  RGBdata *cur;

  l=TRUE;
  if ( (t=(struct palcols *) farmalloc(sizeof(struct palcols)*256))==NULL) l=FALSE;
  else if ((cur=(RGBdata *)  farmalloc(sizeof(RGBdata)*256))==NULL) { farfree(t); l=FALSE; }

  if (!l) { SetAllPal(dest); return; }

  GetAllPal(cur);
  for (l=0;l<256;l++)
    {
    t[l].dy[0]=dest[l].red-cur[l].red;
    t[l].dy[1]=dest[l].green-cur[l].green;
    t[l].dy[2]=dest[l].blue-cur[l].blue;
    for (l1=0;l1<3;l1++) 
      {
      t[l].sdy[l1]  =Sign(t[l].dy[l1]);
      t[l].absdy[l1]=t[l].sdy[l1]*t[l].dy[l1];
      t[l].y[l1]=0;
      }
    }

  for (l1=0;l1<tim;l1++)
    {
    for (l=0;l<256;l++)
      {
      for (l2=0;l2<3;l2++)
        {
        t[l].y[l2] += t[l].absdy[l2];
        while (t[l].y[l2]>=tim)
          {
          t[l].y[l2]-=tim;
          if (l2==0) cur[l].red += t[l].sdy[l2];
          else if (l2==1) cur[l].green += t[l].sdy[l2];
          else if (l2==2) cur[l].blue  += t[l].sdy[l2];
       	  }
        }
      }

    while ((inport(0x03DA)&8) ==0);  /* bit 0 is horiz. retrace &8 is vert. */
    SetAllPal(cur);
    }
  farfree(t);
  farfree(cur);
  SetAllPal(dest);
  }

void FadeAllTo(RGBdata dest, int tim)
  {
  register int l,l1,l2;
  struct palcols
    {
    char y[3],dy[3],absdy[3],sdy[3];
    };
  struct palcols *t;
  RGBdata *cur;

  l=TRUE;
  if ( (t=(struct palcols *) farmalloc(sizeof(struct palcols)*256))==NULL) l=FALSE;
  else if ((cur=(RGBdata *)  farmalloc(sizeof(RGBdata)*256))==NULL) { farfree(t); l=FALSE; }

  if (!l) { SetAllPalTo(&dest); return; }

  GetAllPal(cur);
  for (l=0;l<256;l++)
    {
    t[l].dy[0]=dest.red-cur[l].red;
    t[l].dy[1]=dest.green-cur[l].green;
    t[l].dy[2]=dest.blue-cur[l].blue;
    for (l1=0;l1<3;l1++) 
      {
      t[l].sdy[l1]  =Sign(t[l].dy[l1]);
      t[l].absdy[l1]=t[l].sdy[l1]*t[l].dy[l1];
      t[l].y[l1]=0;
      }
    }

  for (l1=0;l1<tim;l1++)
    {
    for (l=0;l<256;l++)
      {
      for (l2=0;l2<3;l2++)
        {
        t[l].y[l2] += t[l].absdy[l2];
        while (t[l].y[l2]>=tim)
          {
          t[l].y[l2]-=tim;
          if (l2==0) cur[l].red += t[l].sdy[l2];
          else if (l2==1) cur[l].green += t[l].sdy[l2];
          else if (l2==2) cur[l].blue += t[l].sdy[l2];
       	  }
        }
      }
    while ((inport(0x03DA)&8) ==0);  /* bit 0 is horiz. retrace &8 is vert. */
    SetAllPal(cur); 
    }
  farfree(t);
  farfree(cur);
  SetAllPalTo(&dest);
  }


/*
void Palette(unsigned char col,unsigned char red,unsigned char green,unsigned char blue)
  {
  while ((inport(0x03DA)&8) ==0);
  outportb(0x03C8,col);         // Set the color to change
  outportb(0x03C9,red);         // Set the red value 0-63
  outportb(0x03C9,green);       // Set the Green value 0-63
  outportb(0x03C9,blue);        // Set the Blue value 0-63
  }

void SetAllPal(RGBdata *pal)
  {
  register int l;

  while ((inport(0x03DA)&8) ==0);  // Wait for vertical retrace
                                   // bit 0 is horiz. retrace
  for (l=0;l<256;l++)
    {
    outportb(0x03C8,l);                 // Set the color to change
    outportb(0x03C9,pal[l].red);        // Set the red value 0-63
    outportb(0x03C9,pal[l].green);      // Set the Green value 0-63
    outportb(0x03C9,pal[l].blue);       // Set the Blue value 0-63
    }
  }

void SetAllPalTo(RGBdata *pal)
  {
  register int l;

  while ((inport(0x03DA)&8) ==0);  // Wait for vertical retrace
                                   // bit 0 is horiz. retrace
  for (l=0;l<256;l++)
    {
    outportb(0x03C8,l);                 // Set the color to change
    outportb(0x03C9,pal->red);        // Set the red value 0-63
    outportb(0x03C9,pal->green);      // Set the Green value 0-63
    outportb(0x03C9,pal->blue);       // Set the Blue value 0-63
    }
  }
*/

char FindForCol(RGBdata *colurs, char bkcol,char choice)
 {
  register int i;
  char flag = 63;
  int col = -1;

  while(col==-1) {
    for(i=0;i<256;i++) {
      if ( (colurs[i].red>flag)|(colurs[i].blue>flag)|(colurs[i].green>flag) )
        { if (i==bkcol) { col = -2;  break; }
        else if (!(--choice)) { col = i;  break; }
       }
     }
    flag--;
   }
  flag=0;
  while(col==-2) {
    for(i=0;i<256;i++) {
      if ( (colurs[i].red<flag)&(colurs[i].blue<flag)&(colurs[i].green<flag) )
        if (i!=bkcol) if (!(--choice)) { col = i;  break; }
     }
    flag++;
   }
  flag = col;
  return (flag);
 }

void RainbowCols(RGBdata *colors)
  {
  int l;
  const int addamt = 2;  
  char red=0,green=0,blue=0;
  char rdir=0,bdir=0,gdir=0;
  
  for (l=0;l<256;l++)
    {
    if (l==0) bdir=1;
    if (l==64/addamt) rdir=1;
    if (l==128/addamt) { gdir=1; bdir=1; }
    if (l==192/addamt) bdir=1;
    if (l==226/addamt) {rdir=1; gdir=1; }
    if (l==272/addamt) {rdir=0; bdir=1; }
    if (l==336/addamt) {rdir=1; bdir=0; }
    if (l==400/addamt) {bdir=1; gdir=1; }
    if (rdir==1) red+=addamt;
    if (gdir==1) green+=addamt;
    if (bdir==1) blue+=addamt;
    if (rdir==2) red-=addamt;
    if (gdir==2) green-=addamt;
    if (bdir==2) blue-=addamt;
    if (red>=63)   { rdir=2; red=63;   }
    if (blue>=63)  { bdir=2; blue=63;  }
    if (green>=63) { gdir=2; green=63; }
    if (red<0)   { red=0; rdir=0; }
    if (green<0) { green=0; gdir=0; }
    if (blue<0)  { blue=0; bdir=0; }
    insertcol(l,red,green,blue,colors);
    }
  }

void RandCols(RGBdata *colors)
  {
  int l;
  const int addamt = 2;  
  char red=0,green=0,blue=0;
  char rdir=0,bdir=0,gdir=0;
  
  for (l=0;l<256;l++)
    {
    if (l%(64/addamt)==0)
      {
      do {
        if (bdir==0) bdir=random(2);
        if (gdir==0) gdir=random(2);
        if (rdir==0) rdir=random(2);
        } while ((bdir==0)&&(gdir==0)&&(rdir==0));
      }
    if (rdir==1) red+=addamt;
    if (gdir==1) green+=addamt;
    if (bdir==1) blue+=addamt;
    if (rdir==2) red-=addamt;
    if (gdir==2) green-=addamt;
    if (bdir==2) blue-=addamt;
    if (red>=63)   { rdir=2; red=63;   }
    if (blue>=63)  { bdir=2; blue=63;  }
    if (green>=63) { gdir=2; green=63; }
    if (red<0)   { red=0; rdir=0; }
    if (green<0) { green=0; gdir=0; }
    if (blue<0)  { blue=0; bdir=0; }

    insertcol(l,red,green,blue,colors);
    }
  }

void insertcol(int number,unsigned char red,unsigned char green,unsigned char blue,RGBdata *colors)
  {
  colors[number].red=red;
  colors[number].green=green;
  colors[number].blue=blue;
  }
 
void InitStartCols(RGBdata *colors)
  {
  RainbowCols(colors);
  insertcol(246,COL0,colors);           // light gray (surfaces)
  insertcol(247,COL1,colors);           // medium red (buttons)
  insertcol(248,COL2,colors);           // dark red (button sides)
  insertcol(249,COL3,colors);           // black
  insertcol(250,COL4,colors);           // a shade darker than above
  insertcol(251,COL5,colors);           // a shade darker than above
  insertcol(252,COL6,colors);           // a shade darker than above
  insertcol(253,COL7,colors);           // bright red (pressed buttons)
  insertcol(254,COL8,colors);           // imitation LCD back-lighting
  }
