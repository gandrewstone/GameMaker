/* GameMaker V1.05 monster data file definition                       */
/* Copyright 1991 Oliver Stone                                        */
/* Code released to the Public Domain.                                */


#define LASTMON  64     // Max number of monsters
#define LASTSL   20     // Max number of frames per monster animation
#define MAXMARCH 10
#define NORMALMOVEMENT 0
#define ATTACKMOVEMENT 1


typedef struct          /* Define the coordinates data structure */
  {
  int x;                /* X Block address offset from start block address */
  int y;                /* Y Block address offset from start block address */
  int pic;              /* Speed if a path is defined     */
  int orient;           /* Current orientation of picture */
  } coords;

typedef struct          /* Define the Monster data structure */
  {
  int used;             /* Whether this structure instance defines a monster */
  int fly;              /* Whether or not gravity has an effect on a monster */
  int jump;             /* Not currently used */
  int stick;            /* Not currently used */
  unsigned char power;  /* charact. power 100. Higher power kills lower */
  char thru;            /* Whether monster stops on solid */
  int pic[LASTSL+1][2]; /* Array of Pictures and times intervals, 0=pic,1=ttc)*/
  ulongi lasttime;      /* Keeps track of last time of picture change */
  char newmon;          /* Monster (index) born at death of current monster */
  int upscore;          /* Score bias when monster dies */
  char xtra[21];        /* Filler */
  int activate;         /* Set if special march when near player 1-con. 2-rel */
  int end;              /* -2 never end, -1 end after path, 0 end when killed, other is the time til end */
  int towards;          /* If set, + movement is always towards player */
  int close[2];         /* When player is close enough to take action */
  coords march[2][MAXMARCH];  /* Ten corners of a set march (relative to monster) */
                        /* Ten corners of monster when near player */
  coords cur;           /* Current coordinates */
  } monstruct;
