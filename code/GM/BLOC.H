/* Header file for any program that uses bit-blk animation */

#ifndef BLEN
  #define BLEN    20   /*  Length and width of each block               */
#endif
#define BACKBL 150   /*  Number of blocks in background block file    */
#define MONBL  100   /*  Number of blocks in a monster file           */
#define CHARBL  50   /*  Number of blocks in the character file       */
#define BACKBLK  0   /*  Background block number                      */

#define SOLTOP 1
#define SOLBOT 4
#define SOLRIG 2
#define SOLLEF 8  


extern int getblktype(void);
extern int loadblks(int btype,char *buffer);
extern int saveblks(int btype,char *buffer);
extern void freadfar(unsigned char far *address,int numbytes,int numtimes,FILE *fp);
extern void fwritefar(unsigned char far *address,int numbytes,int numtimes,FILE *fp);
extern void drawblocpiece(int x,int y,int bx,int by,int len,int wid,int bloc);
extern void scroll_list(int dir,int x,int y,int listlen,int totalen,int *current,int (*dra)(int));
extern void drawlist(int x,int y,int listlen,int totalen,int *current,int (*dra)(int));
extern void vdrawlist(int x,int y,int listlen,int totalen,int *current,int (*dra)(int));
extern void vscroll_list(int dir,int x,int y,int listlen,int totalen,int *current,int (*dra)(int));

extern "C"
  {
  extern int drawblk(int x,int y,unsigned char  *bloc);
//  extern int drawcbloc(int x,int y,unsigned char *bloc);
  extern int getblk(int x,int y,unsigned char   *bloc);
  }

typedef struct
  {
  unsigned char p[BLEN][BLEN]; // Block picture array (Y,X)
  unsigned char Birthmon;   // Monster to appear (2B Imp)
  int Birthtime;            // Time for monster to appear (high bit used as
                            //    flag by playgame).
  unsigned char Xtra[2];    //  X-tras
  unsigned char solid;      /* 1-top 2-right 4-bottom 8-left */
  unsigned char hitpt;      /* Same as above, but can hurt the character */
  int scorepts;             /* If touched accumlate points */
  int chhitpt;              /* Amount to change character points */
  unsigned char nextbl;     /* Change block after time, nextbl=index self */
  int ntime;                /* Change block time value, 0=never change */
  unsigned char touchbl;    /* Change block when touched, touchbl=index self */
  char grav;                /* bit 1 suck up, 2 right, 4 bottom, 8 left */
  unsigned char obj;        /* if character can pick it up */
  unsigned char effect;     /* a # showing what repetition counter affected */
  int eamt;                 /* how much effect the repetition counter has */
  } blkstruct;
