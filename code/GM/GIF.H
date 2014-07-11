/* gifread header file         */
/* created 6/22 by Ollie Stone */
/* last edited 6/25            */

extern void decode(FILE *gif,int skip);      /* main program.  draws gif */
extern int intro  (FILE *gif,RGBdata colors[256] );
extern int drawbkd(char *bkdname,RGBdata *colors); /* draws gifs from bkd files */
extern int DrawGif(char *gif, RGBdata *colors);

extern int xstart,ystart;  /* x,y current and first coords of point being drawn */
extern int imagex,imagey;  /* dimentions of image. codesize-start val. of codestart */
extern int scrx,scrx1,scry,scry1;
extern char col;
