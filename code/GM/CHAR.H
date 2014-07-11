/* character header file */
/* created 7-7-91 by Ollie Stone */
/* Last modified 7-7-91 */

#define CHARPOWER 100
#define MAXPLAY  2       /* Maximum number of players allowed */
#define MAXSEQ   20      /* Maximum number of sequences */
#define MAXFRAME 10      /* Maximum number of frames */
#define IDLESEQ  0       /* Sequence # of idle seq, do not change from 0 */
#define DIESEQ    1
#define HURTSEQ   2
#define PICKUPSEQ 3
#define DROPSEQ   4
#define INF_REPEAT 32767 /* Number to represent a sequence with no limit   */
                         /*              of repetitions.                   */   
#define MAXINV   10

typedef struct           /* Declare a typed data structure */
 {
  int x,y;               /* Pixel offset from previous frame */
  unsigned orient;       /* Orient bits how far to rotate object */
  unsigned pic1;         /* Block number for top block of character */
  unsigned pic2;         /* Block number for lower block of character */
  unsigned pause;        /* Clock ticks to wait prior moving to next frame */
  char extra[5];         /* Filler */
 } frames;               /* Name the data structure */

typedef struct           /* Declare a typed data structure */
  {
  frames fr[MAXFRAME];   /* Reference frame array instantiation */
  int key;               /* Activation key (upper byte=scan,lower=ascii code) */
  char sound,on;         /* Sound routine #, 'enable' flag */
  unsigned char bloc;    /* Block required in order to perform this sequence */
  char ground;           /* Must be on solid block to perform this sequence */
                         // Actually, "ground" is now whether the seq can be interrupted or not.
  char jstk_but;         /* Bytes used for joystick movment */
  char jstk_mov;
  char norepeat;         // 0=keep repeating  1=one seq per key stroke
  char Momentum;         // TRUE = motion affects momentum (gravx,gravy), FALSE = does not
  char xtra[5];          /* Filler */
  } sequence;            /* Name the data structure */

typedef struct           /* Declare a typed data structure */
  {
  sequence  sq[MAXSEQ];  /* Reference sequence array instantiation */
  int x[3],y[3];         /* [0]=cur in map; [1]=1-20 cur block; [3]= cur addon (momentum) */
  char cframe;           /* Current frame number in current sequence */
  char cseq;             /* Current sequence of the character, if any */
  unsigned char inv[MAXINV]; /* Array of inventory items gathered */
  int meter[MAXSEQ+10];  /* Repetition counters */
  } chrstruct;           /* Name the data structure */
