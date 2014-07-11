/*----------------------------------------------------------*/
/* Graphics Screen Display routines - source in facelift.c  */
/* Include:  pal.h and graph.h                              */
/*----------------------------------------------------------*/

#ifndef BLEN
  #define BLEN 20
#endif

#define NONE  0
#define LEFT  1
#define RIGHT 2
#define MAXFACECOL 9
#define GREY1 0
#define RED1  1
#define RED2  2
#define BLACK 3
#define GREY2 4
#define GREY3 5
#define GREY4 6
#define RED3  7
#define GREEN 8
#define BKGCOL FaceCols[BLACK]                //Background color

void drawlistframe(int x,int y,int blnum,unsigned char *Cols);
void drawsurface(int x,int y,int xlen,int ylen,unsigned char *Cols);
void drawborder(int x,int y,int xlen,int ylen,int xthick,int ythick,unsigned char *Cols);
void DrawArrows(int x,int y,int blnum, char selected,unsigned char *Cols);
void BoxWithText(int x,int y,char *str,unsigned char *Cols);
void SetCols(RGBdata *colors,unsigned char *Cols);

extern void helpb(int x,int y, unsigned char col[4]);
extern void menub(int x,int y, unsigned char col[4]);
extern void noneb(int x,int y, unsigned char col[4]);
extern void doneb(int x,int y, unsigned char col[4]);
