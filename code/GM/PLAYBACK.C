/*---------------------------------------------------------------------*/
/* playgame.c -- plays a game made using gamemaker                     */
/* Created:  JUNE 20,1991    Programmer:   Andy Stone                  */
/*                                                                     */
/*                                                                     */
/*                                                                     */
/* Last Edited: July 8, 1993    Andy Stone     Version: 2.04           */
/* Feb 21, 94 - MicroChannel kbd support added.                        */
/*---------------------------------------------------------------------*/

//#pragma inline
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <conio.h>
#include <alloc.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>

#include "svga.h"
#include "gen.h"
#include "graph.h"
#include "mousefn.h"
#include "jstick.h"
#include "scrnrout.h"
#include "windio.h"
#include "pal.h"
#include "gif.h"
#include "bloc.h"
#include "mon.h"
#include "map.h"
#include "char.h"
#include "grator.h"
#include "sound.h"
#include "dirty.h"
#include "coord2d.hpp"

#ifndef MOUSE
#error MOUSE needs to be defined! (old mouse is used in playgame).
#endif

extern int mouinstall;

// Needed definitions from "time.h"
typedef long time_t;
time_t   _Cdecl time(time_t _FAR *__timer);

#define  TRANSCOL 255     // The value that is transparent in _pic fns
#define  DEAD       2

#define  CHRSEQ chr.sq[chr.cseq]
#define  CHRFRAME CHRSEQ.fr[chr.cframe]

#define MAXKEYS          20      // Every Finger and Toe, hee hee hee...
#define MAXGRAV          11
#define SCRNBLEN         16
#define SCRNBWID         10
#define GRAVSPEED        1
#define MONMOVESPEED     1
#define JSTICKPOLLSPEED  1
#define JSTICKKEY        1       // Since 1 is ESC, which can't be on the stack, we are safe
#define ATTACKX          8       // Boundaries
#define ATTACKY          5
#define RESETTIMER       1
#define OFFSCRN         -100
#define SLOWSCROLLAMT    4
#define SCRSPDX          40
#define SCRSPDY          40
#define SCROLLPERMOVE    6
#define MAX_MONS_ON_SCRN 50

#define CRTCINDEX      0x3D4
#define CRTCADDRESS    0x3D5

//#define SOLDEBUG
//#define GENDEBUG
//#define HITDEBUG
//#define NOMONS
//#define NOCHAR
//#define NOBACKANI
//#define SCROLLDEBUG
//#define WAYSLOW
//#define REALLYFAST
//#define MEMDEBUG

//#define RECORDER
#define DIGITALSND
 
typedef struct
  {
  unsigned char key;
  int seq,frame;
  } onekey;

typedef struct
  {
  int DScroll;
  int Ctr;
  int Iters;
  } BresScroll;

class minfo
  {
  public:
  char fromchar;        /* Birthed monsters can't hurt the character      */
  Coord2d Delta;            /* Bresenham's algorythm - dx = (curpos-dest)     */
  int curx[2],cury[2];  /* [0] block position [1] pos. in block           */
  Coord2d Target;  /* Where the monster is going if in a path.       */
  int offl,offs;        /* Xy ratios in Bresenham's line algorythm        */
  char CurMarch;        /* index to normal=0 or attack=1 pattern          */
  int  CurPic;          /* Index in mon[].pic[Curpic] of monster picture  */
  unsigned char orient; /* How to flip, rotate picture                    */ 
  unsigned char *save;  /* address to saved screen behind monster         */
  char PicDrawn;        /* Whether or not there is a picture to erase     */
  Coord2d ScrPos;        /* Screen coords of monster                       */
  Coord2d OldPos;        /* Where to erase the picture                     */
  unsigned int EraseAddr;
  char dest;            /* Index to Next Position of Monster in monstruct */
  unsigned long int NextAni;    /* Time to animate next                   */
  unsigned long int NextMove;   /* Time to move next                      */
  unsigned long int BirthTime;  /* The time the monster was born          */
  Coord2d Bresenhams(int speed);
  boolean ChkWalls(Coord2d *Move);
  void    DistToPlace(Coord2d Spot);
  void    Draw(int MMnum);
  void    Erase(void);
  void    Reset(void);
  };

static boolean MonEndPath(int MMnum,int MONnum);

typedef struct
  {
  long int score;
  unsigned char  *savetop;     // save space for behind the top of the character
  unsigned char  *savebottom;  // save space for behind the bottom of the char
  char update;
  char topthere;      // TRUE if character has a top block to erase
  char bottomthere;   // TRUE if character has a bottom block to erase
  char curtop;        // The current top picture
  char curbottom;     // The current bottom picture
  int nextx,nexty;    // Where character will be located on the screen.
  int scrx,scry;      // Where character should be drawn.
  int oldx,oldy;      // where character should be erased.
  unsigned int EraseAddr; // address in mem of last place character was.
  char netx,nety;     // Delta of character's last 2 moves
  int gravx,gravy;    // Current velocity due to gravity
  int hitpts;         // Current # of hitpoints the char has
  int lives;          // Current # of lives -1 = game over
  unsigned long int NextAni; // Next time to move the character
  unsigned char inv[MAXINV]; // Array of inventory items gathered
  int meter[MAXSEQ+10];      // Repetition counters
  char seqon[MAXSEQ];
  } cinfo;

typedef struct      //   Structure which contains map coords of blocks the
  {                 // character is touching.
  int blks[6][2];            // contains x,y map coords
  unsigned char blknum[6];   // contains block number
  } touchblk;

typedef struct
  {
  unsigned long int zero;  // Where screen is in memory for playgame only
  int nx,ny;               // 1-20, Where edge of screen is in the edge block
  int Totalx,Totaly;   // How far to move screen in next 1/18 sec, - = mov screen up or left
  int distx,disty;         // distance in blocks to center of screen.
  int cont;
  int MaxX,MaxY,MinX,MinY;
  int LeftStop,RightStop,UpStop,DownStop;
  BresScroll Movx,Movy;
  int movx,movy;
  char LittleShiftx,LittleShifty;
  unsigned char up,down,left,right;
  } ScrollStruct;

typedef struct
  {
  unsigned char blnum;
  unsigned long int nexttime;
  } BlkMonStruct;

static void SimKeyPress(unsigned char key);

#ifdef  RECORDER
#define RECLEN 5000
#define RECORD 1
#define PLAYBACK 2

typedef struct
  {
  unsigned int DeltaTime;
  unsigned char key;
  } RecordStruct;

RecordStruct     *Rec=NULL;
int              RecIndex=0;
char             RecFile[MAXFILENAMELEN];
char             RecFlag=0;
unsigned long int LastKeyPress=0;
#endif RECORDER

blkstruct        *blks[3]={NULL,NULL,NULL};   // addresses - Backgnd[0],mon[1],char[2]
integratorstruct *scns;
monstruct        *mon=NULL;
minfo            *mi=NULL;
ScrollStruct     scroll;
mapstruct far    map[MLEN][MWID];
monmapstruct     mm[LASTMM];
RGBdata          colors[256];
sndstruct        snd[LASTSND];
char             DigiSnd[LASTSND][9];
RGBdata          Black={0,0,0};
cinfo            ci;
chrstruct        chr;

unsigned int     FirstEmptyMM=0;
unsigned char    monsolid=0;     // > 0 means that a monster is limiting movement.
unsigned char    MMsolid=LASTMM; // Use SOLTOP, BOT,LEF,RIG to find which dir.
unsigned char    NoMons=FALSE;

BlkMonStruct      *BlkMon=NULL;   // Next time to birth monster for blocks.
unsigned char     MaxBlkMon=0;    // Number of blocks which birth monsters.
unsigned long int nxtchg[BACKBL]; // Next time array for each block change.
unsigned char     blkmap[BACKBL]; // Current block displayed on map Array.

int                    mx=0,my=0; // Screen upperleft in the map[].
extern char            *FontPtr;
extern volatile unsigned long int clock;
volatile unsigned long int SndClock=1;
char                   soundon=TRUE;
char                   anysound=FALSE;
volatile unsigned char CurSnd=LASTSND;
volatile int           AtNote=0;
extern unsigned int    zeroaddon;
extern unsigned char   zeropage;
extern VideoCards far  Vcard;
volatile onekey        keystack[MAXKEYS];
volatile int           keystkptr=0;
volatile unsigned char KeysPending[50];
volatile int           PendCtr=0;

unsigned char          *MonsterMem=NULL;
unsigned int           MonsOnScrn=0;

extern int             joyinstall;          // Is the Joystick installed?
ConfigStruct           cs;                  // Joystick coordinates array.
int                    jstickon=FALSE,jinstalled=FALSE;

static char            FromGM[6] = {0,0,0,0,0,0};

// These are not needed because I use .asm keyboard routines now.
//volatile unsigned char key=0;       // Used in the keyboard interrupt routine.
//volatile unsigned char oldkey=0;    // In kbd int, stops key repeats.
//volatile unsigned char keyFlag=0;   // Used in the keyboard interrupt routine.
//volatile char          in=FALSE;    // True if in keyboard interrupt routine.

volatile char          keydn[128];  // TRUE if key w/ scan code index is pressed.

volatile unsigned char curkey=0;
int                    oldscene=0;  // Scene 0 is the startup scene
int                    curscene=0;
int                    doscene,prevscene,linkin;
static void interrupt  (*OldKbd) (...);
static void interrupt  (*OldTimer)(...);

static char            gamename[MAXFILENAMELEN];
char                   gamepath[MAXFILENAMELEN];

extern char            serialnum[];
extern char            curfile[31];
extern unsigned        _stklen=10000U;
int                    Instruments;
char                   SongInfo = 0;
int                    InitHorizShift=0;
char                   MicroChnl;

/*--------------------------------------------------------------------------*/
/*                         FUNCTION    PROTOTYPES                           */
/*--------------------------------------------------------------------------*/
inline int Min (int a, int b); // { if (a>b) return b; return a; }
inline int Max (int a, int b); // { if (b>a) return b; return a; }
inline int Abs (int a);        // { if (a<0) return (-a); else return (a); }

extern "C" char MicroChannel(void);

static int  AskConfigData(ConfigStruct *cs);
static void DrawConfigScrn(ConfigStruct *cs,int Vga,char *DrvrName,char *DrvrPath);
static void DrawSurf(int x,int y,int xlen,int ylen);
static void CuteBox(int x,int y,int x1,int y1,char col1,char col2);


static int  BresenhamScroll(BresScroll *b,int speed);

static void DoFunKy(int keynum);
static void Load_Game(void);
static void Save_Game(unsigned long int SndTimeSaved);
static void Display_File(char *filen,unsigned char col);
static void GMabout();
static void HelpFiles();
static void Display_Invent();
static void Joy_Config(ConfigStruct *cs);
static char getjoydat(unsigned int *x, unsigned int *y,unsigned int *butn);
static char DropBloc(void);
static int  PickupBloc(void);
static void checkseq_fortie(unsigned char block,char setting);

static unsigned int chkdowns(touchblk *t);
static unsigned int chkups(touchblk *t);
static unsigned int chkrights(touchblk *t);
static unsigned int chklefts(touchblk *t);

static void StealKbd(void);
extern "C" void interrupt NewMicroKbd(...);
extern "C" void interrupt NewATKbd(...);
static void interrupt NewTimer(...);   // Advances timer every clock tick.
static void PopKey(unsigned char key);
static void PushKey(void);
static void CleanKeyStk(void);
static void cleanup(void);

static int  DoCollision(int mm1,int mm2,int where);
static char PicChkTouch(int x1,int y1,unsigned char *pic1, int x2,int y2,unsigned char *pic2);
static int  ChkMonChr(int MMnum);
static char MonInRange(int MMnum);

static void play(int startlink);
static void drawmap(int x,int y);
static void initalldata(void);
static void changeblks(void);
static void GetTouchbl(touchblk *t);
static void CheckTouch(void);
static char really_checktouch(int x,int y,unsigned char bnum);
static unsigned char DoBlockTouchChar(blkstruct *blk);
static void chkmapbounds(void); /* Check to see is outside of map boundaries */
static void chkscrnbounds(int *tx,int *ty);/* Makes sure char stays within screen bounds */
static int  MoveCharGrav(int fn); /* fn=0 Test for Grav. fn=1 Reset timer */

static void BlocBirthMon(void);
static void CharBirthMon(int x,int y,int mon);
static int  BirthMon(int MMnum,char mon,int x,int y);
static boolean KillMon(int MMnum);
static int  moveallmons(void);
static int  movechars(void);
static void UpdateScreen(int monmoved);
static void erasechars(void);
static void drawchars(void);
static int  MonPicNum(int MMnum); 

static int  chksolids(int xmotion,int ymotion,touchblk *t);
static void OnGround(int SolidInfo);

static int loadscene(int scnum);
static int loadspecblk(char *file,char *ext, char *path,int numblks,blkstruct *addr);
static int loadspecmap(char *file, char *ext, char *path);
static int loadspecmon(char *file,char *ext, char *path,int nummons);
static int loadspecany(char *fname,char *ext,char *path,unsigned int bytes, char *addr);
static int loadspecsnd(char *fname,char *ext,char *path);
static int playscene(int curscene,int oldscene,int oldlink);
static int chklnk(int cursc);
static boolean onscrn(int x,int y,Coord2d *ret);

static void Sdrawspbloc(int x,int y,char dir,blkstruct *blk);

static int  ChangeScroll(void);
static void DrawScroll(int speed);
static void drawhorizside(unsigned int offset,int mx,int my,int nx,int ny);
static void drawvertside(unsigned int offset,int mx,int my,int nx,int ny);

static boolean ShiftEverything(Coord2d Shift);

static void SlowKeys();
static void FastKeys();
static void SlowTimer();
static void FastTimer();

static char HandleJStick(int fn); // 0=handle, RESETTIMER=reset internal clock

extern void checkhi(char *gamename,long int score);

static void StartSceneInits(int startx, int starty);
static void StartGameInits(void);

static void GameMenu(int col);
static void DoGame(void);
static void DrawDefaultScrn(RGBdata *colors);

static unsigned char DoSound(unsigned char sndnum);

static void ShowVidMem(void);
void GetMem(char *s,char *mem);

/*--------------------------------------------------------------------------*/
/*                                 C O D E                                  */
/*--------------------------------------------------------------------------*/

QuitCodes main(int argc,char *argv[])
  {
#ifndef TRANSFER
  int gamein=FALSE;
  int attr=0;
  char w[1000];
  int choice;

  if (argc>2)
    if (strlen(argv[2]) <= 5)
      strcpy(FromGM,argv[2]);
#endif

  randomize();
  TextMode();

        // Show the video Bios area
  if ((argc==2)&&(strcmpi(argv[1],"ShowVideoMemory")==0))
    {
    ShowVidMem();
    TextMode();
    exit(quit);
    }

  //--Get memory for everything
  ci.savetop    = (unsigned char  *) farmalloc(BLEN*BLEN);
  ci.savebottom = (unsigned char  *) farmalloc(BLEN*BLEN);
  #ifdef RECORDER
  Rec    = (RecordStruct *) farmalloc(sizeof(RecordStruct)*RECLEN);
  #endif RECORDER
  MonsterMem=(unsigned char *)    farmalloc(BLEN*BLEN*MAX_MONS_ON_SCRN);
  mi        =(minfo *)            farmalloc(sizeof(minfo)*LASTMM);
  mon       =(monstruct  *)       farmalloc(sizeof(monstruct)*LASTMON);
  scns      =(integratorstruct *) farmalloc(sizeof(integratorstruct)*MAXSCENES);
  blks[2]=(blkstruct  *) farmalloc(sizeof(blkstruct)*(CHARBL+1));
  blks[1]=(blkstruct  *) farmalloc(sizeof(blkstruct)*(MONBL+1));
  blks[0]=(blkstruct  *) farmalloc(sizeof(blkstruct)*(BACKBL+1));
  FontPtr=GetROMFont();
  if ((blks[0]==NULL)||(!InitDirty())) // If any allocs fail the last one must
    {                                  // since its really big.
    errorbox("NOT ENOUGH MEMORY!","  (Q)uit");
    exit(quit);
    }
  #ifdef MEMDEBUG
  printf ("EXTRA MEMORY:%lu\n",farcoreleft());
  PauseTimeKey(2);
  #endif
#ifdef RECORDER
  if (argc>2)
    {
    if (stricmp(argv[2],"Record")==0)   RecFlag=RECORD;
    if (stricmp(argv[2],"PlayBack")==0) RecFlag=PLAYBACK;
    strcpy(RecFile,argv[3]);
    printf("Record/Playback with %s.\n",RecFile);
    }
  if (RecFlag==PLAYBACK)
    {
    FILE *fp;

    if ((fp=fopen(RecFile,"rb"))==NULL) RecFlag=FALSE;
    else
      {
      fread(Rec,sizeof(RecordStruct),RECLEN,fp);
      fclose(fp);
      }
    }
#endif
  jinstalled=InitJStick();
  initmouse();
  if ((!LoadConfigData(&cs))
      ||(strcmpi((const char *) cs.SndDrvr,"NotInit")==0)
      ||((argc>0)&&(strncmpi(argv[1],"config",6)==0)))
    {
    GraphMode();
    if (AskConfigData(&cs)) SaveConfigData(&cs);
    TextMode();
    }

  if (cs.ForceSnd==SndBlaster) SongInfo=SoundCard(&cs);
  else SongInfo=0;
  if ((MicroChnl=MicroChannel())==TRUE) printf("MicroChannel Bus Detected!\n");

  #ifdef DIGITALSND
  if (cs.ForceSnd==SndBlaster) InitSbVocDriver(&cs);
  #endif
  if (cs.ForceVGA==NoType) Identify_VGA();
  else Force_VGA(cs.ForceVGA);

  PauseTimeKey(2);
  MoveWindow(0);
  ViewWindow(0);
  TextMode();
  
  initalldata();
  atexit(cleanup);

  #ifdef TRANSFER
  if (!ParseFileName(argv[0],gamename,gamepath)) return(2);
  if (!loadspecany(gamename,".gam",gamepath,sizeof(integratorstruct)*MAXSCENES,(char*) scns))
    {
    printf("You are missing %s.gam!  I can't do anything without it!",gamename);
    PauseTimeKey(4);
    return(quit);
    }
  DoGame();
  MoveWindow(0);
  ViewWindow(0);
  TextMode();
  return(quit);
  #else
  choice=1;
  while (choice!=3)
    {
    if (choice==1)
      {
      gamein=loadany("Enter game to play: ",".gam","gam\\",sizeof(integratorstruct)*MAXSCENES,(char*) scns,1);
      if (gamein!=TRUE) return(menu);
      if (!ParseFileName(curfile,gamename,gamepath)) return(menu);
      DoGame();
      ViewWindow(0);
      MoveWindow(0);
      TextMode();
      choice++;
      }
    else if (choice==2)
      {
      if (SongInfo) ShutSbVocDriver();
      GraphMode();
      if (AskConfigData(&cs)) SaveConfigData(&cs);
      TextMode();

      if (cs.ForceVGA!=NoType) Force_VGA(cs.ForceVGA);
      else Identify_VGA();
      if (cs.ForceSnd==SndBlaster)
        {
        SongInfo=SoundCard(&cs);
        SongInfo &= ~MUSICOFF;  // Turn Music on.
        }
      else SongInfo=0;
      #ifdef DIGITALSND
      if (cs.ForceSnd==SndBlaster)
        {
        InitSbVocDriver(&cs);
        soundon=TRUE;
        }
      #endif
      }

    if ((choice==3)||(choice==0)) return(menu);
    choice++;
    attr = openmenu(30,5,20,7,w);
    moucur(FALSE);
    writestr(31,5,attr+4,   "PLAYGAME     V"GMVER);
    writestr(31,6,attr+15,  " By Gregory Stone ");
    writestr(31,7,attr+15,  "Copyright(C) 1990");
    writestr(31,9, attr+14, " Play a Game!");
    writestr(31,10,attr+14, " Configure");
    writestr(31,11,attr+14, " Quit");
    choice=vertmenu(30,9,20,3,choice);
    closemenu(30,5,20,7,w);
    }
  return(menu);
  #endif
  } 

static void DoGame(void)
  {
  const int menustart = ((15*8)+1);
  int Red,Blue;
  int choice=1;
  int done=FALSE;
  int inkey=0;
  char far *TheGif=NULL;
  char out[MAXFILENAMELEN];

  GraphMode();
  MoveWindow(0);
  ViewWindow(0);

  //--------------------Get old timer and keyboard interrupts
  OldTimer=getvect(0x8);
  OldKbd=getvect(0x9);

#ifdef TRANSFER
  if (DrawGif("GMtitle.gif",colors))
    {
    FadeTo(colors);
    PauseTimeKey(5);
    }
  else
    {
    TextMode();
    printf("You are missing Game Maker's Title Screen (gmtitle.gif)!\n");
    PauseTimeKey(4);
    return;
    }
#endif
  setvect(0x8,NewTimer);
  FastTimer();
  if (SongInfo&CARDEXIST)                // Board installed
    {
    if (scns[0].fnames[4][0]!=0)         // Start intro music
      {
      MakeFileName(out,"snd\\",scns[0].fnames[4],".cmf");
      if (!InitMusic(&Instruments,out))
        SongInfo |=SONGLOADED;           // Song is valid--start playing it!
      else
        {
        SongInfo &= ~SONGLOADED;         // set bit two to zero
        StopIt();                        // Stop's the music
        }
      }
    }
  do
    {
    if (TheGif==NULL)             // Draw the user's intro screen
      {
      if ((scns[0].fnames[0][0]==0)||(!drawbkd(scns[0].fnames[0],colors)))
        {
        GraphMode();
        DrawDefaultScrn(colors);  // If no intro screen draw the default screen
        }
      else
        {
        FadeTo(colors);
        PauseTimeKey(5);
        }
      TheGif=(char far *) farmalloc(64000);
      if (TheGif!=NULL) GetScrn(TheGif);
      }
    else RestoreScrn(TheGif);
    Blue  = PalFindCol(40,0,40,colors);
    Red   = PalFindCol(63,0,0,colors);
    GameMenu(Blue);
    done=FALSE;
    if (mouinstall)
      {
      mouclearbut();
      setmoupos(100,100);
      }
    while(!done)
      {
      BoxFill(70,menustart-8+(choice*8),75,menustart+4-8+(choice*8),Blue);
      inkey=0;
      while(inkey==0)
        {
        int mx,my,mbuts;
        inkey=bioskey(1);
        if (!inkey)
          {
          if (mouinstall)
            {
            moustats(&mx,&my,&mbuts);
            if (my<90)  inkey=(UPKEY<<8);
            else if (my>110) inkey=(DOWNKEY<<8);
            else if (mbuts>0) inkey=13;
            }
          }
        }
      if (mouinstall) setmoupos(100,100);
      if (bioskey(1)) bioskey(0);
      switch(inkey&255)
        {
        case 0:
          BoxFill(70,menustart-8+(choice*8),75,menustart+4-8+(choice*8),0);
          switch(inkey>>8)
            {
            case 72:
              choice--;
              break;
            case 80:
              choice++;
              break;
            }
          if (choice<1) choice=6;
          if (choice>6) choice=1;
          break;
        case 13:
          done=TRUE;
          break;
        case 27:
          done=TRUE;
          choice=0;
          break;
        }
      }
    switch(choice)
      {
      char s[MAXFILENAMELEN];
      case 1:
        if (TheGif) { farfree(TheGif); TheGif=NULL; }
        if ((SongInfo&0x03)==0x03) // Board installed, music loaded
          {
          StopIt();              // Stop intro music (prepare for game music)
          sbfreemem();           // Free memory
          SongInfo &= ~SONGLOADED;   // Erase bit 2
          }
        SlowKeys();
        play(0);
        GraphMode();
        FastKeys();
        if (SongInfo&CARDEXIST)                      // Board installed
          {
          if (scns[0].fnames[4][0]!=0)               // Start intro music
            {
            MakeFileName(out,"snd\\",scns[0].fnames[4],".cmf");
            if (!InitMusic(&Instruments,out))
              SongInfo|=SONGLOADED;     // Song is valid--start playing it!
            else
              {
              SongInfo &= ~SONGLOADED;  // set bit two to zero
              StopIt();             // turn off music
              }
            }
          }
        break;
      case 2:
        BoxFill(40-(64*4)/10,100-64,280+(64*4)/10,180+(64*2)/10,0);
        if (scns[0].fnames[2][0] == 0) 
          {
          Gwritestr(5,10,Red,"No game-specific instructions ",30);
          do { bioskey(0); } while (bioskey(1));
          }
        else
          {
          MakeFileName(s,"gam\\",scns[0].fnames[2],".txt");
          Display_File(s,Blue);
          }
        break; 
      case 3:
        BoxFill(40-(64*4)/10,100-64,280+(64*4)/10,180+(64*2)/10,0);
        if (scns[0].fnames[1][0] == 0)
          {
          Gwritestr(5,10,Red,"No game-specific storyline.   ",30);
          do { bioskey(0); } while (bioskey(1));
          }
        else
          {
          MakeFileName(s,"gam\\",scns[0].fnames[1],".txt");
          Display_File(s,Blue);
          }
        break; 
      case 4:
        BoxFill(40-(64*4)/10,100-64,280+(64*4)/10,180+(64*2)/10,0);
        if (scns[0].fnames[3][0] == 0)
          { 
          Gwritestr(5,10,Red,"No game-specific credits given",30);
          do { bioskey(0); } while (bioskey(1));
          }
        else
          {
          MakeFileName(s,"gam\\",scns[0].fnames[3],".txt");
          Display_File(s,Blue);
          }
        break; 
      case 5:   // High Score file
        TextMode();
        checkhi(gamename,0);
        GraphMode();
        SetAllPal(colors);
        break; 
      }
    } while ((choice!=6)&&(choice!=0));

  if (TheGif) { farfree(TheGif); TheGif=NULL; }

  if ((SongInfo&0x03) == 0x03)  // Board installed and song is loaded
    {
    StopIt();                   // Stop intro music (prepare for game music)
    sbfreemem();                // Free memory
    SongInfo &= ~SONGLOADED;    // Erase bit 2
    }
  SlowTimer();
  if (SongInfo) ResetFM();                  // Reset the FM chip
  setvect(0x8,OldTimer);
  }

static void DrawDefaultScrn(RGBdata *colors)
  {
  register int l;
  BoxFill(0,0,SIZEX,SIZEY,0);
  RandCols(colors);
  SetAllPal(colors);
  for (l=0;l<=100;l++) Box(40-(l*4)/10,100-l,280+(l*4)/10,180+(l*2)/10,l);
  }

static void GameMenu(int col)
  {
  BoxFill(40,100,280,180,0);
  Box(40,100,280,180,col);
  GWrite(12*8,13*8,col,scns[0].desc);
  GWrite(80,15*8,col,"Play");
  GWrite(80,16*8,col,"Read Instructions");
  GWrite(80,17*8,col,"Read Storyline");
  GWrite(80,18*8,col,"See Credits");
  GWrite(80,19*8,col,"See Highest Scores");
  GWrite(80,20*8,col,"Quit");
  }

static void readyVgaRegs(void)
  {
  int v;
  outportb(0x3d4,0x11);
  v = inportb(0x3d5) & 0x7f;
  outportb(0x3d4,0x11);
  outportb(0x3d5,v);
  }



static void play(int curlink) // Link # off of scene 0 to start in 
  {
  int oldlink=0;
  char out[20];
 
  oldscene=0;
  StartGameInits();

  curscene = scns[0].links[curlink].tnum;
#ifndef TRANSFER                // Make sure the start pos != the end pos
  int l;
  for (l=0;l<MAXLINK;l++)
    if ((scns[curscene].links[l].tnum==1)&&
       (scns[curscene].links[l].fx==scns[0].links[curlink].fx)&&
       (scns[curscene].links[l].fy==scns[0].links[curlink].fy))
      {
      int attr=63;
      TextMode();
      drawbox(8,4,70,15,attr,1,8);
      clrbox (9,5,69,14,attr);
      writestr(10,5,attr,"   Although you correctly drew links between the various");
      writestr(10,6,attr,"scenes, you forgot to specify EXACTLY where in a scene");
      writestr(10,7,attr,"you want the character to start and end (ie.win).  As a");
      writestr(10,8,attr,"result, the starting place and the game won place are in");
      writestr(10,9,attr,"the same position!  This means that player will win the");
      writestr(10,10,attr,"game before he even starts playing.  I'm going to change");
      writestr(10,11,attr,"the game won link temporarily, but you should go to the ");
      writestr(10,12,attr,"integrator and fix the problem.");
      writestr(10,13,attr,"Still Confused?  Read the Owner's Manual Section 4.8.2.");
      writestr(10,14,attr,"              (fourth from the last paragraph)         ");
      bioskey(0);
      scns[curscene].links[l].fx +=  10;
      scns[curscene].links[l].fy +=  10;
      scns[curscene].links[l].fx %=MLEN;
      scns[curscene].links[l].fy %=MWID;
      GraphMode();
      }
#endif   
  BoxFill(0,0,SIZEX,SIZEY,0);
  int temp;               // Shorten the screen so wrap around painting invisible
  readyVgaRegs();

  outportb(CRTCINDEX,0x1);            // cut off horizontally
//  temp=inportb(CRTCADDRESS);
  outportb(CRTCADDRESS,0x4D);

  outportb(CRTCINDEX,0x11);           // lock registers 0-7
  outportb(CRTCADDRESS,(inportb(CRTCADDRESS)|128));

  outportb(CRTCINDEX,0x12);           // Cut off vertically
  temp=inportb(CRTCADDRESS);
  outportb(CRTCADDRESS,temp-8);

#ifdef RECORDER
  LastKeyPress=SndClock;
  RecIndex=0;
#endif
  while ((curscene<MAXSCENES)&&(curscene!=1))
    {
    if (!loadscene(curscene)) break;            // Error Loading scene
    oldlink=curlink;
    curlink=playscene(curscene,oldscene,curlink);
    while(bioskey(1)) bioskey(0);               // Clear input devices 
    if (curlink==MAXLINK) break;                // ESC key hit
    if (curlink!=MAXLINK+1)           // Equals means dead - so do scene over
      {
      oldscene=curscene;           // Save prev scene so link can be accessed
      curscene=scns[oldscene].links[curlink].tnum; //Set next scene to correct # 
      }
    else curlink=oldlink;
    }
#ifdef RECORDER
  if (RecFlag==PLAYBACK) { RecIndex=0; LastKeyPress=0; }
  if (RecFlag==RECORD)
    {
    FILE *fp;

    if ((fp=fopen(RecFile,"wb"))==NULL) RecFlag=FALSE;
    else
      {
      fwrite(Rec,sizeof(RecordStruct),RECLEN,fp);
      fclose(fp);
      }
    }
#endif

  MoveWindow(0);
  ViewWindow(0);
  MoveViewScreen(0,0);
  if (curscene==1)  // Game Won!
    {
    char s[MAXFILENAMELEN];
    unsigned char Blue=0;

    BoxFill(0,0,SIZEX,SIZEY,0);    // Clear the screen
    if (SongInfo&CARDEXIST)        // Board installed
      {
      if (scns[1].fnames[3][0]!=0) // play ending music
        {
        MakeFileName(out,"snd\\",scns[1].fnames[3],".cmf");
        if (!InitMusic(&Instruments,out))
          SongInfo|=SONGLOADED;       //Song is valid
        else
          {
          SongInfo &= ~SONGLOADED;   // set bit two to zero
          StopIt();                  // stop music
          }
        }
      }
    if ((scns[1].fnames[0][0]!=0)&&(drawbkd(scns[1].fnames[0],colors)))
      {                 // Draw the end picture
      FadeTo(colors);
      PauseTimeKey(7);
      }
    else { RandCols(colors); SetAllPal(colors); }
    
    Blue = PalFindCol(40,40,40,colors);
    if (scns[1].fnames[1][0]!=0) // Draw the epilogue
      {
      BoxFill(40-(64*4)/10,100-64,280+(64*4)/10,180+(64*2)/10,0);
      MakeFileName(s,"gam\\",scns[1].fnames[1],".txt");
      Display_File(s,Blue);
      }
    if (scns[1].fnames[2][0]!=0) // Draw the credits 
      {
      BoxFill(40-(64*4)/10,100-64,280+(64*4)/10,180+(64*2)/10,0);
      MakeFileName(s,"gam\\",scns[1].fnames[2],".txt");
      Display_File(s,Blue);
      }
    }
  TextMode();
  checkhi(gamename,ci.score);
  if ((SongInfo&0x03) == 0x03)  // Board installed and song loaded
    {
    StopIt();                   // Stop ending music
    sbfreemem();                // Free memory
    SongInfo &= ~SONGLOADED;    // Erase bit 2
    }
  }

static int playscene(int dosc,int prev,int link)
  {
  #define ADJBLOC(a,b) blks[0][blkmap[map[(chr.y[0]+b)%MWID][(chr.x[0]+a)%MLEN].blk]]
  int done=FALSE;
  int loop;
  int tx,ty;
  int doscroll=FALSE;
  int newlink=MAXLINK;  
  unsigned long int OldClock=0;
  unsigned long int oldclk=0;

  doscene=dosc;
  prevscene=prev;
  linkin=link;

  StartSceneInits(scns[prevscene].links[linkin].tx,scns[prevscene].links[linkin].ty);

   /* ss is the dir that the MAP scrolls, scroll.__ limits the way Character can go */
  scroll.up=scns[doscene].ssdown;
  scroll.down=scns[doscene].ssup;
  scroll.left=scns[doscene].ssright;
  scroll.right=scns[doscene].ssleft;
  scroll.zero=0;            // Set Page scrolling variables to initial states
  scroll.MaxX=SIZEX/3;
  scroll.MinX=-SIZEX/3;
  scroll.MaxY=60;
  scroll.MinY=-60;
  scroll.UpStop=-SIZEY/6;
  scroll.DownStop=SIZEY/6;
  scroll.LeftStop=-SIZEX/4;
  scroll.RightStop=SIZEX/4;
  ViewWindow(0);            // Set everything in VGA to the initial page
  MoveWindow(0);
  MoveViewScreen(0,0);
/***********************************************/
/*    Set up and display the startup screen    */
/***********************************************/
  SetAllPalTo(&Black);
  drawmap(mx,my);                      /* Draw the display map    */
  FadeTo(colors);                      /* Fade the map in         */

  StealKbd();

  oldclk=SndClock+1;
/***********************************************/
/*                                             */
/*    The main processing loop starts here     */
/*                                             */
/***********************************************/
  while((done==FALSE)&&(newlink==MAXLINK))    // If <ESC> or end scene, exit
    {                                         // Start of "while" logic
    #ifdef KEYDEBUG
    char numb[61];
    sprintf(numb,"Keystk:%4d",keystkptr);
    Gwritestr(20,5,27,numb,strlen(numb));
    #endif

    if (oldclk<=SndClock)                     // Scroll map
      {
      if (doscroll) DrawScroll(SndClock-oldclk+1);
      oldclk=SndClock+1;
      }

    if (keydn[1]) done=TRUE;
 
    for (loop=1;loop<11;loop++)   // for function keys 1 to 10
      {
      if (keydn[58+loop])
        switch (loop)
          {
          case 3: if ((SongInfo&(CARDEXIST|SONGLOADED)) == (CARDEXIST|SONGLOADED))
                     {
                     if ((SongInfo&MUSICOFF)==0)  // if Music was on-- Turn off
                       StopIt();
                     SongInfo^=MUSICOFF;          // Toggle music on/off setting
                     }
                   break;
          case 4: soundon^=TRUE; break;           // Toggles sound on/off setting
          case 7: break;                          // reserved for future use
          case 8: if (jinstalled) jstickon ^=TRUE; break;   // Toggle Joystick
          default:
            DoFunKy(loop);
            OldClock=clock;
            oldclk=SndClock;
            break;   // functions which take over screen
          }
      keydn[58+loop]=0;
      }

    if (OldClock<clock)
      {
      OldClock=clock;
      erasechars();
      #ifndef NOMONS
      if (!NoMons)
        {
        for (loop=LASTMM-1;loop>=0;loop--) mi[loop].Erase();
        BlocBirthMon();
        moveallmons();
        }
      #endif
      #ifdef MEMDEBUG
      char temp[81];
      sprintf (temp,"MEM:%10lu",farcoreleft());
      //GWrite(160-(zeroaddon%320),100-(zeroaddon/320),39,temp);
      Gwritestr(16,10,39,temp,14);
      #endif

      if ((PendCtr)&&
          ((!chr.sq[chr.cseq].ground)||    // interruptable,
           (chr.cframe==0))                // or beginning of seq...
         )
        { // If key pending and current sequence interruptible
        int tx=0;
        for (tx=0;tx<PendCtr;tx++) SimKeyPress(KeysPending[tx]);
        PendCtr=0;
        }

      if (movechars()) done=DEAD;        // Move char on the map

      #ifndef NOBACKANI
      changeblks();
      #endif
      #ifndef NOMONS
      if (!NoMons)
        {
        MonsOnScrn=0;
        for (loop=0;((loop<LASTMM)&&(MonsOnScrn<MAX_MONS_ON_SCRN));loop++)
          {
          if ((mm[loop].monnum<LASTMON)&&(mi[loop].CurPic>=0)&&(mi[loop].CurPic<LASTSL))
            mi[loop].Draw(loop);
          }
        }
      #endif
      drawchars();
//      SvgaBufToScrn(65535,zeroaddon);    // For debugging only,
//      ClearDirtyRects();                 //  this will put EVERYTHING in Virtual buffer on the screen.
      DrawDirtyRects();

      doscroll=ChangeScroll();    // Recalculate whether we should be scrolling
                                  // or not and the Bresenham's algorithm stuff it we should.

      monsolid=0; MMsolid=LASTMM;
      newlink=chklnk(doscene);    // See if time to change scene
      }
    }

/*************************************************************/
/*      Exit main "playscene" loop here, then clean up       */
/*************************************************************/
  if ((SongInfo&0x03) == 0x03)  // turn off music and free memory
    {
    StopIt();
    sbfreemem();
    SongInfo &= ~SONGLOADED;      // Erase bit 2 (no song loaded)
    }
  FadeAllTo(Black);
  setvect(0x9,OldKbd);                 /* Reset old keyboard handler address */
  nosound();

  if (keydn[1])                        // Was the Esc key hit?
    newlink=MAXLINK;                   // Signal to quit game.

  #ifdef GENDEBUG
  MoveWindow(0);
  ViewWindow(0);
  MoveViewScreen(0,0);
  TextMode();
  printf("Character Info\n");
  printf("Hitpoints: %d\n",ci.hitpts);
  printf("Lives: %d\n",ci.lives);
  printf("Current Loc: (%d,%d)\n",chr.x[0],chr.y[0]);
  printf("Score:%lu\n",ci.score);
  printf("Current Sequence: %d, Frame: %d\n",chr.cseq,chr.cframe);
  printf("KeyStack:%d (Must be 0 or >)!\n",keystkptr);
  bioskey(0);
  GraphMode();
  #endif

  if (done==DEAD)
    {
    if (ci.lives>0)
      {
      ci.lives--;                    /* Subtract a life */
      ci.hitpts=-1;
      newlink=MAXLINK+1;                /* Start this scene over */
      } else newlink=MAXLINK;
    } 
  return(newlink);                     /* Return with the scene number */
  }                                    /* End of function "playscene" */

static void DoFunKy(int keynum)
  {
  int bluecol;    // blue border color
  int l=0;
  int tx=0,ty=0;
  unsigned long int ClockPause;

  ClockPause=SndClock;                // Save so no time passes when fn key hit.
  setvect(0x9,OldKbd);                // Reset old keyboard handler address
  nosound();
  Screen(0);                          // Turn off screen
  MoveWindow(0);                      // just clear it all and use the whole
  BoxFill(0,0,SIZEX-1,SIZEY-1,0);     // screen. No time for fancy
  ViewWindow(0);
  MoveViewScreen(0,0);
  Screen(1);
  for(l=0;l<128;l++)
    keydn[l]=0;
  PendCtr=0;
  for(l=0;l<LASTMM;l++)
    {
    mi[l].EraseAddr=0;
    mi[l].OldPos.Set(OFFSCRN,OFFSCRN);
    mi[l].PicDrawn=FALSE;
    }
  keystkptr=0;                        // Reset key/sequence stack.
  curkey=0;

  bluecol=PalFindCol(0,0,60,colors);  // Get the closest blue color.
  Box(10,10,310,190,bluecol);
  Box(15,15,305,185,bluecol); 
  while(bioskey(1)) bioskey(0);       // Clear the keyboard buffer
  switch (keynum)
    {
    case 1: HelpFiles(); break;       // Display Help.
    case 2: Display_Invent(); break;  // Status Display.
    case 5: Save_Game(SndClock); break;  // Saves game.
    case 6: Load_Game();                 // Restore game.
            ClockPause=SndClock; break;  // Set clock to restored value.
    case 7: break;                       // reserved for future use.
    case 9: Joy_Config(&cs); break;      // Configure Joystick.
    case 10: GMabout(); break;        // about Game-maker.
    }

  // End Stuff - re-setup screen, variables for game play.
  scroll.zero=0;    // Set Page scrolling variable to initial states
  scroll.nx=scroll.ny=0;
  scroll.movx=scroll.movy=0;

  drawmap(mx,my);                      // Draw the display map
                 // Calculate new character coordinates since after
                 // hitting a func key, the computer centers the screen
  if (mx<chr.x[0]) tx=(chr.x[0]-mx)*BLEN;
  else tx=(MLEN+(chr.x[0]-mx))*BLEN;
  tx+=chr.x[1];
  ci.nextx=ci.scrx = tx;         // Save the new char x coord
 
  if (my<chr.y[0]) ty=(chr.y[0]-my)*BLEN;
  else ty=(MWID+(chr.y[0]-my))*BLEN;
  ty+=chr.y[1];
  ci.nexty=ci.scry = ty;         // Save the new char y coord

  chr.cseq=IDLESEQ;
  chr.cframe=0;
  ci.topthere=FALSE;
  ci.bottomthere=FALSE;
  drawchars();
  StealKbd();
  SndClock=ClockPause;     // Set clock to value before the user hit Fn key.
  clock=ClockPause>>3;
  }

static void Load_Game(void)
  {
  unsigned char Yellow;
  FILE *fp;
  char loadname[MAXFILENAMELEN];
  register int i;
  unsigned char *temp,*temp2;
  unsigned long int SavedTime;

  Yellow=PalFindCol(60,60,0,colors);  // Get the closest color to bright yellow.
  MakeFileName(loadname,gamepath,gamename,".sav");
  if ( (fp = fopen(loadname,"rb")) == NULL )
    {
    Gwritestr(5,10,Yellow,"You have not saved a game!",26);
    PauseTimeKey(2);
    return;
    }

  Gwritestr(11,10,Yellow,"Old Game Loaded!",16);

  temp =ci.savetop;
  temp2=ci.savebottom;

  if (BlkMon!=NULL)
    {
    farfree(BlkMon); // Ollie-- free memory used in struct
    BlkMon=NULL;
    }
        // Load stuff specific to this game
  fread(&prevscene,sizeof(int),1,fp);
  oldscene = prevscene;
  fread(&doscene,sizeof(int),1,fp);
  curscene = doscene;
  fread(&linkin,sizeof(int),1,fp);
  fread((char *)&SavedTime,sizeof(unsigned long int),1,fp);
  fread((char *)&nxtchg[0],sizeof(unsigned long int),BACKBL,fp);
  fread((char *)&blkmap[0],sizeof(unsigned char),BACKBL,fp);
  fread((char *)&ci,sizeof(cinfo),1,fp);
  fread((char *)&chr,sizeof(chrstruct),1,fp);
  fread((char *)&mx, sizeof(int), 1, fp);
  fread((char *)&my, sizeof(int), 1, fp);
  fread((char *)&mi[0],sizeof(minfo),LASTMM,fp);
  fread((char *)&mm[0],sizeof(monmapstruct),LASTMM,fp);
  fread((char *)&map[0][0],sizeof(mapstruct),MLEN*MWID,fp);
  fread((char *)&NoMons,sizeof(unsigned char),1,fp);
  fread((char *)&MaxBlkMon,sizeof(unsigned char),1,fp);
  if (MaxBlkMon!=0)                     //Ollie--reallocate memory for struct
    {
    if ((BlkMon=(BlkMonStruct *) farmalloc(sizeof(BlkMonStruct)*MaxBlkMon))==NULL)
      {
      TextMode();
      errorbox("NOT ENOUGH MEMORY!","  (Q)uit");
      exit(0);
      }
    fread((char *)&BlkMon[0],sizeof(BlkMonStruct),MaxBlkMon,fp);
    }
  fclose(fp);
  
      // Load generic data 
  if (scns[doscene].fnames[0][0]!=0) loadspecany(scns[doscene].fnames[0],".pal","pal\\",sizeof(RGBdata)*256,(char *) &colors);
  if (scns[doscene].fnames[2][0]!=0) loadspecblk(scns[doscene].fnames[2],".bbl","blk\\",BACKBL,blks[0]);
  if (scns[doscene].fnames[3][0]!=0) loadspecmon(scns[doscene].fnames[3],".mon","mon\\",LASTMON);
  if (scns[doscene].fnames[4][0]!=0) loadspecblk(scns[doscene].fnames[4],".mbl","blk\\",MONBL,blks[1]);
  if (scns[doscene].fnames[6][0]!=0) loadspecblk(scns[doscene].fnames[6],".cbl","blk\\",CHARBL,blks[2]);
  if ((scns[doscene].fnames[7][0]!=0)&&(loadspecsnd(scns[doscene].fnames[7],".snd","snd\\")))
    anysound=TRUE;
  else anysound=FALSE;
  if ((SongInfo&(CARDEXIST|SONGLOADED)) == (CARDEXIST|SONGLOADED))
    {                     // Turn off music currently being played in order to
    StopIt();             // replace it with the music for the loaded scene.
    sbfreemem();          // Free memory
    SongInfo &= ~SONGLOADED;  // Erase bit 2 (no song)
    }
  if (SongInfo&CARDEXIST)                   // Board installed
    {                                       //
    if (scns[doscene].fnames[8][0]!=0)      // Music file exists
      {                                     // Start music for restored game
      MakeFileName(loadname,"snd\\",scns[doscene].fnames[8],".cmf");
      if (!InitMusic(&Instruments,loadname))
        SongInfo|=SONGLOADED; // Song is valid
      else
        {
        SongInfo &= ~SONGLOADED;  // set bit two to zero
        StopIt();             // stop music
        }
      }
    }

  SetAllPal(colors);

  scroll.up=scns[doscene].ssdown;
  scroll.down=scns[doscene].ssup;
  scroll.left=scns[doscene].ssright;
  scroll.right=scns[doscene].ssleft;

  for (i=0;i<LASTMM; i++)
    {
    mi[i].save      =NULL;    // Zero the Memory Pointer
    mi[i].PicDrawn  =FALSE;
    }

  ci.topthere     =FALSE;
  ci.bottomthere  =FALSE;
  ci.scrx=ci.nextx=OFFSCRN;
  ci.scry=ci.nexty=OFFSCRN;
  ci.savetop      =temp;
  ci.savebottom   =temp2;

/*  mx=chr.x[0]-(SCRNBLEN/2);  // Set up screen variables around character.
  my=chr.y[0]-(SCRNBWID/2);
  if (mx<0) mx+=MLEN;        // Contend for the map wrap around.
  if (my<0) my+=MWID;
*/ // Commented out because mx,my are now saved and restored.

  scroll.nx=scroll.ny=0;
  scroll.zero=0;        // Set Page scrolling variables to initial states.
  scroll.Totalx=scroll.Totaly=0;
  clock=SavedTime>>3;
  HandleJStick(1); // Set static variables to the new clock value
  MoveCharGrav(1);
  PauseTimeKey(1);
  SndClock=SavedTime;
  }

static void Save_Game(unsigned long int ClockAt)
  {
  FILE *fp;
  char savename[MAXFILENAMELEN];
  unsigned char Yellow;
  register int i;


  Yellow=PalFindCol(60,60,0,colors);  // Get the closest Yellow color.
  MakeFileName(savename,gamepath,gamename,".sav");

  if ( (fp = fopen(savename,"wb")) == NULL )
    Gwritestr(5,10,Yellow,"I cannot save your game!",24);
  else
    {
    Gwritestr(14,10,Yellow,"Game Saved!",11);
    fwrite(&prevscene,sizeof(int),1,fp);
    fwrite(&doscene,sizeof(int),1,fp);
    fwrite(&linkin,sizeof(int),1,fp);
    fwrite((char *)&ClockAt,sizeof(unsigned long int),1,fp);
    fwrite((char *)&nxtchg[0],sizeof(unsigned long int),BACKBL,fp);
    fwrite((char *)&blkmap[0],sizeof(unsigned char),BACKBL,fp);
    fwrite((char *)&ci,sizeof(cinfo),1,fp);
    fwrite((char *)&chr,sizeof(chrstruct),1,fp);
    fwrite((char *)&mx, sizeof(int), 1, fp);
    fwrite((char *)&my, sizeof(int), 1, fp);
    fwrite((char *)&mi[0],sizeof(minfo),LASTMM,fp);
    fwrite((char *)&mm[0],sizeof(monmapstruct),LASTMM,fp);
    fwrite((char *)&map[0][0],sizeof(mapstruct),MLEN*MWID,fp);
    fwrite((char *)&NoMons,sizeof(unsigned char),1,fp);
    fwrite((char *)&MaxBlkMon,sizeof(unsigned char),1,fp);
    fwrite((char *)&BlkMon[0],sizeof(BlkMonStruct),MaxBlkMon,fp);
    fclose(fp);  
    }
  PauseTimeKey(2);
  }

static void GMabout()
  {
  int yellowcol;

  yellowcol=PalFindCol(60,50,0,colors);
  while (bioskey(1)) bioskey(0);
  Gwritestr(5,5,yellowcol,"      G A M E - M A K E R",25);
  Gwritestr(5,6,yellowcol,"          Version "GMVER"   ",25);
  Gwritestr(5,8,yellowcol,"Recreational Software Designs",29);
  Gwritestr(5,9,yellowcol,"Box 1163,  Amherst, NH, 03031",29);
  Gwritestr(5,11,yellowcol,"This game was made with GAME-",29);
  Gwritestr(5,12,yellowcol,"MAKER.  No programming needed!",30);
  Gwritestr(5,13,yellowcol,"Design your own worlds, mon-  ",30);
  Gwritestr(5,14,yellowcol,"sters, characters, sounds...  ",30);
  Gwritestr(5,16,yellowcol,"Game-Maker software, documen- ",30);
  Gwritestr(5,17,yellowcol,"tation, and data files are    ",30);
  Gwritestr(5,18,yellowcol,"COPYRIGHTED and may be copied ",30);
  Gwritestr(5,19,yellowcol,"only as allowed by the license",30);
  Gwritestr(5,20,yellowcol,"agreement.",10);
  Gwritestr(5,21,yellowcol,"                      MORE... ",30);
  do { bioskey(0); } while (bioskey(1));
  BoxFill(16,16,304,184,0);
  Gwritestr(5,5,yellowcol, "Order from your local         ",30);
  Gwritestr(5,6,yellowcol, "computer store or:            ",30);
  Gwritestr(5,8,yellowcol, " KD Software                  ",30);
  Gwritestr(5,9,yellowcol, " Rochester, NH, 03867         ",30);
  Gwritestr(5,10,yellowcol," Info:      1-603-332-8164    ",30);
  Gwritestr(5,11,yellowcol," Orders:    1-800-533-6772    ",30);
  Gwritestr(5,12,yellowcol,"            Visa/MC           ",30);
  Gwritestr(5,13,yellowcol," Call for the latest price!   ",30);
  Gwritestr(5,14,yellowcol,"                              ",30);

  do { bioskey(0); } while (bioskey(1));
  }

static void HelpFiles()
  {
  char file[61];
  unsigned char redcol,yellowcol;
  int l;

  redcol=PalFindCol(50,0,5,colors);
  yellowcol=PalFindCol(60,50,0,colors);

  MakeFileName(file,"gam\\","gmhelp",".txt");
  Display_File(file,yellowcol);
  BoxFill(16,16,304,184,0);
  if (scns[0].fnames[2][0]!=0)
    {
    MakeFileName(file,"gam\\",scns[0].fnames[2],".txt");
    Display_File(file,yellowcol);
    }
  else
    {
    Gwritestr(5,10,redcol,"No game-specific help included",30);
    PauseTimeKey(5);
    }
  }

static void Display_Invent()
  {
  int bluecol;
  register int j,k;
  char lineofinfo[41];
  long time;
  
  bluecol = PalFindCol(0,0,60,colors);
  Box(30,30,35+(5*(BLEN+5)),35+(2*(BLEN+5)),bluecol);
  for (k=0;k<2;k++)
    for (j=0;j<5;j++)
      {
      Box(34+j*(BLEN+5),34+k*(BLEN+5),35+BLEN+j*(BLEN+5),35+BLEN+k*(BLEN+5),bluecol);
      if ( ci.inv[(5*k)+j] != BACKBL )
        drawblk(35+j*(BLEN+5),35+k*(BLEN+5),blks[0][ci.inv[(5*k)+j]].p[0]);
      }
  sprintf(lineofinfo,"Score:     %ld",ci.score);
  Gwritestr(5,11,bluecol,lineofinfo,strlen(lineofinfo));
  if (ci.meter[3] != INF_REPEAT)
    {
    sprintf(lineofinfo,"Money:     %d",ci.meter[3]-1);
    Gwritestr(5,12,bluecol,lineofinfo,strlen(lineofinfo));
    }
  sprintf(lineofinfo,"Hitpoints: %d",ci.hitpts);
  Gwritestr(5,14,bluecol,lineofinfo,strlen(lineofinfo));
  sprintf(lineofinfo,"Lives:     %d",ci.lives);
  Gwritestr(5,15,bluecol,lineofinfo,strlen(lineofinfo));
  time= (clock/18) - (clock/1818);
  sprintf(lineofinfo,"Elapsed Time: %02ld:%02ld:%02ld.%02ld",time/3600,(time %3600)/60,(time%60),(clock*100/18-clock*100/1818)%100);
  Gwritestr(5,19,bluecol,lineofinfo,strlen(lineofinfo));
  do { bioskey(0); } while (bioskey(1));
  }

static void Display_File(char *filen,unsigned char col)
  {
  FILE *fp;
  char lineoftext[41];
  char vertpos=5;
  int  redcol;
  int mx,my,mbuts=0;


  redcol=PalFindCol(50,0,0,colors);
  while (bioskey(1)) bioskey(0);
  mouclearbut();
  if ((fp=fopen(filen,"rt")) == NULL) 
    {
    Gwritestr(5,10,redcol,"Missing File:",13);
    sprintf(lineoftext,"The file %s is",filen);
    Gwritestr(5,12,redcol,lineoftext,strlen(lineoftext));
    Gwritestr(5,13,redcol,"missing or cannot be opened.",28);
    }
  else
    {
    int done=FALSE;
    while ((fgets(lineoftext,32,fp) != NULL)&&(done!=3))
      {
      done=FALSE;
      if (vertpos > 20)
        {
        Gwritestr(28,21,redcol,"MORE...",7);
        do
          {
          if (mouinstall)
            {
            moustats(&mx,&my,&mbuts);
            if (mbuts) done=TRUE;
            }
          if ((bioskey(1)&0x00FF) == 27) done = 3;
          if (bioskey(1)) done=2;
          } while (!done);
        if (done>TRUE) while (bioskey(1)) bioskey(0);
        else mouclearbut();
        vertpos=5;
        BoxFill(40,40,280,176,0);  // Clear the text
        }
      mbuts = strlen(lineoftext);
      if (lineoftext[mbuts]=='\n') lineoftext[mbuts-1]=0;
      GWrite(5*8,vertpos*8,col,lineoftext);
      vertpos+=1;
      }
    fclose(fp);
    }

  mbuts=0;
  do
    {
    if (mouinstall) moustats(&mx,&my,&mbuts);
    } while ((!bioskey(1))&&(mbuts==0));
  while (bioskey(1)) bioskey(0);
  mouclearbut();
  }

static void Joy_Config(ConfigStruct *cs)
  {
  char tempstr[41];
  int  mx,my;
  unsigned int  x=0,y=0,butn=0;
  unsigned int filestatus;
  int redcol,bluecol;

  redcol=PalFindCol(50,0,0,colors);
  bluecol=PalFindCol(0,0,50,colors);
  ReadJoyStick(&x,&y,&butn);
  if ((x==0xFFFF)&&(y==0xFFFF))
    {
    Gwritestr(5,10,redcol,"This machine does not have a  ",30);
    Gwritestr(5,11,redcol,"game port installed!     You  ",30);
    Gwritestr(5,12,redcol,"cannot configure a joystick   ",30);
    Gwritestr(5,13,redcol,"until you have installed a    ",30);
    Gwritestr(5,14,redcol,"game port and joystick.       ",30);
    PauseTimeKey(10);
    return;
    }
  jinstalled=TRUE;
  Gwritestr(5,5,bluecol,"Move joystick all the way Left",30);
  Gwritestr(5,6,bluecol,"and Forward.  Press either    ",30);
  Gwritestr(5,7,bluecol,"joystick button when there.   ",30);
  if (!getjoydat(&x,&y,&butn)) return;

  cs->joyy[0]=y;
  cs->joyx[0]=x;
  sprintf(tempstr,"Stick minimum (%3d,%3d)",x,y);
  Gwritestr(8,10,bluecol,tempstr,23);
  delay(500);
  joyclearbuts();

  Gwritestr(5,5,bluecol,"Move joystick all the way Back",30);
  Gwritestr(5,6,bluecol,"and Right.  Press either      ",30);
  Gwritestr(5,7,bluecol,"joystick button when there.   ",30);
  butn=0;
  if (!getjoydat(&x,&y,&butn)) return;
  cs->joyy[4]=y;
  cs->joyy[2]=(((cs->joyy[4]-cs->joyy[0])/2)+cs->joyy[0]);
  cs->joyy[1]=(((cs->joyy[2]-cs->joyy[0])/2)+cs->joyy[0]);
  cs->joyy[3]=(((cs->joyy[4]-cs->joyy[2])/2)+cs->joyy[2]);
  cs->joyx[4]=x;
  cs->joyx[2]=(((cs->joyx[4]-cs->joyx[0])/2)+cs->joyx[0]);
  cs->joyx[1]=(((cs->joyx[2]-cs->joyx[0])/2)+cs->joyx[0]);
  cs->joyx[3]=(((cs->joyx[4]-cs->joyx[2])/2)+cs->joyx[2]);

  sprintf(tempstr,"Stick maximum (%3d,%3d)",x,y);
  Gwritestr(8,10,bluecol,tempstr,23);
  delay(500);
  joyclearbuts();

  filestatus=SaveConfigData(cs);
  if (filestatus!=TRUE)
    {
    Gwritestr(5,15,redcol,"Unable to write data to disk! ",30);
    }
  }

static char getjoydat(unsigned int far *x,unsigned int far *y, unsigned int far *butn)
  {
  char key=0;
  while (((*butn)==0)&&(key!=27))
    {
    ReadJoyStick(x,y,butn);
    key=bioskey(1)&255;
    delay(50);
    }
  if (key==27) return(FALSE);
  return(TRUE);
  }

static int PickupBloc(void)
  {
  register int j;
  char spot=0;
  touchblk t;
  char tempor;

  if (ci.inv[9]!=BACKBL) return(FALSE);
  while (ci.inv[spot]!=BACKBL) spot++;
  GetTouchbl(&t);
  for (j=0;j<6;j++)
    if (t.blks[j][0]!=MLEN)
      if (blks[0][ t.blknum[j] ].obj)
        {
        tempor=really_checktouch(t.blks[j][0],t.blks[j][1],t.blknum[j]);
        if (tempor != 2)
          {
          ci.inv[spot] = t.blknum[j];
          checkseq_fortie(t.blknum[j],TRUE);
          return(TRUE);
          }
        }
  return(FALSE);
  }

static char DropBloc(void)
  {
  int bluecol,Yellow;    /* blue border color, yellow select color  */
  char done=0;
  register int k,j;
  char chosen;
  int x,y,keytyp;
  unsigned long int ClockPause;

  ClockPause=clock;                   // Save so no time passes when fn key hit.
  setvect(0x9,OldKbd);                // Reset old keyboard handler address
  nosound();
  MoveWindow(0);                      // Just clear it all and use the whole
  BoxFill(0,0,SIZEX-1,SIZEY-1,0);     // screen. No time for fancy
  ViewWindow(0);                      
  MoveViewScreen(0,0);
  ci.topthere=FALSE;
  ci.bottomthere=FALSE;

  bluecol=PalFindCol(0,0,60,colors);
  Yellow=PalFindCol(60,60,0,colors);
  Box(10,10,310,190,bluecol);
  Box(15,15,305,185,bluecol); 

  Box(30,30,35+(5*(BLEN+5)),35+(2*(BLEN+5)),bluecol);
  for (k=0;k<2;k++)
    for (j=0;j<5;j++)
      {
      Box(34+j*(BLEN+5),34+k*(BLEN+5),35+BLEN+j*(BLEN+5),35+BLEN+k*(BLEN+5),bluecol);
      if ( ci.inv[(5*k)+j] != BACKBL )
        drawblk(35+j*(BLEN+5),35+k*(BLEN+5),blks[0][ci.inv[(5*k)+j]].p[0]);
      }
  j=0; k=0;
  Box(33+j*(BLEN+5),33+k*(BLEN+5),36+BLEN+j*(BLEN+5),36+BLEN+k*(BLEN+5),Yellow);
  Gwritestr(5,13,bluecol,"Choose object to drop",21);
  Gwritestr(5,15,bluecol,"Use arrow keys to select,",25);
  Gwritestr(5,16,bluecol,"<Enter> to accept, <ESC> ",25);
  Gwritestr(5,17,bluecol,"to quit without dropping.",25);

  while (!done)
   {
   keytyp=bioskey(0);
   switch(keytyp&0x00FF)
     {
     case 0:
       Box(33+j*(BLEN+5),33+k*(BLEN+5),36+BLEN+j*(BLEN+5),36+BLEN+k*(BLEN+5),0);
       switch(keytyp>>8) {
        case 72:  // Up Arrow
        case 80:  // Down Arrow
           k ^= 1;
           break;
        case 75:  // Left Arrow
           j = (j+4)%5;
           break;
        case 77:  // Right Arrow
           j = (j+1)%5;
           break;
        }
       Box(33+j*(BLEN+5),33+k*(BLEN+5),36+BLEN+j*(BLEN+5),36+BLEN+k*(BLEN+5),Yellow);
       break;
     case 13:
       chosen = k*5+j;
       if (ci.inv[chosen] == BACKBL) break;
       map[chr.y[0]][chr.x[0]].blk=ci.inv[chosen];
       checkseq_fortie(ci.inv[chosen],FALSE);
       for (k=0;k<2;k++)
         for (j=0;j<5;j++)
           if ( (k*5+j) == 9) ci.inv[9]=BACKBL;
           else if ( (k*5+j) >= chosen) ci.inv[k*5+j]=ci.inv[k*5+j+1];
       done=2;
       break;
     case 27:
       done=1;
       break;
     }
   }
  // End Stuff
  scroll.zero=0;        // Set Page scrolling variable to initial states
  scroll.nx=scroll.ny=0;
  scroll.Totalx=scroll.Totaly=0;
  drawmap(mx,my);       // Draw the display map
  StealKbd();
  keydn[chr.sq[DROPSEQ].key>>8]=0;
  clock=ClockPause;  
  return(done-1);
  }

static void checkseq_fortie(unsigned char block,char setting)
 {
  register int i;
   
  for (i=0;i<MAXSEQ;i++)
    if (chr.sq[i].bloc == block)
      ci.seqon[i] = setting;
 }

static void drawchars(void)
  {
#ifndef NOCHAR
  ci.update=FALSE;
  if (ci.scrx>OFFSCRN) // scrx has not yet been calculated
    {
    ci.EraseAddr=zeroaddon+(ci.scrx)+(ci.scry*SIZEX);
    ci.oldx=ci.scrx; ci.oldy=ci.scry;
    if (ci.curtop<CHARBL)  // &(ci.curbottom< CHARBL)
      {
        // Get and save block behind character
      ci.topthere=TRUE;
      BufGetBlk(ci.scrx,ci.scry-BLEN,ci.savetop);
        // draw character top
      Sdrawspbloc(ci.scrx,ci.scry-BLEN,CHRFRAME.orient&15,&blks[2][ci.curtop]);
      }
    else ci.topthere=FALSE;
    if (ci.curbottom<CHARBL)
      {
      ci.bottomthere=TRUE;
      BufGetBlk(ci.scrx,ci.scry,ci.savebottom);
      Sdrawspbloc(ci.scrx,ci.scry,CHRFRAME.orient>>4,&blks[2][ci.curbottom]);
      }
    else ci.bottomthere=FALSE;
    }
#endif
  }

static void erasechars(void)
  {
#ifndef NOCHAR
  if (ci.topthere)
    {
    BufDrawBlkAddr(ci.EraseAddr-(BLEN*SIZEX),ci.savetop);
    AddDirtyRect(ci.EraseAddr-(BLEN*SIZEX),BLEN,BLEN);
    }
  if (ci.bottomthere)
    {
    BufDrawBlkAddr(ci.EraseAddr,ci.savebottom);
    AddDirtyRect(ci.EraseAddr,BLEN,BLEN);
    }
#endif
  }


void minfo::Draw(int MMnum)
  {
  int temp;
  int num;
  Coord2d LowBnd(BLEN/2,BLEN/2), UpBnd(SIZEX-BLEN,SIZEY-BLEN);

  if (ScrPos.In(LowBnd,UpBnd)&&((temp=MonPicNum(MMnum))<MONBL))
    {
    save=MonsterMem+(MonsOnScrn*(BLEN*BLEN));
    BufGetBlk(ScrPos.x,ScrPos.y,save);
    MonsOnScrn++;
    PicDrawn=TRUE;
    Sdrawspbloc(ScrPos.x,ScrPos.y,orient,&blks[1][temp]);
    OldPos=ScrPos;
    EraseAddr=ScrPos.x+(SIZEX*ScrPos.y)+zeroaddon;
    }
  else PicDrawn=FALSE;
  }

void minfo::Erase(void)
  {
  if (PicDrawn)        // Monster being used
    {                  // -1 = not saved flag, so don't erase
    BufDrawBlkAddr(EraseAddr,save);
    AddDirtyRect(EraseAddr,BLEN,BLEN);
    PicDrawn=FALSE;
    OldPos.Set(OFFSCRN,OFFSCRN);
    }
  }

static void changeblks(void)
  {
  register int lp;
  int lx,ly;
  int tx,ty;
  int tmap;
  char Changed[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  for (lp=0;lp<BACKBL;lp++)
    {
    tmap=blkmap[lp];                           // Fetch displayed block num
    if ((nxtchg[lp]<=clock)&&(blks[0][tmap].nextbl != tmap)) // Time to change a block?
      {                                        // to the next one blk[].nextbl
      blkmap[lp]=blks[0][tmap].nextbl;         // Save the next block number
      Changed[lp>>3]|=1<<(lp&7);
      if (blks[0][tmap].ntime<=0)              // Will block EVER change?
        nxtchg[lp]=0;                          // No, so set time as NEVER.
      else 
        nxtchg[lp]+=blks[0][blkmap[lp]].ntime; // set the next time to change
      }
    }

  for (lx=mx+1;lx<mx+SCRNBLEN;lx++)        // Go through looking for
    for (ly=my+1;ly<my+SCRNBWID;ly++)      // changed blocks.
      {
      lp=map[ly%MWID][lx%MLEN].blk;
      if (Changed[lp>>3]&(1<<(lp&7)))
        {
        tx=((lx-mx)*BLEN)-scroll.nx;
        ty=((ly-my)*BLEN)-scroll.ny;
        BufDrawBlk(tx,ty,blks[0][blkmap[lp]].p[0]);
        AddDirtyRect(tx,ty,tx+BLEN-1,ty+BLEN-1);
        }
      }
  }

static int movechars(void)
  {
  int retval=0;
  int tx=0,ty=0;
  int charmoved=FALSE;
  int gravmoved=FALSE;
  char oldseq,oldframe;
  char ZeroBefore=FALSE;
  touchblk t;   // Array telling what blocks are touching character
  
  ci.scrx = ci.nextx;
  ci.scry = ci.nexty;
  gravmoved=MoveCharGrav(0); // Test to see if character should be affected by gravity

  ci.update|=HandleJStick(0);   // See if joystick is giving info

  if (clock>=ci.NextAni)
    {
    oldframe=chr.cframe;
    oldseq  =chr.cseq;
    if (chr.cframe==-1)     // FIRST FRAME in a sequence
      {
      if (chr.cseq==DROPSEQ)    // First frame, drop sequence,
        {
        if (!DropBloc())        // so drop the block.
          {
          chr.cseq=IDLESEQ;
          keystkptr=0;         // Reset all Keyboard and character stuff
          curkey=0;
          }
        }
      if (chr.cseq==PICKUPSEQ) // First frame, pickup sequence
        {
        if (!PickupBloc())     // so pick up a block.
          {
          chr.cseq=IDLESEQ;
          keystkptr=0;         // Reset all Keyboard and character stuff
          curkey=0;
          }
        }
      // Sound after pickup and drop so that sound will only be done if
      // pickup/drop successful.
      if (chr.sq[chr.cseq].sound<LASTSND) DoSound(chr.sq[chr.cseq].sound);
      }
    do
      {
      if (chr.cframe==0) ZeroBefore++;
      chr.cframe++;
      while ((CHRFRAME.pic2 == CHARBL+1)||(chr.cframe>=MAXFRAME))
        {                                 // ONE AFTER LAST FRAME
        if (chr.cseq==DIESEQ)
          { chr.cframe--; return(TRUE); } // End after last frame of die seq

        if ((ci.meter[chr.cseq]<INF_REPEAT)&&(chr.cseq>4))
          ci.meter[chr.cseq]--;   // If seq has a rep ctr, then decrement the counter by 1
      
        // To make key work only once, add to this if statement
        if (((ci.meter[chr.cseq]<=0)&&(chr.cseq>4))||
           (chr.cseq==HURTSEQ)||      // Only do these sequences once
           (chr.cseq==DROPSEQ)||
           (chr.cseq==PICKUPSEQ)||
           (chr.sq[chr.cseq].norepeat)||
           ((keydn[(chr.sq[chr.cseq].key>>8)]==0)&&  // Quit if key lifted
            (curkey != JSTICKKEY))||
           (ZeroBefore>1)   // gone 2 times around w/ out getting out - inf loop, force break out.
           )
          {
          PopKey(curkey);       // Do The Previous sequence
          ZeroBefore=FALSE;
          }
        else
          {
          chr.cframe=0;
          if (CHRFRAME.pic2 == CHARBL+1)
            {
            if (chr.cseq==IDLESEQ) CHRFRAME.pic2=0;
            else PopKey(curkey);       // Do The Previous sequence
            ZeroBefore=FALSE;
            }
          }
        }
      ci.NextAni=clock+CHRFRAME.pause;
      if ((CHRFRAME.pic2 == MONBL)&&(!NoMons))  // Birth a monster
        {
        CharBirthMon(chr.x[0],chr.y[0]+(CHRFRAME.y/BLEN)-1,CHRFRAME.pic1);
        ty=0;
        }
      else // Move character
        {  
        ci.curtop=CHRFRAME.pic1;
        ci.curbottom=CHRFRAME.pic2;
        if(CHRSEQ.Momentum)
          {
          tx=CHRFRAME.x;  // tx,ty = temporary variables
          ty=CHRFRAME.y;
          ci.gravx+=tx; // through gravx and gravy.
          ci.gravy+=ty;
          if (!((tx==0)&&(ty==0)))  //Remember the character's last direction and speed
            { ci.netx=tx; ci.nety=ty; } // For monsters going "away"
          tx=0;ty=0;  // Don't add it to tx,ty, cause it will be added below
          }
        else
          {
          tx=CHRFRAME.x;  // tx,ty = the char's next displacement
          ty=CHRFRAME.y;
          }
        if (!((tx==0)&&(ty==0)))  //Remember the character's last direction and speed
          { charmoved=TRUE; ci.netx=tx; ci.nety=ty; } // For monsters going "away"
        }
      } while (CHRFRAME.pic2>CHARBL); // Keep going until you find a picture to draw (verses a monster being birthed).
    if ((chr.cseq!=oldseq)||(chr.cframe!=oldframe)) ci.update=TRUE;
    }
 
  tx+=ci.gravx;  // Add Gravity's effect to the displacement
  ty+=ci.gravy;

  if ((charmoved)||(gravmoved)||(monsolid)||(ci.update))
    {
    int oldcx,oldcy,oldy,oldx;

    oldcx=chr.x[1];  // Remember the last position inside current block
    oldcy=chr.y[1];
    oldy=chr.y[0];
    oldx=chr.x[0];

    if (tx>MAXGRAV)       tx = MAXGRAV;     // Limit the character's maximum
    if (tx< (-1*MAXGRAV)) tx = -1*MAXGRAV;  // speed.
    if (ty>MAXGRAV)       ty = MAXGRAV;
    if (ty< (-1*MAXGRAV)) ty = -1*MAXGRAV; 
    chkscrnbounds(&tx,&ty);  // Change displacement if character is about to
                             // Run off the screen

    chr.x[1]+=tx;         // Move the character's x position variable
    chr.y[1]+=ty;         // Move y var for Checktouch
            // Has character gone off the edge of the block
    while (chr.x[1]<0)     { chr.x[1]+=BLEN; chr.x[0]--; }
    while (chr.x[1]>=BLEN) { chr.x[1]-=BLEN; chr.x[0]++; }
    while (chr.y[1]<0)     { chr.y[1]+=BLEN; chr.y[0]--; }
    while (chr.y[1]>=BLEN) { chr.y[1]-=BLEN; chr.y[0]++; }
    chkmapbounds();
    GetTouchbl(&t);
    CheckTouch();
    chr.y[1]=oldcy;
    chr.y[0]=oldy;
    chr.x[1]=oldcx;
    chr.x[0]=oldx;

    if (!NoMons)
      {
      if ((monsolid&SOLRIG)&&(!(monsolid&SOLLEF)))
        {
        chr.x[1]=mi[MMsolid].curx[1];
        chr.x[0]=mi[MMsolid].curx[0]-1;
        tx=0;
        }
      if ((monsolid&SOLLEF)&&(!(monsolid&SOLRIG)))
        {
        chr.x[1]=mi[MMsolid].curx[1];
        chr.x[0]=mi[MMsolid].curx[0]+1;
        tx=0;
        }
      if ((monsolid&SOLTOP)&&(!(monsolid&SOLBOT)))
        {
        chr.y[1]=mi[MMsolid].cury[1];
        chr.y[0]=mi[MMsolid].cury[0]+1;
        ty=0;
        }
      if ((monsolid&SOLBOT)&&(!(monsolid&SOLTOP)))
        {
        chr.y[1]=mi[MMsolid].cury[1];
        chr.y[0]=mi[MMsolid].cury[0]-1;
        ty=0;
        }
      }
    chr.x[1]+=tx;         // Move the character's x position variable
    while (chr.x[1]<0)     { chr.x[1]+=BLEN; chr.x[0]--; }
    while (chr.x[1]>=BLEN) { chr.x[1]-=BLEN; chr.x[0]++; }
    while (chr.y[1]<0)     { chr.y[1]+=BLEN; chr.y[0]--; }
    while (chr.y[1]>=BLEN) { chr.y[1]-=BLEN; chr.y[0]++; }
    chkmapbounds();

    GetTouchbl(&t);
    retval=chksolids(tx,0,&t);
    OnGround(retval); 
    #ifdef SOLDEBUG    
    Gwritestr(1,12,25,"     ",5);
    Gwritestr(1,11,25,"    ",4);
    Gwritestr(1,13,25,"   ",3);
    Gwritestr(1,14,25,"      ",6);
    #endif
//    CheckTouch();
   
    if (retval&SOLRIG) // Right
      { 
      #ifdef SOLDEBUG    
      Gwritestr(1,12,25,"Right",5);
      #endif
      chr.x[1]=0;
      }
    if (retval&SOLLEF) // Left
      { 
      #ifdef SOLDEBUG    
      Gwritestr(1,11,25,"Left",4);
      #endif
      chr.x[0]++;
      chr.x[1]=0;
      }

    chr.y[1]+=ty;

            // Has character gone off the edge of the block
    while (chr.y[1]<0)     { chr.y[1]+=BLEN; chr.y[0]--; }
    while (chr.y[1]>=BLEN) { chr.y[1]-=BLEN; chr.y[0]++; }
    chkmapbounds();
    GetTouchbl(&t);
    retval=chksolids(0,ty,&t);
    OnGround(retval); 
  
    if (retval&SOLBOT)  // Bottom
      {
      #ifdef SOLDEBUG    
      Gwritestr(1,14,25,"Bottom",4);
      #endif
      chr.y[1]=0;
      }
    if (retval&SOLTOP) // Top
      { 
      #ifdef SOLDEBUG    
      Gwritestr(1,13,25,"Top",3);
      #endif
      chr.y[0]++;
      chr.y[1]=0;
      }
                 // Calculate new coordinates
    if (mx<chr.x[0]) tx=(chr.x[0]-mx)*BLEN;
    else tx=(MLEN+(chr.x[0]-mx))*BLEN;
    tx+=chr.x[1];
    ci.nextx = tx-scroll.nx;     // Save the new char x coord
 
    if (my<chr.y[0]) ty=(chr.y[0]-my)*BLEN;
    else ty=(MWID+(chr.y[0]-my))*BLEN;
    ty+=chr.y[1];
    ci.nexty = ty-scroll.ny;     // Save the new char y coord
    }
  return (FALSE);  // False means that Character is not dead
  } 

static char MonInRange(int MMnum) // Return TRUE if monster should be in its
  {                               // Attack pattern.
  if ((abs(mi[MMnum].curx[0]-chr.x[0])<ATTACKX)&&
      (abs(mi[MMnum].cury[0]-chr.y[0])<ATTACKY))
    { return (TRUE); }
  return (FALSE);
  }


boolean minfo::ChkWalls(Coord2d *Move)
  {
  Coord2d Temp;
  int walls;
  boolean solidhit=FALSE;
  minfo *moni=this;

  Temp.x=moni->curx[0];
  Temp.y=moni->cury[0];

  if ((moni->curx[1]+(Move->x))>BLEN) Temp.x+=2;
  if ((moni->cury[1]+(Move->y))>BLEN) Temp.y+=2;
  if ((moni->curx[1]+(Move->x))<0)    Temp.x--;
  if ((moni->cury[1]+(Move->y))<0)    Temp.y--;
 
  if (Temp.x>=MLEN) Temp.x-=MLEN;
  if (Temp.x<0)     Temp.x+=MLEN;
  if (Temp.y>=MLEN) Temp.y-=MWID;
  if (Temp.y<0)     Temp.y+=MWID;

  walls=blks[0][blkmap[map[Temp.y][Temp.x].blk]].solid;
  
  Temp.x=(moni->curx[1]+Move->x);
  Temp.y=(moni->cury[1]+Move->y);
  
  if ((Temp.x<0)&&(walls&SOLRIG))
    {
    Move->x=-(moni->curx[1]);
    solidhit=TRUE;    
    }
  if ((Temp.x>=BLEN)&&(walls&SOLLEF))
    {
    Move->x=BLEN-(moni->curx[1]);
    solidhit=TRUE;
    }    
  if ((Temp.y<0)&&(walls&SOLBOT))
    {
    Move->y=-(moni->cury[1]);
    solidhit=TRUE;    
    }
  if ((Temp.y>=BLEN)&&(walls&SOLTOP))
    {
    Move->y=BLEN-(moni->cury[1]);
    solidhit=TRUE;    
    }
  return(solidhit);
  }

static int MonPicNum(int MMnum)
  {
  int temp;
  if ((mi[MMnum].CurPic>=0)&&(mi[MMnum].CurPic<LASTSL))
    {
    temp=mon[mm[MMnum].monnum].pic[mi[MMnum].CurPic][0];

    if ((temp<MONBL)&&(temp>=0)) return(temp);
    else return(MONBL);
    }
  else return (MONBL);
  }

static int ChkMonChr(int MMnum)
  {
  int retval=FALSE;
  Coord2d Mon,Chr,Maxx,Least;
  int mons;
  minfo *moni = &mi[MMnum];

  if (mi[MMnum].ScrPos.x==OFFSCRN) return(FALSE);

  Least.x=Min(moni->ScrPos.x,ci.scrx);
  Least.y=Min(moni->ScrPos.y,ci.scry);

  Chr = MakeCoord2d(ci.scrx,ci.scry) - Least;
  Mon = moni->ScrPos                 - Least;

  Maxx = MakeCoord2d(Max(Chr.x,Mon.x),Max(Chr.y,Mon.y));

  if (((Mon.x<=Chr.x+BLEN)&&(Mon.x+BLEN>=Chr.x))&&
      ((Mon.y<=Chr.y+BLEN)&&(Mon.y+(BLEN*2)>=Chr.y)))  // *2 for 2-high chars
    {  // Monsters and character might be touching
    #ifdef GENDEBUG
    Gwritestr(20,20,15,"      ",6);
    #endif
    if ( (mons=MonPicNum(MMnum))>=MONBL) return(FALSE);
    retval=0;
    if (CHRFRAME.pic2<CHARBL) retval=PicChkTouch(Chr.x,Chr.y,(char*)blks[2][CHRFRAME.pic2].p,Mon.x,Mon.y,(char*)blks[1][mons].p);
    if (CHRFRAME.pic1<CHARBL) retval|=PicChkTouch(Chr.x,Chr.y,(char*)blks[2][CHRFRAME.pic1].p,Mon.x,Mon.y+BLEN,(char*)blks[1][mons].p);
    if (retval)
      {
      mons=blks[1][mons].solid;
      if (mons>0)
        {
        if (Maxx.x>=Maxx.y)   // Push horizontzally
          {
          if (Chr.x==0) monsolid|=(SOLRIG&mons);
          else          monsolid|=(SOLLEF&mons);
          }
        else        // Push Vertically
          {
          if (Chr.y==0) monsolid|=(SOLBOT&mons);
          else          monsolid|=(SOLTOP&mons);
          }
        MMsolid=MMnum;
        }
      }
    if ((monsolid&SOLRIG)&&(monsolid&SOLLEF))  // 2 monster stuff
      {
      if (Chr.y==0) monsolid|=(SOLBOT&mons);      // Push him in the Y
      else          monsolid|=(SOLTOP&mons);      // 'Cause no where to go in x
      }
    if ((monsolid&SOLBOT)&&(monsolid&SOLTOP))  // 2 monster stuff
      {
      if (Chr.x==0) monsolid|=(SOLRIG&mons);      // Push him in X 'Cause no Y
      else          monsolid|=(SOLLEF&mons);
      }
    }
    #ifdef HITDEBUG
    if (retval)
      {
      DoSound(1); Gwritestr(30,10,clock%256,"HIT!",4);
      while(keydn[46]==0);
      while(keydn[46]==1);
      }
    else Gwritestr(30,10,19,"MISS",4);
    #endif
  return(retval);
  }  

static int DoCollision(int mm1,int mm2,int where)
  {
  char tch=FALSE;
  int mon1,mon2;
  int monpic1,monpic2;
  int addx=0,addy=0;
  
  #ifdef HITDEBUG
  Gwritestr(5,4,(clock%128),"DoColl",6);
  #endif

  mon1=mm[mm1].monnum;
  mon2=mm[mm2].monnum;
  
  if ((mon1<LASTMON)&&(mon2<LASTMON))
    {
    if(mon[mon1].power!=mon[mon2].power)
      {
      if (where&1) addx=BLEN;
      if (where&2) addy=BLEN;
     

      if ((monpic1=MonPicNum(mm1))>=MONBL) return(mm2);
      if ((monpic2=MonPicNum(mm2))>=MONBL) return(mm1);

      tch=PicChkTouch(mi[mm1].curx[1]+addx,mi[mm1].cury[1]+addy,blks[1][monpic1].p[0],mi[mm2].curx[1],mi[mm2].cury[1],blks[1][monpic2].p[0]);
      if (tch)
        {
        if (mon[mon1].power<mon[mon2].power)
          {
          KillMon(mm1); 
          if (mi[mm2].fromchar>0) 
            {
            ci.score += mon[mon1].upscore;
            DoSound(KILLSOUND);
            KillMon(mm2);
            return(LASTMM);
            }
          return(mm2);
          }
        if (mon[mon1].power>mon[mon2].power)
          { 
          KillMon(mm2); 
          if (mi[mm1].fromchar>0) 
            {
            ci.score += mon[mon2].upscore;
            DoSound(KILLSOUND);
            KillMon(mm1);
            return(LASTMM);
            }
          return(mm1);
          }
        }
      }
    }
  return(mm2);
  }

static char PicChkTouch(int x1,int y1,unsigned char far *pic1, int x2,int y2,unsigned char far *pic2)
  {
  register int lx,ly;
  int dx,dy;
  int ctr1,ctr2;

  #ifdef HITDEBUG
  static unsigned char col=0;
  #endif
  
  dx=abs(x2-x1);
  dy=abs(y2-y1);
  x2-=dx; x1-=dx;
  y2-=dy; y1-=dy;

  #ifdef HITDEBUG
  Gwritestr(20,20,col++,"IN Check ",9);
  drawblk(100+x1,100+y1,pic1);
  drawblk(100+x2,100+y2,pic2);
  #endif
  if (dx>BLEN) return(FALSE); // Can't touch if blks not touching
  if (dy>BLEN) return(FALSE);

  if ((x2>=x1)&&(y2>=y1))
    {
    #ifdef HITDEBUG
    Gwritestr (30,20,col,"1",1);
    #endif
    ctr1=BLEN*dy;
    ctr2=0;
    for (ly=0;ly< (y1+BLEN)-y2; ly++)
      {
      ctr1+=dx;
      for (lx=0;lx< (x1+BLEN)-x2; lx++,ctr1++,ctr2++)
        {
        if (( (*(pic1+ctr1))!=TRANSCOL)&((*(pic2+ctr2)) !=TRANSCOL))
          {
          return(TRUE);
          }
        }
      ctr2+=dx;
      }
    return(FALSE);
    }
  if ((x2>=x1)&&(y2<=y1))
    {
    #ifdef HITDEBUG
    Gwritestr (30,20,col,"2",1);
    #endif
    ctr1=0;
    ctr2=dy*BLEN;
    for (ly=0;ly< (y2+BLEN)-y1; ly++)
      {
      ctr1+=dx;
      for (lx=0;lx< (x1+BLEN)-x2; lx++,ctr1++,ctr2++)
        {
        if (( *(pic1+ctr1)!=TRANSCOL)&(*(pic2+ctr2) !=TRANSCOL))
          {
          return(TRUE);
          }
        }
      ctr2+=dx;
      }
    return(FALSE);
    }
  if ((x2<=x1)&&(y2>=y1))
    {
    #ifdef HITDEBUG
    Gwritestr (30,20,col,"3",1);
    #endif
    ctr2=0;
    ctr1=dy*BLEN;
    for (ly=0;ly< (y1+BLEN)-y2; ly++)
      {
      ctr2+=dx;
      for (lx=0;lx< (x2+BLEN)-x1; lx++,ctr1++,ctr2++)
        {
        if (( *(pic1+ctr1)!=TRANSCOL)&(*(pic2+ctr2) !=TRANSCOL))
          {
          return(TRUE);
          }
        }
      ctr1+=dx;
      }
    return(FALSE);
    }
  if ((x2<=x1)&&(y2<=y1))
    {    
    #ifdef HITDEBUG
    Gwritestr (30,20,col,"4",1);
    #endif
    ctr2=(BLEN*dy);
    ctr1=0;

    for (ly=0;ly< (y2+BLEN)-y1; ly++)
      {
      ctr2+=dx;
      for (lx=0;lx< (x2+BLEN)-x1; lx++,ctr1++,ctr2++)
        {
        if (( *(pic1+ctr1)!=TRANSCOL)&&(*(pic2+ctr2) !=TRANSCOL))
          {
          return(TRUE);
          }
        }
      ctr1+=dx;
      }
    return(FALSE);
    }
  return(FALSE);
  }

static int moveallmons(void)
  {
  int MMnum=0;
  int MONnum=0;  
  Coord2d Temp;
  Coord2d Move;
  int anymoved=FALSE;
  int survivor;
  int speed;
  minfo *Minfo;
  monstruct *Mon;

  Minfo=&mi[0];
  for (MMnum=0;MMnum<LASTMM;MMnum++,Minfo++)
    {
    MONnum=mm[MMnum].monnum;
    Mon   =&mon[MONnum];
    if (MONnum<LASTMON)                // Monster being used
      {
      if (clock>=Minfo->NextAni)
        {                              // It's time to change the picture 
        survivor=Minfo->CurPic;        // Save old to see if change
        Minfo->CurPic++;               // Inc curpic and check bounds.
        if ((Minfo->CurPic>=LASTSL)||(Mon->pic[Minfo->CurPic][0]>=MONBL))
           {
           if (Mon->pic[0][0]<MONBL) Minfo->CurPic=0;
           else Minfo->CurPic=LASTSL;
           }
        if (Minfo->CurPic<LASTSL) // Set next animation time
          Minfo->NextAni=clock+Mon->pic[Minfo->CurPic ][1];
        else Minfo->NextAni=clock+100000; // No picture, never try to change

        if ((Minfo->NextAni!=survivor)&&(onscrn(Minfo->curx[0],Minfo->cury[0],&Minfo->ScrPos)))
          anymoved=TRUE; // Redraw everything if pic has changed and on scrn
        }

      if (clock>=Minfo->NextMove)              //Time to move monster?
        {
        speed=(clock-Minfo->NextMove+1)/MONMOVESPEED;  // How far?
        Minfo->NextMove=clock+MONMOVESPEED;    //Set next time to move

        if ( (Mon->activate!=FALSE)            // If near the character and
           &&(Minfo->CurMarch!=ATTACKMOVEMENT) //   and attack path is set
           &&(MonInRange(MMnum)) )             //   start using it.
          { 
          Minfo->CurMarch= ATTACKMOVEMENT;
          Minfo->dest    = 1;
          }
        if ( (Minfo->CurMarch==ATTACKMOVEMENT)    // Currently attacking
           &&(Mon->activate==2) )                 // Relative to player
          {
          Minfo->Target.Set(
             (Mon->march[ATTACKMOVEMENT][Minfo->dest].x+chr.x[0]+MLEN)%MLEN,
             (Mon->march[ATTACKMOVEMENT][Minfo->dest].y+chr.y[0]+MWID)%MWID);
          Minfo->DistToPlace(Minfo->Target);
          Minfo->offl=0;
          Move=Minfo->Bresenhams((Mon->march[ATTACKMOVEMENT][Minfo->dest].pic)*speed);
          if (Minfo->offl==-1)
            {
            Minfo->dest++;
            Minfo->offs=0;
            if ( (Minfo->dest==MAXMARCH)||(Mon->march[ATTACKMOVEMENT][Minfo->dest].pic==0) )
              {                                 // End of march, so
              Minfo->CurMarch=NORMALMOVEMENT;   // go back to normal pattern
              Minfo->dest=0;
              }
            else Minfo->orient=Mon->march[ATTACKMOVEMENT][Minfo->dest].orient;
            }
          }
        else if (Mon->towards==0)   // monster in any sort of set path.
          {
//          gotoxy(1,10);
//          printf("Path %d Offl:%d Dest:%d ",MMnum,Minfo->offl,Minfo->dest);
          if (Minfo->offl==-1) if (MonEndPath(MMnum,MONnum)) { anymoved=TRUE; continue;}
//DEBUG
//          if (Minfo->dest>MAXMARCH) printf("DEST OUT OF BOUNDS!\n");
//          if ( (Minfo->CurMarch!=1)&&(Minfo->CurMarch!=0))
//            { printf("ERROR %d\n",Minfo->CurMarch); }
//          if (Minfo->dest<0) printf("Dest Error\n");

          Move=Minfo->Bresenhams((Mon->march[Minfo->CurMarch][Minfo->dest-1].pic)*speed);
//          printf("(%3d,%3d) ",Move.x,Move.y);
          }
        else if (Mon->towards<64)  // Move mon always towards or away from char
          {
          if (!Minfo->fromchar)
            {
            Minfo->DistToPlace(MakeCoord2d(chr.x[0],chr.y[0]));
            Minfo->Delta.x+=(chr.x[1]-Minfo->curx[1]);
            Minfo->Delta.y+=(chr.y[1]-Minfo->cury[1]);
            if (Mon->towards<0)       //If moving away, reverse the sign
              Minfo->Delta *= -1;

            if (Minfo->Delta.x<0) Minfo->orient |= 4;  // Keep monster facing
            else                  Minfo->orient = 0;   // correctly.
            }
          Move=Minfo->Bresenhams(abs(Mon->towards)*speed);
          }
        else                       // random movement
          {
          if ((Minfo->curx[0]==Minfo->Target.x)&&(Minfo->cury[0]==Minfo->Target.y))
            {
            if (random(2)) Minfo->Target.x = (Minfo->Target.x+random(7)+5)%MLEN;
            else
              {
              Minfo->Target.x -= (random(7)+5);
              if (Minfo->Target.x<0) Minfo->Target.x += MLEN;
              }
            if (random(2)) Minfo->Target.y = (Minfo->Target.y+random(7)+5)%MWID;
            else
              {
              Minfo->Target.y -= (random(7)+5);
              if (Minfo->Target.y<0) Minfo->Target.y += MWID;
              }
            Minfo->DistToPlace(Minfo->Target);
            Minfo->offl=0;
            Minfo->offs=0;
            }
          Move=Minfo->Bresenhams((Mon->towards>>6)*speed);
          }


        if (Mon->thru)
          if (Minfo->ChkWalls(&Move)&&(Minfo->fromchar>0))
            if (KillMon(MMnum)) continue;  // Don't do monster since its dead.

          
        if ( (Move.x==0)&&(Move.y==0) )  // Keep monsters moving
          Minfo->Target.Set(Minfo->curx[0],Minfo->cury[0]);
        else
          {
          Minfo->curx[1]+=Move.x;
          Minfo->cury[1]+=Move.y;
          }

          // Erase the monster from the old map location.
        if (map[Minfo->cury[0]][Minfo->curx[0]].mon==MMnum)
          map[Minfo->cury[0]][Minfo->curx[0]].mon=LASTMM;
   
        while (Minfo->curx[1]>=BLEN) { Minfo->curx[1]-=BLEN; Minfo->curx[0]++; }
        while (Minfo->curx[1]<0)     { Minfo->curx[1]+=BLEN; Minfo->curx[0]--; }
        while (Minfo->cury[1]>=BLEN) { Minfo->cury[1]-=BLEN; Minfo->cury[0]++; }
        while (Minfo->cury[1]<0)     { Minfo->cury[1]+=BLEN; Minfo->cury[0]--; }
        if (Minfo->curx[0]<0)          Minfo->curx[0]+=MLEN;
        if (Minfo->cury[0]<0)          Minfo->cury[0]+=MWID;
        if (Minfo->curx[0]>=MLEN)      Minfo->curx[0]-=MLEN;
        if (Minfo->cury[0]>=MLEN)      Minfo->cury[0]-=MWID;
   
        Temp = Minfo->ScrPos;
        if (onscrn(Minfo->curx[0],Minfo->cury[0],&Minfo->ScrPos))
          {      
          Minfo->ScrPos.x+=Minfo->curx[1];
          Minfo->ScrPos.y+=Minfo->cury[1];
          if (Temp != Minfo->ScrPos) anymoved=TRUE;
          anymoved=TRUE;
          }
        else Minfo->ScrPos.Set(OFFSCRN,OFFSCRN);  // Monster off of screen

        // Check to see if monster has touched a character

        if ((Minfo->CurPic>=0)&&(Minfo->CurPic<LASTSL)&&(ChkMonChr(MMnum)))
          {
          if (Minfo->fromchar!=1)
            {
            int temp;
            if ( (temp=MonPicNum(MMnum))<=MONBL) temp=DoBlockTouchChar(&blks[1][temp])-1;
            if ((mon[mm[MMnum].monnum].power<CHARPOWER)&&
               (mon[mm[MMnum].monnum].end!=-2)) // -2 means never kill monster
              {
              if (KillMon(MMnum))
                {
                if (!temp) DoSound(KILLSOUND);  // Dosound must be before DoBlockTouchchar
                continue;
                }
              anymoved=TRUE;
              }
            }
          }   
        if (mm[MMnum].monnum<LASTMON)     // Monster was not killed above
          {
          survivor=MMnum;  // check for one monster killing another
          if (map[Minfo->cury[0]][Minfo->curx[0]].mon<LASTMM)
            survivor=DoCollision(map[Minfo->cury[0]][Minfo->curx[0]].mon,MMnum,0);
          if (map[Minfo->cury[0]][Minfo->curx[0]+1].mon<LASTMM)
            survivor=DoCollision(map[Minfo->cury[0]][Minfo->curx[0]+1].mon,MMnum,1);
          if (map[Minfo->cury[0]+1][Minfo->curx[0]+1].mon<LASTMM)
            survivor=DoCollision(map[Minfo->cury[0]+1][Minfo->curx[0]+1].mon,MMnum,3);
          if (map[Minfo->cury[0]+1][Minfo->curx[0]].mon<LASTMM)
            survivor=DoCollision(map[Minfo->cury[0]+1][Minfo->curx[0]].mon,MMnum,2);
        // Put monster into map array at new location
          if (survivor<LASTMM) map[mi[survivor].cury[0]][mi[survivor].curx[0]].mon=survivor;

          if ((Mon->end>0)&&(Minfo->BirthTime+Mon->end<=clock))
            { anymoved=TRUE; KillMon(MMnum); }

          }
        }
      }
    }
  return(anymoved);
  }


static boolean MonEndPath(int MMnum,int MONnum)
  {
  int l;
  minfo *moni;  

  moni=&mi[MMnum];

  if ((MMnum<0)||(MMnum>=LASTMM))    printf("MMnum OOR! %d\n",MMnum);
  if ((MONnum<0)||(MONnum>=LASTMON)) printf("MONnum OOR! %d\n",MONnum);

  if ((moni->dest==MAXMARCH)||(mon[MONnum].march[moni->CurMarch][moni->dest].pic==0))
    {                         // End of march.
    if ((mon[MONnum].end==-1)&&(moni->CurMarch==NORMALMOVEMENT))
       if (KillMon(MMnum)) return(TRUE);  // Kill monster after path
    moni->dest=0;
    moni->CurMarch=NORMALMOVEMENT;
    }

  moni->Target.Set
     (mon[MONnum].march[moni->CurMarch][moni->dest].x+moni->curx[0],
      mon[MONnum].march[moni->CurMarch][moni->dest].y+moni->cury[0]);

  if (moni->dest!=0)  // Other then the first, the data struct is Deltas, so subtract the previous
    moni->Target -= MakeCoord2d(
      mon[MONnum].march[moni->CurMarch][(moni->dest)-1].x,
      mon[MONnum].march[moni->CurMarch][(moni->dest)-1].y);

  moni->orient=mon[MONnum].march[moni->CurMarch][moni->dest].orient;
  moni->Target.Wrap(MakeCoord2d(MLEN,MWID));
  moni->DistToPlace(moni->Target);

  moni->offl=0;     // Set up the Bresenhams' counters.
  moni->offs=0;
  moni->dest++;
  return(FALSE);    // False means monster not dead.
  }
  

Coord2d minfo::Bresenhams(int speed)
  {
  Coord2d Move(0,0);
  register int l;

  if (abs(Delta.x)>=abs(Delta.y))
    {
    for (l=0; l<speed; l++)
      {
      if (offl >= abs(Delta.x)) { offl=-1; break; }  // At destination.
      offl++;
      Move.x += sign(Delta.x);
      offs   +=  abs(Delta.y);
      if (offs >= abs(Delta.x))
        {
        offs   -= abs(Delta.x);
        Move.y += sign(Delta.y);
        }
      }
    }
  else
    {
    for (l=0;l<speed;l++)
      {
      if ( offl >= abs(Delta.y)) { offl=-1; break; }
      offl++;
      offs   += abs(Delta.x);
      Move.y += sign(Delta.y);
      if (offs >= abs(Delta.y))
        {
        offs   -= abs(Delta.y);
        Move.x += sign(Delta.x);
        }
      }
    }
  return(Move);
  }

void minfo::DistToPlace(Coord2d Spot)  // Calc shortest dist, incl wrap around
  {                                    // sign indicates direction.
  Delta = Spot - MakeCoord2d(curx[0],cury[0]);
  if (Delta.x >  MLEN/2) Delta.x -= MLEN;    // Wrap around handling code.
  if (Delta.x < -MLEN/2) Delta.x += MLEN;
  if (Delta.y >  MWID/2) Delta.y -= MWID;
  if (Delta.y < -MWID/2) Delta.y += MWID;
  Delta*=BLEN;  // Convert block distance to pixel distance.
  }

static boolean KillMon(int MMnum)
  {
  if (mon[mm[MMnum].monnum].end!=-2)  // -2 means never kill monster
    {
    if (mon[mm[MMnum].monnum].newmon==LASTMON)   // Really Kill it.
      {
      mm[MMnum].monnum=LASTMON;
      if (MMnum<FirstEmptyMM) FirstEmptyMM=MMnum-1; // Birth monster optimization
      return(TRUE);
      }
    else  // Change to new Monster
      {
      BirthMon(MMnum,mon[mm[MMnum].monnum].newmon,mi[MMnum].curx[0],mi[MMnum].cury[0]);
      if (mi[MMnum].fromchar>0) mi[MMnum].fromchar++;
      }
    }
  return(FALSE);
  }

static void BlocBirthMon(void)
  {
  register int lp;
  int lx,ly;
  int tmap,MMslot,yinmap,xinmap;

  for (lp=0;lp<BACKBL;lp++) blks[0][lp].Birthtime &= 0xBFFF; // Clear flag

  for (lp=0;lp<MaxBlkMon;lp++)
    {
    tmap=BlkMon[lp].blnum;
    if (BlkMon[lp].nexttime<=clock)                 // Time to birth a mon?
      {
      BlkMon[lp].nexttime+=blks[0][tmap].Birthtime; // set next birthing time
      blks[0][tmap].Birthtime |= 0x4000;            // Set flag to birth monster
      }
    }
  for (lx=mx-(SCRNBLEN)/2;lx<mx+(SCRNBLEN)*3/2;lx++)
    {
    for (ly=my-(SCRNBWID)/2;ly<my+(SCRNBWID)*3/2;ly++)
      {
      xinmap=(lx+MLEN)%MLEN;
      yinmap=(ly+MWID)%MWID;
      tmap=blkmap[map[yinmap][xinmap].blk];
      if (blks[0][tmap].Birthtime & 0x4000)         // If flag bit set
        {
        if ((MMslot=BirthMon(LASTMM,blks[0][tmap].Birthmon,xinmap,yinmap)) < LASTMM)
          {
          mi[MMslot].fromchar =         0;          // Birth monster
          mi[MMslot].Reset();
          }
        }
      }
    }
  }

void minfo::Reset(void)
  {
  Delta.Set(0,0);
  ScrPos.Set(OFFSCRN,OFFSCRN);
  OldPos   = ScrPos;
  PicDrawn = FALSE;
  curx[1]  = 0;
  cury[1]  = 0;
  CurPic   = -1;
  offl     = -1;
  offs     = 0;
  CurMarch = NORMALMOVEMENT;
  orient   = 0;
  dest     = 0;  // Index to Next Pos & pic of Monster
  }

static int BirthMon(int MMnum,char mon,int x,int y)
  {
  int t;
  if (MMnum>=LASTMM)
    {
    t=FirstEmptyMM;
    do
      {
      if (mm[t].monnum==LASTMON) { MMnum=t; break; }
      t++;
      if (t>=LASTMM) t=0;
      } while (t!=FirstEmptyMM);
    FirstEmptyMM=t;
    }

  if ((MMnum<LASTMM)&&(MMnum>=0))          // Empty Monster Slot found
    {
    mm[MMnum].monnum   =           mon;
    mi[MMnum].BirthTime=         clock;
    mi[MMnum].NextAni  =         clock;
    mi[MMnum].NextMove =         clock;
    mi[MMnum].curx[0]  =             x;
    mi[MMnum].cury[0]  =             y;
    mi[MMnum].Target.Set(x,y);
    }
  else MMnum=LASTMM;
  return(MMnum);
  }

static void CharBirthMon(int x,int y,int mon)
  {
  int MMslot;
  minfo *moni;

  if ( (MMslot=BirthMon(LASTMM,mon,x,y)) >= LASTMM) return; // No empty Monster Slots */
  moni=&mi[MMslot];

  moni->Reset();
  moni->fromchar = 1;
  moni->curx[1]  = chr.x[1];
  moni->cury[1]  = chr.y[1];
  moni->offl     = 0;
  moni->Delta.Set(ci.netx*BLEN,ci.nety*BLEN);  // Make monster go away from char

  if (abs(ci.netx)-abs(ci.nety)>0)    // Set orientation so goes away from char
    {
    if (ci.netx<0) moni->orient = 4;
    else           moni->orient = 0;
    }
  else 
    {
    if (ci.nety<0) moni->orient = 3;
    else           moni->orient = 1;
    }
  }

static void chkmapbounds(void) // Check to see is outside of map boundaries
  {
  if (chr.x[0]>=MLEN)      chr.x[0]-=MLEN;
  if (chr.x[0]<0)          chr.x[0]+=MLEN;
  if (chr.y[0]>=MWID)      chr.y[0]-=MWID;
  if (chr.y[0]<0)          chr.y[0]+=MWID;
  }

   // Makes sure char stays within screen bounds
static void chkscrnbounds(int *tx, int *ty)
  {
  int cx,cy;
  int tmx,tmy;
  int temp[2];


  cx=mx;
  cy=my;

  temp[0]=cx-chr.x[0];  // Straight Dist from top left to char
  if (temp[0]>0) temp[1]=-1*((MLEN-cx)+chr.x[0]); // Dist. other way
  else temp[1]= (MLEN-chr.x[0])+cx;
  if (abs(temp[1])>abs(temp[0])) tmx=temp[0];
  else tmx=temp[1];
    
  temp[0]=cy-chr.y[0];  // Straight dist
  if (temp[0]>0) temp[1]=-1*((MWID-cy)+chr.y[0]); // Dist wrapped around/
    else temp[1]= (MWID-chr.y[0])+cy;
  if (abs(temp[1])>abs(temp[0])) tmy=temp[0];
  else tmy=temp[1];

  tmx*=-1;
  tmy*=-1;

  if (*tx<0)
    {
    if (tmx<2) if (chr.x[1]<=abs(*tx)) *tx=-1*(chr.x[1]);
    if (tmx<1) *tx=0;
    }

  if ((tmx>SCRNBLEN-2)&&(*tx>0)) *tx=0;

  if (*ty<0)
    {
    if (tmy<2) if (chr.y[1]<=abs(*ty)) *ty=-1*(chr.y[1]);
    if (tmy<1) *ty=0;
    }

  if ((tmy>SCRNBWID-2)&&(*ty>0)) *ty=0;


/*  if ((*tx<0)&&(ci.scrx+(*tx)<=BLEN))            *tx=0;
  if ((*tx>0)&&(ci.scrx+(*tx)>=SIZEX-BLEN-BLEN)) *tx=0;
  if ((*ty<0)&&(ci.scry+(*ty)<=BLEN))            *ty=0;
  if ((*ty>0)&&(ci.scry+(*ty)>=SIZEX-BLEN-BLEN)) *ty=0;
*/
  }

static unsigned char DoBlockTouchChar(blkstruct *blk)
  {
  unsigned char Snded=FALSE;
  if ( (blk->effect >= 20) || (blk->effect == 3))  // Don't let a touch happen
    if (ci.meter[blk->effect]+blk->eamt <= 0)      // if you don't have enough
      return(FALSE);                               // in a counter.

  if (chr.cseq!=DIESEQ)  // only execute this code once per death
    {
    if (blk->chhitpt<=0) ci.hitpts+=blk->chhitpt;
    else
      {
      if ( ((unsigned int) (((unsigned int)ci.hitpts)+blk->chhitpt)) <INF_REPEAT)
        ci.hitpts+=blk->chhitpt;    // Add or Sub hitpoints
      else ci.hitpts=INF_REPEAT-1;
      }
    if (blk->chhitpt>0) Snded=DoSound(HELPSOUND);
    else if ((blk->chhitpt<0)&&(chr.cseq!=HURTSEQ))  // Do injure seq.
      {
      PushKey();
      ci.NextAni=clock;
      chr.cseq=HURTSEQ;
      chr.cframe=-1;
      curkey=0xFF;
      if (chr.sq[HURTSEQ].sound<LASTSND) Snded=DoSound(chr.sq[HURTSEQ].sound);
      }
    if (ci.hitpts<=0)
      {
      ci.NextAni=clock;
      chr.cseq=DIESEQ;         // Set up to do death sequence
      chr.cframe=-1;
      curkey=0;
      }
    }

  if (blk->effect==1)
    {
    if (blk->eamt>0) Snded=DoSound(ADDLIFESOUND);
    ci.lives+=blk->eamt;
    }

  if ((blk->effect>2)&&(ci.meter[blk->effect]!=INF_REPEAT))
    {
    if (!Snded)
      {
      if (blk->effect == 3)
        {
        if (blk->eamt>0)      Snded=DoSound(ADDMONEYSOUND);
        else if (blk->eamt<0) Snded=DoSound(SUBMONEYSOUND);
        }
      else
        {
        if (blk->eamt>0)      Snded=DoSound(ADDOTHERSOUND);
        else if (blk->eamt<0) Snded=DoSound(SUBOTHERSOUND);
        }
      }
    if (((unsigned int) (((unsigned int) ci.meter[blk->effect])+blk->eamt))<INF_REPEAT)  // Stop any wrap-around
      {
      if (ci.meter[blk->effect]+blk->eamt < -1*(INF_REPEAT-128))
        ci.meter[blk->effect] = -1*(INF_REPEAT-128);
      else ci.meter[blk->effect]+=blk->eamt;
      }
    else ci.meter[blk->effect]=INF_REPEAT-1;
    } 

  ci.score+=blk->scorepts;      // Add or sub score
  if (!Snded)
    {
    if (blk->scorepts>0)      Snded=DoSound(ADDSCORESOUND);
    else if (blk->scorepts<0) Snded=DoSound(SUBSCORESOUND);
    }
  Snded++;
  return(Snded);
  }

static void CheckTouch(void)
  {
  register int l;
  touchblk t;

  GetTouchbl(&t);

  for (l=0;l<6;l++)
    {
    if (!blks[0][t.blknum[l]].obj)     /* if not an object */
      really_checktouch(t.blks[l][0],t.blks[l][1],t.blknum[l]);
    }      /* either true or false -- A two is returned for use by pickup */
  }
 
static char really_checktouch(int x,int y,unsigned char bnum)
  {
  Coord2d Tmp;

  if (x != MLEN)   // Touching this block
    {
    if (!DoBlockTouchChar(&blks[0][bnum])) return(2);
    else
      {
      if (bnum != blks[0][bnum].touchbl)
        {
        if (onscrn(x,y,&Tmp))
          {
          map[y][x].blk=blks[0][bnum].touchbl;          // Update in mem
          BufDrawBlk(Tmp.x,Tmp.y,(char*)blks[0][blkmap[blks[0][bnum].touchbl]].p);
          AddDirtyRect(Tmp.x,Tmp.y,Tmp.x+BLEN-1,Tmp.y+BLEN-1);
          }
        }
      }
    }
  return(TRUE);
  }

static int chksolids(int dx,int dy,touchblk *t)
  {
  unsigned int retstatus=0;

  if (dy>0) retstatus = chkdowns(t);
  if (dy<0) retstatus = chkups(t);

  if (dx>0) retstatus |= chkrights(t);
  if (dx<0) retstatus |= chklefts(t);

  return(retstatus);
  } 

static unsigned int chklefts(touchblk *t)
  {
  unsigned int retval=FALSE;
  unsigned char spots[4]={0,0,0,0};
  int twohigh=2;

  if ((t->blks[0][0]) !=MLEN) twohigh=0;  // Two high character

  if (t->blknum[twohigh+1] != BACKBL)
    spots[1] |= blks[0][t->blknum[twohigh+1]].solid&SOLBOT;
  spots[0] |= blks[0][t->blknum[twohigh]].solid&SOLRIG;
  if (twohigh==0)  // Actually, this means that the character IS two high!
    {              // Stop char from going left when only one block stops him
    spots[0] |= blks[0][t->blknum[2]].solid&SOLRIG;
    }
  if (t->blknum[4] !=BACKBL)
    {
    if (t->blknum[5] != BACKBL) spots[3] |= blks[0][t->blknum[5]].solid&SOLTOP;
    if (!(spots[3]&SOLTOP)) spots[2] |= blks[0][t->blknum[4]].solid&SOLRIG;
    }

  if ((spots[0]&SOLRIG)&&(!(spots[2]&SOLRIG)))  // Code that moves him into 1
    if (chr.y[1]>((BLEN*2)/4)) chr.y[1]=BLEN;
    
  if ((!(spots[0]&SOLRIG))&&(spots[2]&SOLRIG))  // Code that moves him into 1
    if (chr.y[1]<(BLEN/2)) chr.y[1]=0;
    
  if ((spots[0]&SOLRIG)||(spots[2]&SOLRIG)) retval |=SOLLEF;
  return(retval);
  }

static unsigned int chkrights(touchblk *t)
  {
  unsigned int retval=FALSE;
  unsigned char spots[4]={0,0,0,0};
  int twohigh=2;

  if ((t->blks[0][0]) !=MLEN) twohigh=0;

  if (t->blknum[twohigh+1] !=BACKBL)
    {
    if (t->blknum[twohigh] !=BACKBL) spots[0] |= blks[0][t->blknum[twohigh]].solid&SOLBOT;
    if (!(spots[0]&SOLBOT)) spots[1] |= (blks[0][t->blknum[twohigh+1]].solid&SOLLEF);
    }
  if (twohigh==0)  // Actually, this means that the character IS two high!
    {              // Stop char from going left when only one block stops him
    spots[1] |= blks[0][t->blknum[3]].solid&SOLLEF;
    }

  if (t->blknum[5] !=BACKBL)
    {
    if (t->blknum[4] !=BACKBL) spots[2] |= blks[0][t->blknum[4]].solid&SOLTOP;
    if (!(spots[2]&SOLTOP)) spots[3] |= (blks[0][t->blknum[5]].solid&SOLLEF);
    }

  if ((spots[1]&SOLLEF)&&(!(spots[3]&SOLLEF)))  // Code that moves him into 1
    if (chr.y[1]>((BLEN*2)/4)) chr.y[1]=BLEN;
   
  if ((!(spots[1]&SOLLEF))&&(spots[3]&SOLLEF))  // Code that moves him into 1
    if (chr.y[1]<(BLEN/2)) chr.y[1]=0;
    
  if ((spots[1]&SOLLEF)|(spots[3]&SOLLEF)) retval |=SOLRIG;
  return(retval);
  }


static unsigned int chkups(touchblk *t)
  {
  unsigned int retval=FALSE;
  unsigned char spots[4]={0,0,0,0};
  int twohigh=2;

  if ((t->blks[0][0]) !=MLEN) twohigh=0;
  
  if (t->blknum[twohigh] !=BACKBL)
    {
    if (t->blknum[4] !=BACKBL) spots[2] |= blks[0][t->blknum[4]].solid&SOLRIG;
    if (!(spots[2]&SOLRIG)) spots[0] |= (blks[0][t->blknum[twohigh]].solid&SOLBOT);
    }
  if (t->blknum[twohigh+1] !=BACKBL)
    {
    if (t->blknum[5] !=BACKBL) spots[3] |= blks[0][t->blknum[5]].solid&SOLLEF;
    if (!(spots[3]&SOLLEF)) spots[1] |= (blks[0][t->blknum[twohigh+1]].solid&SOLBOT);
    }

  if ((spots[0]&SOLBOT)&&(!(spots[1]&SOLBOT)))  // Code that moves him into 1
    if (chr.x[1]>((BLEN*2)/4)) chr.x[1]=BLEN;
    
  if ((!(spots[0]&SOLBOT))&&(spots[1]&SOLBOT))  // Code that moves him into 1
    if (chr.x[1]<(BLEN/2)) chr.x[1]=0;
   
  if ((spots[0]&SOLBOT)|(spots[1]&SOLBOT)) retval |=SOLTOP;
  return(retval);
  }


static unsigned int chkdowns(touchblk *t)
  {
  unsigned int retval=FALSE;
  unsigned char spots[4]={0,0,0,0};
  int twohigh=2;

  if ((t->blks[0][0]) !=MLEN) twohigh=0; 

  if (t->blknum[4] !=BACKBL)  // Don't do if not even touching
    { 
    spots[2] |= blks[0][t->blknum[4]].solid&SOLRIG; 
    if (t->blknum[twohigh] != BACKBL) spots[0] |= blks[0][t->blknum[twohigh]].solid&SOLRIG;
    if (!((spots[2]&SOLRIG)&&(spots[0]&SOLRIG))) 
      spots[2] |= (blks[0][t->blknum[4]].solid&SOLTOP);
    }

  if (t->blknum[5] !=BACKBL)
    {
    if (t->blknum[twohigh+1] !=BACKBL) spots[1] |= blks[0][t->blknum[twohigh+1]].solid&SOLLEF;
    spots[3] |= blks[0][t->blknum[5]].solid&SOLLEF;
    if (!((spots[1]&SOLLEF)&(spots[3]&SOLLEF)))
      spots[3] |= (blks[0][t->blknum[5]].solid&SOLTOP);
    }

  if ((spots[2]&SOLTOP)&&(!(spots[3]&SOLTOP)))  // Code that moves him into 1
    {                                           // wide corridors
    if (chr.x[1]>((BLEN*2)/4)) chr.x[1]=BLEN;
    }
  if ((!(spots[2]&SOLTOP))&&(spots[3]&SOLTOP))  // Code that moves him into 1
    {                                           // wide corridors
    if (chr.x[1]<(BLEN/2)) chr.x[1]=0;
    }
    
  if ((spots[2]&SOLTOP)|(spots[3]&SOLTOP)) retval |=SOLBOT;
  return(retval);
  }

static void OnGround(int Solid)
  {
  int gravdir=0;
  int tempx=0;
  int tempy=0;
  
  tempy=chr.y[0];                 //   Use as grav the block the char is
  if (chr.y[1]>BLEN/2) tempy+=1;  // mostly on.
  tempx=chr.x[0];
  if (chr.x[1]>BLEN/2) tempx+=1;
  if (tempy>=MLEN) tempy-=MLEN;
  if (tempx>=MLEN) tempx-=MLEN;

  gravdir=blks[0][blkmap[map[tempy][tempx].blk]].grav;

  //    Only stop gravity if there is a solid block opposing gravity's
  //    direction.
  if (((monsolid&SOLBOT)||(Solid&SOLBOT))&&(ci.gravy>0))   // Gravity Down
    ci.gravy=0;
  if (((monsolid&SOLTOP)||(Solid&SOLTOP))&&(ci.gravy<0))   // Gravity Up
    ci.gravy=0;
  if (((monsolid&SOLRIG)||(Solid&SOLRIG))&&(ci.gravx>0))   // Gravity Right
    ci.gravx=0;
  if (((monsolid&SOLLEF)||(Solid&SOLLEF))&&(ci.gravx<0))   // Suck Left
    ci.gravx=0;
  
  // Slow down grav momentum if no more gravity in that dir
  if (!CHRSEQ.Momentum)
    {
    if (!((gravdir&SOLBOT)||(gravdir&SOLTOP))) { ci.gravy*=5; ci.gravy/=6; }
    if (!((gravdir&SOLLEF)||(gravdir&SOLRIG))) { ci.gravx*=5; ci.gravx/=6; }
    }
  }

static int MoveCharGrav(int fn)
  {
  static unsigned long int gclk=1;
  int retval=FALSE;
  int tempx=0;
  int tempy=0;
  
  if (fn==1) { gclk=clock; return(TRUE); }

  if (clock>=gclk+GRAVSPEED)
    {
    gclk=clock;

    tempy=chr.y[0];                 //   Use as grav the block the char is
    if (chr.y[1]>BLEN/2) tempy+=1;  // mostly on.
    tempx=chr.x[0];
    if (chr.x[1]>BLEN/2) tempx+=1;
    if (tempy>=MWID) tempy-=MWID;
    if (tempx>=MLEN) tempx-=MLEN;


    if (((blks[0][blkmap[map[tempy][tempx].blk]].grav)&4)==4)   
      ci.gravy+=1;
    if (((blks[0][blkmap[map[tempy][tempx].blk]].grav)&1)==1)   
      ci.gravy-=1;
    if (((blks[0][blkmap[map[tempy][tempx].blk]].grav)&2)==2) 
      ci.gravx+=1;
    if (((blks[0][blkmap[map[tempy][tempx].blk]].grav)&8)==8) 
      ci.gravx-=1;

    if ((ci.gravx)||(ci.gravy)) retval=TRUE; /* Since gravity still has an effect, redraw */
     
    if (ci.gravx>MAXGRAV*4)      ci.gravx=MAXGRAV*4;
    if (ci.gravx<(-1*MAXGRAV*4)) ci.gravx= -1*MAXGRAV*4;
    if (ci.gravy>MAXGRAV*4)      ci.gravy=MAXGRAV*4;
    if (ci.gravy<(-1*MAXGRAV*4)) ci.gravy= -1*MAXGRAV*4;
    }

  return(retval);
  }

static void GetTouchbl(touchblk *t)
  {
  register int l;

  for (l=0;l<6;l++)         /* Initalize tblks array */
    { 
    t->blks[l][0]=MLEN;     /* Put on extra lines just for Pete */
    t->blks[l][1]=MWID;
    t->blknum[l]=BACKBL;
    } 

  if (ci.topthere)  /* Top block does exist */
    {
    int temp=0;

    if (chr.y[0]-1<0) temp=chr.y[0]-1+MWID; /* Wrap around boundary */
    else temp=chr.y[0]-1;

    t->blks[0][0] = chr.x[0]; t->blks[0][1] = temp;  /* Top left */
    t->blknum[0] = blkmap[map[t->blks[0][1] ][t->blks[0][0] ].blk];

    if (chr.x[1]>0)
      { 
      t->blks[1][0]=(chr.x[0]+1)%MLEN; t->blks[1][1]=temp;  /* Top Right */
      t->blknum[1] = blkmap[map[t->blks[1][1]][t->blks[1][0]].blk];
      } 
    }

  t->blks[2][0] = chr.x[0];   t->blks[2][1] = chr.y[0];  /* Mid Left */
  t->blknum[2] = blkmap[ map[ t->blks[2][1] ][ t->blks[2][0] ].blk ];

  if (chr.x[1]>0)                                      /* Mid Right */
    { 
    t->blks[3][0]=(chr.x[0]+1)%MLEN; t->blks[3][1]=chr.y[0];
    t->blknum[3] = blkmap[  map[ t->blks[3][1] ][ t->blks[3][0] ].blk  ];
    } 
  if (chr.y[1]>0)                                      /* Bottom left */
    { 
    t->blks[4][0]=chr.x[0]; t->blks[4][1]=(chr.y[0]+1)%MWID;
    t->blknum[4] = blkmap[  map[ t->blks[4][1] ][ t->blks[4][0] ].blk  ];
    } 
  if ((chr.x[1]>0)&&(chr.y[1]>0))                    /* Bottom Right */
    { 
    t->blks[5][0]=(chr.x[0]+1)%MLEN; t->blks[5][1]=(chr.y[0]+1)%MWID;
    t->blknum[5] = blkmap[  map[ t->blks[5][1] ][ t->blks[5][0] ].blk  ];
    } 
  }

static boolean onscrn(int x,int y,Coord2d *Ret)
  {
  if (mx<=x) Ret->x=(x-mx)*BLEN;
  else       Ret->x=(MLEN+(x-mx))*BLEN;
  Ret->x-=scroll.nx;
 
  if (my<=y) Ret->y=(y-my)*BLEN;
  else       Ret->y=(MWID+(y-my))*BLEN;
  Ret->y-=scroll.ny;

  return (Ret->In(MakeCoord2d(0,0),MakeCoord2d(SIZEX-BLEN,SIZEY-BLEN)));
  }


/**********************************************************/
/*      Function to check if time to change the scene     */
/**********************************************************/
static int chklnk(int cursc)
  {
  register int tloop,loo;
  touchblk t;

  GetTouchbl(&t);

  for (loo=0;loo<MAXLINK;loo++)
    {
    for (tloop=0;tloop<6;tloop++)
      {
      if ((scns[cursc].links[loo].tnum!=MAXSCENES)
        &&(scns[cursc].links[loo].fx==t.blks[tloop][0])
        &&(scns[cursc].links[loo].fy==t.blks[tloop][1]))
        {  
        chr.x[0]=scns[cursc].links[loo].tx;
        chr.y[0]=scns[cursc].links[loo].ty;
        return (loo);
        }
      }
    }
  return (MAXLINK);
  }

static void Sdrawspbloc(int x,int y,char dir,blkstruct *b)
  {
  register int j,k;

  AddDirtyRect(x,y,x+BLEN-1,y+BLEN-1);
  BufDrawSpBlk(x,y,dir,(char*)b->p);

/*
  for (j=0;j<BLEN;j++)
    for (k=0;k<BLEN;k++)
      switch(dir) 
        {
        case 1:       
          if (b->p[BLEN-j-1][k]!=255)
            BufPoint (x+j,y+k,b->p[BLEN-j-1][k]);
          break;
        case 2: 
          if (b->p[BLEN-k-1][BLEN-j-1]!=255)
            BufPoint (x+j,y+k,b->p[BLEN-k-1][BLEN-j-1]);
          break;
        case 3:
          if (b->p[j][BLEN-k-1]!=255)
            BufPoint (x+j,y+k,b->p[j][BLEN-k-1]);
          break;
        case 4: 
          if (b->p[k][BLEN-j-1]!=255)
            BufPoint (x+j,y+k,b->p[k][BLEN-j-1]);
          break;
        case 5: 
          if (b->p[j][k]!=255)
            BufPoint (x+j,y+k,b->p[j][k]);
          break;
        case 6: 
          if (b->p[BLEN-k-1][j]!=255)
            BufPoint (x+j,y+k,b->p[BLEN-k-1][j]);
          break;
        case 7: 
          if (b->p[BLEN-j-1][BLEN-k-1]!=255)
            BufPoint (x+j,y+k,b->p[BLEN-j-1][BLEN-k-1]);
          break;
        case 0:
        default:
          if (b->p[k][j]!=255)
            BufPoint (x+j,y+k,b->p[k][j]);
          break;
        }
*/
  }

static int ChangeScroll(void)
  {
  int l;
  int tmx,tmy;

  scroll.Totalx=scroll.Totaly=0;
  tmx=(ci.nextx+(BLEN/2))-(SIZEX/2);
  tmy=(ci.nexty+(BLEN/2))-(SIZEY/2);

  scroll.distx=tmx;
  scroll.disty=tmy;

  if ((scroll.left==0xFF)&&(tmx>scroll.MaxX))         // Scroll map Left
    scroll.cont|=SOLLEF;
  if ((scroll.right==0xFF)&&(tmx<scroll.MinX))        // Scroll map right
    scroll.cont|=SOLRIG;

  if ((scroll.up==0xFF)&&(tmy>scroll.MaxY))           // Scroll map up
    scroll.cont|=SOLTOP;
  if ((scroll.down==0xFF)&&(tmy<scroll.MinY))         // Scroll map down
    scroll.cont|=SOLBOT;

  if (scroll.cont&SOLLEF)
    {
    if (tmx<scroll.LeftStop) scroll.cont &= ~SOLLEF;
    else scroll.Totalx= ((tmx-scroll.LeftStop)*SCRSPDX)/(scroll.MaxX-scroll.LeftStop);
    //                ^this formula is distributes the speed 1-SCRSPDX across
    //                the Screen distance (scroll.LeftStop -> scroll.MaxX)
    //                linearly. (ie it is really (X*MAXSPEED)/MAXX)
    }
  if (scroll.cont&SOLRIG)
    {
    if (tmx>scroll.RightStop) scroll.cont &= ~SOLRIG;
    else scroll.Totalx=-((-tmx+scroll.RightStop)*SCRSPDX)/(scroll.RightStop-scroll.MinX);
    }
  if (scroll.cont&SOLTOP)
    {
    if (tmy<scroll.UpStop) scroll.cont &= ~SOLTOP;
    else scroll.Totaly= ((tmy-scroll.UpStop)*SCRSPDY)/(scroll.MaxY-scroll.UpStop);
    }
  if (scroll.cont&SOLBOT)
    {
    if (tmy>scroll.DownStop) scroll.cont &= ~SOLBOT;
    else scroll.Totaly=- ((-tmy+scroll.DownStop)*SCRSPDY)/(scroll.DownStop-scroll.MinY);
    }

  if (scroll.Totaly>28)  scroll.Totaly=28;
  if (scroll.Totaly<-28) scroll.Totaly=-28;
  if (scroll.Totalx>28)  scroll.Totalx=28;
  if (scroll.Totalx<-28) scroll.Totalx=-28;

  scroll.Movx.DScroll=scroll.Totalx;
  scroll.Movx.Ctr=0;
  scroll.Movx.Iters=SCROLLPERMOVE;

  scroll.Movy.DScroll=scroll.Totaly;
  scroll.Movy.Ctr=0;
  scroll.Movy.Iters=SCROLLPERMOVE;

  return(scroll.cont);
  }

static void DrawScroll(int speed)
  {
  union
    {
    unsigned char c[4];
    unsigned long int li;
    unsigned int i[2];
    } wb;  

  unsigned char dir=0;
  int temp;
  Coord2d Shift(0,0);
  int oldmx,oldmy, oldnx,oldny;
  boolean MoveChar=FALSE;

  oldmx=mx;
  oldmy=my;
  oldnx=scroll.nx;
  oldny=scroll.ny;

  temp=BresenhamScroll(&scroll.Movx,1);  //speed);
  scroll.movx+=temp;
  temp=BresenhamScroll(&scroll.Movy,1);  //speed);
  scroll.movy+=temp;

  if (scroll.movx<0)        // MAP GOES -> (right)
    {
    while (scroll.movx<0)
      {
      scroll.movx+=4;
      scroll.nx-=4; scroll.zero--; Shift.x+=4;
      dir=SOLRIG;
      }
    }
  else if (scroll.movx>0)   // MAP GOES <- (left)
    {
    while (scroll.movx>3)
      {
      Shift.x+= -4; scroll.movx-=4; scroll.nx+=4; scroll.zero++;
      dir|=SOLLEF;
      }
    }
  if (scroll.movy<0)        // MAP GOES v (down)
    {
    while (scroll.movy<0)
      {
      Shift.y+=4; scroll.movy+=4; scroll.ny-=4; scroll.zero-=SIZEX;
      dir|=SOLBOT;
      }
    }
  else if (scroll.movy>0)   // MAP GOES ^ (UP)
     {
    while (scroll.movy>3)
      {
      Shift.y+= -4; scroll.movy-=4; scroll.ny+=4; scroll.zero+=SIZEX;
      dir|=SOLTOP;
      }
    }

  scroll.LittleShiftx=scroll.movx&3;
  scroll.LittleShifty=scroll.movy&3;
    
  if (scroll.nx>=BLEN) { scroll.nx-=BLEN; mx++; if (mx>=MLEN) mx-=MLEN; }
  if (scroll.ny>=BLEN) { scroll.ny-=BLEN; my++; if (my>=MWID) my-=MWID; }
  if (scroll.nx<0)     { scroll.nx+=BLEN; mx--; if (mx<0)     mx+=MLEN; }
  if (scroll.ny<0)     { scroll.ny+=BLEN; my--; if (my<0)     my+=MWID; }
  
  wb.li=scroll.zero;     // Calculate zeroaddon before actually moving the
  wb.li<<=2;             // screen, so we can draw to the screen as if it
  zeroaddon=wb.i[0];     // was in the new location.
  zeropage= (wb.c[2]&0x0F);
  wb.li>>=2;

  if (Shift.x|Shift.y) MoveChar=ShiftEverything(Shift);

  if (dir&SOLTOP)
    {
    int l,tempny,tempmy;
    for (l=4,tempny=oldny,tempmy=oldmy;l<=-Shift.y;l+=4)
      {
      tempny+=4;
      if (tempny>=BLEN) { tempny-=BLEN; tempmy++; if (tempmy>=MLEN) tempmy-=MLEN; }
      drawhorizside(zeroaddon+64000+(SIZEX*(Shift.y+l-4)),mx,(tempmy+10)%MLEN,scroll.nx,tempny-4);
      }
    }

  if (dir&SOLBOT)
    {
    int l,tempny,tempmy;
    for (l=4,tempny=oldny,tempmy=oldmy;l<=Shift.y;l+=4)
      {
      tempny-=4;
      if (tempny<0) { tempny+=BLEN; tempmy--; if (tempmy<0) tempmy+=MLEN; }
      drawhorizside(zeroaddon+(SIZEX*(Shift.y-l)),mx,tempmy,scroll.nx,tempny);
      }
    }

  if (dir&SOLLEF)
    {
    int l,tempnx,tempmx;
    for (l=4,tempnx=oldnx,tempmx=oldmx;l<=-Shift.x;l+=4)
      {
      tempnx+=4;
      if (tempnx>=BLEN) { tempnx-=BLEN; tempmx++; if (tempmx>=MLEN) tempmx-=MLEN; }
      drawvertside(zeroaddon+SIZEX+Shift.x+l-4,(tempmx+16)%MLEN,my,tempnx-4,scroll.ny);
      }
    DrawDirtyRects();
    }
  if (dir&SOLRIG)
    {
    int l,tempnx,tempmx;
    for (l=4,tempnx=oldnx,tempmx=oldmx;l<=Shift.x;l+=4)
      {
      tempnx-=4;
      if (tempnx<0)     { tempnx+=BLEN; tempmx--; if (tempmx<0) tempmx+=MLEN; }
      drawvertside(zeroaddon+Shift.x-l,tempmx,my,tempnx,scroll.ny);
      }
    }

  if (dir&SOLRIG) SvgaBufScrnBox(0,0,Shift.x,SIZEY);
  if (dir&SOLTOP) SvgaBufToScrn(SIZEX*(-Shift.y),zeroaddon+64000+(SIZEX*Shift.y));
  if (dir&SOLBOT) SvgaBufToScrn(SIZEX*Shift.y,zeroaddon);

  MoveViewScreen(scroll.zero+(80*scroll.LittleShifty),scroll.LittleShiftx);   // Actual VGA scroll (updates dirty ptr too)
  if ((MoveChar)||(scroll.LittleShiftx)||(scroll.LittleShifty))
    {  // Scroll.LittleShiftx is a absolute location, not whether it has moved in x or not!
    erasechars();
    drawchars();
    DrawDirtyRects();
    }
  // This is below so that the scrolling does not show on the wrong side.
  if (dir&SOLLEF) SvgaBufScrnBox(319+Shift.x,0,319,SIZEY);
  }

static void drawvertside(unsigned int offset,int mx,int my,int nx,int ny)
  {        
  register int ly;

  if (nx<0) { nx+=BLEN; mx--; if (mx<0) mx+=MLEN; }
  for (ly=0;ly<SIZEY;ly+=4)
    {
    BufDraw4x4Addr(offset+(ly*SIZEX),&blks[0][blkmap[map[my][mx].blk]].p[ny][nx]);
    ny+=4;
    if (ny>=BLEN) { ny-=BLEN; my++; if (my>=MWID) my-=MWID; } 
    }
  }

static void drawhorizside(unsigned int offset,int mx,int my,int nx,int ny)
  {
  register int lx;

  if (ny<0) { ny+=BLEN; my--; if (my<0) my+=MWID; }

  for (lx=0;lx<SIZEX;lx+=4)
    {
    BufDraw4x4Addr(offset+lx,&blks[0][blkmap[map[my][mx].blk]].p[ny][nx]);
    nx+=4;
    if (nx>=BLEN) { nx-=BLEN; mx++; if (mx>=MLEN) mx-=MLEN; }
    }
  }
  
static int BresenhamScroll(BresScroll *b,int speed)
  {
  int l,Amt=0;

   for (l=0;l<speed;l++)
    {
    if (abs(b->DScroll)>b->Iters)
      {
      while (b->Ctr<abs(b->DScroll))
        {
        Amt+=sign(b->DScroll);
        b->Ctr+=b->Iters;
        }
      b->Ctr -= abs(b->DScroll);
      }
    else
      {
      b->Ctr += abs(b->DScroll);
      if (b->Ctr>=b->Iters)
        {
        b->Ctr-=b->Iters;
        Amt+=sign(b->DScroll);
        }
      }
    }
  return (Amt);
  }


static boolean ShiftEverything(Coord2d Shift)
  {
  register int l;
  boolean retval=FALSE;
  Coord2d LowBnd(BLEN/2,BLEN/2), UpBnd(SIZEX-BLEN,SIZEY-BLEN);
  minfo *moni;

  moni=&mi[0];
  for (l=0;l<LASTMM;l++,moni++)
    {
    if (mm[l].monnum<LASTMON)
      {
      if (moni->ScrPos.x!=OFFSCRN) moni->ScrPos+=Shift;
      if (moni->OldPos.x!=OFFSCRN)
        {
//        if ((mi[l].oldx+shiftx<BLEN/2)||(mi[l].oldx+shiftx>SIZEX-BLEN)   // Off scrn now so
//         || (mi[l].oldy+shifty<BLEN/2)||(mi[l].oldy+shifty>SIZEY-BLEN))  // erase old monster.
        moni->OldPos+=Shift;
        if (!moni->OldPos.In(LowBnd,UpBnd)) moni->Erase();
        }
      }
    }

  if ((ci.nexty-ci.oldy==0)||(sign(Shift.y)==sign(ci.nexty-ci.oldy)))
    {
    if (ci.scry!=OFFSCRN) ci.scry+=Shift.y;  // don't move rel to background.
    }
  else
    {
    retval=TRUE;
    if (abs(ci.nexty-ci.oldy)<4) ci.scry+= (ci.nexty-ci.oldy)+Shift.y;
    }
  if ((ci.nextx-ci.oldx==0)||(sign(Shift.x)==sign(ci.nextx-ci.oldx)))
    {
    if (ci.scrx!=OFFSCRN) ci.scrx+=Shift.x;
    }
  else
    {
    retval=TRUE;
    if (abs(ci.nextx-ci.oldx)<4) ci.scrx+= (ci.nextx-ci.oldx)+Shift.x;
    }

  if (ci.nextx!=OFFSCRN) ci.nextx+=Shift.x;
  if (ci.nexty!=OFFSCRN) ci.nexty+=Shift.y;
  if (ci.oldx!=OFFSCRN) ci.oldx+=Shift.x;
  if (ci.oldy!=OFFSCRN) ci.oldy+=Shift.y;
  return(retval);
  }

static void InitBirthArray(void)
  {
  unsigned register int lx,ly=0;
 
  MaxBlkMon=0;
  for (lx=0;lx<BACKBL;lx++)
    if (blks[0][lx].Birthtime>0) MaxBlkMon++;
  if (MaxBlkMon!=0)
    {
    if ((BlkMon=(BlkMonStruct *) farmalloc(sizeof(BlkMonStruct)*MaxBlkMon))==NULL)
      {
      errorbox("NOT ENOUGH MEMORY!","  (Q)uit");
      exit(0);
      }
    for (lx=0;lx<BACKBL;lx++)
      if (blks[0][lx].Birthtime>0)
        {
        BlkMon[ly].blnum=lx;
        ly++;
        }
    }
  }

static int loadscene(int scnum)
  {
  char out[MAXFILENAMELEN];
  int result=0,loop=0,cury=0;
  char exts[][5] = { ".pal",".map",".bbl",".mon",".mbl",".chr",".cbl",".snd",".cmf"};
  char fpaths[][5]={ "pal\\","map\\","blk\\","mon\\","blk\\","chr\\","blk\\","snd\\","snd\\"};
 
  NoMons=FALSE;

  if (BlkMon!=NULL) { farfree(BlkMon); BlkMon=NULL; }
  if (scns[scnum].fnames[0][0]!=0) result = !loadspecany(scns[scnum].fnames[0],".pal","pal\\",sizeof(RGBdata)*256,(char *) &colors);
  if (scns[scnum].fnames[1][0]!=0) result|= (!loadspecmap(scns[scnum].fnames[1],".map","map\\"))<<1;
  if (scns[scnum].fnames[2][0]!=0) result|= (!loadspecblk(scns[scnum].fnames[2],".bbl","blk\\",BACKBL,blks[0]))<<2;
  if (scns[scnum].fnames[3][0]!=0) result|= (!loadspecmon(scns[scnum].fnames[3],".mon","mon\\",LASTMON))<<3;
  else NoMons=TRUE;
  if (scns[scnum].fnames[4][0]!=0) result|= (!loadspecblk(scns[scnum].fnames[4],".mbl","blk\\",MONBL,blks[1]))<<4;
  else NoMons=TRUE;
  if (scns[scnum].fnames[5][0]!=0) result|= (!loadspecany(scns[scnum].fnames[5],".chr","chr\\",sizeof(chrstruct),(char *) &chr))<<5;
  if (scns[scnum].fnames[6][0]!=0) result|= (!loadspecblk(scns[scnum].fnames[6],".cbl","blk\\",CHARBL,blks[2]))<<6;
  if (result)
    {
    Palette(1,63,63,0);   // Yellow
    Palette(2,63,0,33);   // MAGENTA
    Palette(3,0,58,58);   // Turquoise
    Palette(4,63,0,0);    // Red

    MoveWindow(0);
    ViewWindow(0);
    MoveViewScreen(0,0);
    BoxFill(0,0,319,199,0);
    Box(10,10,310,190,4);
    Box(15,15,305,185,4);

    Gwritestr(5,7,1,"I am missing these game files:",30);
    for(loop=0,cury=9;loop<LASTFNAME;loop++,result>>=1)
      {
      if (result&1)  // The file is missing
        {
        MakeFileName(out,fpaths[loop],scns[scnum].fnames[loop],exts[loop]);
        Gwritestr(15,cury,2,out,strlen(out));
        cury++;
        }
      }
    Gwritestr(15,21,3,"Hit a key...",12);
    PauseTimeKey(20);
    return(FALSE);
    }
  // Load Sound Set
  if ((scns[scnum].fnames[7][0]!=0)&&(loadspecsnd(scns[scnum].fnames[7],".snd","snd\\")))
    anysound=TRUE;
  else anysound=FALSE;
  // Load Music
  if (SongInfo&CARDEXIST)        //Sound Board installed
    {
     if (scns[scnum].fnames[8][0]!=0)
      {
      MakeFileName(out,"snd\\",scns[scnum].fnames[8],".cmf");
      if (!InitMusic(&Instruments,out)) SongInfo|=SONGLOADED; // Song is valid
      }
    }
  return(TRUE);
  }

static int loadspecsnd(char *file,char *ext,char *path)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN];
  register int loop;

  MakeFileName(fname,path,file,ext);

  if ((fp=fopen(fname,"rb"))==NULL) return(FALSE);
  fread((unsigned char *)snd,sizeof(sndstruct),LASTSND,fp);
  for (loop=0;loop<9*LASTSND;loop++) *(DigiSnd[0]+loop)=0;
  fread(DigiSnd,9*LASTSND,1,fp);
  fclose(fp);
  return(TRUE);

  }


static int loadspecmon(char *file,char *ext, char *path,int nummons)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN];

  MakeFileName(fname,path,file,ext);

  if ((fp=fopen(fname,"rb"))==NULL) return(FALSE);
  fread((unsigned char *)mon,sizeof(monstruct),nummons,fp); 
  fclose(fp);
  return(TRUE);
  }

static int loadspecblk(char *file,char *ext, char *path,int numblks,blkstruct *bl)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN];

  MakeFileName(fname,path,file,ext);

  if ((fp=fopen(fname,"rb"))==NULL) return(FALSE);
  fread((unsigned char *)bl,sizeof(blkstruct),numblks,fp); 
  fclose(fp);
  return(TRUE);
  }

static int loadspecmap(char *file, char *ext, char *path)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN];
  
  MakeFileName(fname,path,file,ext);
  
  if  ((fp=fopen(fname,"rb"))==NULL) return(FALSE); 
  fread( (char*) &map,sizeof(mapstruct),MLEN*MWID,fp);
  fread( (char*) &mm,sizeof(monmapstruct),LASTMM,fp); 
  fclose(fp);
  return(TRUE);
  }

static void drawmap(int x,int y)
  {
  register int lx,ly;

  for (lx=0;lx<16;lx++)
    for (ly=0;ly<10;ly++)
      BufDrawBlk(lx*BLEN,ly*BLEN,blks[0][map[(ly+y)%100][(lx+x)%100].blk].p[0]);

  SvgaBufToScrn(65535,zeroaddon);
  }

static void initalldata(void)
  {
  unsigned register int loop;
 
  ci.score=0;

  for (loop=0; loop<BACKBL;loop++)
    {
    nxtchg[loop]=0;
    blkmap[loop]=loop;
    }    
  for (loop=0;loop<LASTMM;loop++)
    mi[loop].save = NULL;
  }

static char HandleJStick(int fn)
  {
  register int loop;
  char retval=FALSE;
  char joypos=0;
  int joybut=0;
  static char oldbut=0;
  static unsigned long oldtime=1;

  if (fn==RESETTIMER) { oldtime=clock; return(FALSE); }

  if ((jstickon)&&(joyinstall)&&(oldtime+JSTICKPOLLSPEED<=clock))
    {
    oldtime=clock;
    GetJoyPos(&joypos, &joybut, &cs);
    if ((joypos==0)&&(joybut==0)) 
      {
      oldbut=0;
      if (curkey==JSTICKKEY) PopKey(JSTICKKEY);
      return(FALSE);
      }

    if (chr.cseq!=DIESEQ)
      {
      for (loop=3;loop<MAXSEQ;loop++)
        { 
        if ((chr.sq[loop].jstk_mov!=0)    // = 0 no joystick sequence
         && ((chr.sq[loop].jstk_mov==9)||(chr.sq[loop].jstk_mov==joypos))
         && (!chr.sq[chr.cseq].ground) // Interruptible?
         && (ci.meter[loop]>0)            // Shot left?
         && (ci.seqon[loop])              // Sequence enabled?
         && ((chr.sq[loop].jstk_but==0)                // Any button
         ||  ((joybut==0)&&(chr.sq[loop].jstk_but==4)) // No buttons
         ||    ((joybut!=oldbut)&&(chr.sq[loop].jstk_but==joybut))))
          {   /* J-stick matches w/ a sequence - do the sequence */
          oldbut=joybut;
          if (chr.cseq!=loop)
            { // New Sequence - switch to it
            if (curkey!=JSTICKKEY) PushKey(); // Joystick does not stack like keys, but completely replaces
            curkey=JSTICKKEY;  // Remember the input device
            chr.cframe=-1;     // Set up info to do the sequence
            chr.cseq=loop;
            ci.NextAni=clock;  // Force the char to move or draw the new sequence
            if (chr.sq[loop].sound<LASTSND) DoSound(chr.sq[loop].sound);
            retval=TRUE;
            }
          break;
          }
        }   //  End of For loop 
      }
    }
  return(retval);
  }

static void interrupt NewTimer(...)
  {
  if ((SndClock&7)==0)
    { 
    (*OldTimer)();
#ifndef REALLYFAST
    clock++;
#endif
    }
  else
    {
    enable();
    outportb(0x20,0x20); // Tell 8259 PIC controller to enable interrupts
    }
#ifdef REALLYFAST
  if ((SndClock%4)==0) clock++;
#endif
  if ((CurSnd<LASTSND)&&(AtNote<NUMFREQ))
    {
    sound(snd[CurSnd].freq[AtNote]);
    AtNote++;
    }
  else if (AtNote==NUMFREQ) { AtNote++; nosound(); }
  if (SongInfo==(CARDEXIST|SONGLOADED)) PlayIt();
  SndClock++;
  #ifdef RECORDER
  if (keyFlag)
    {
    keyFlag=FALSE;
    if (RecFlag==RECORD)
      {
      Rec[RecIndex].key=key;
      Rec[RecIndex].DeltaTime=SndClock-LastKeyPress;
      LastKeyPress=SndClock;
      RecIndex++;
      }
    SimKeyPress(key);
    }
  if ((RecFlag==PLAYBACK)&&(SndClock>=LastKeyPress+Rec[RecIndex].DeltaTime))
    {
    LastKeyPress=SndClock;
    SimKeyPress(Rec[RecIndex].key);
    RecIndex++;
    }
  #endif
  return;
  }

/*  // This routine has been converted to assembly language.
static void interrupt NewKbdAT(...)
  { 
 // (*OldKbd)();
  outportb(0x20,0x20);  // Tells 8259 PIC controller to enable all interrupts
  enable();             // Tells CPU to enable all interrupts

  if (in==1) return;

  in=1;
  while ((key=inportb(0x60))!=oldkey)
    {                       // loop in case key comes in when we're in here.
    oldkey=key;
//    if (key==1)         keydn[1]=TRUE;
//    else if (key==0x81) keydn[1]=FALSE;
//    else
//      {
      KeysPending[PendCtr]=key;
      PendCtr++;
//      keyFlag=TRUE;    // Only needed for recorder
//      }
    }
  in=0;
  return;
  }
*/

static void SimKeyPress(unsigned char key)
  { 
  int loop=0;
  if (key < 0x80)        // Key Pushed (if high bit set key was released)
    {
    if (keydn[key]==0)
      {
      for (loop=0;loop<MAXSEQ;loop++)
        if ((chr.sq[loop].key>>8)==key)
          {
          if ((chr.cseq!=DIESEQ)&&
              (!chr.sq[chr.cseq].ground)&&   //Interruptible=!.ground
              (ci.meter[loop]>0)&&
              (ci.seqon[loop])&&
              (keystkptr<MAXKEYS))
            {
            keystack[keystkptr].key=curkey;
            keystack[keystkptr].seq=chr.cseq;
            keystack[keystkptr].frame=chr.cframe;
            keystkptr++;
            if (keystkptr>=MAXKEYS) CleanKeyStk();
            chr.cseq=loop;
            chr.cframe=-1;
            ci.NextAni=clock;  /* Force the char to move or draw the new sequence */
            loop=MAXSEQ;
            curkey=key;
            }
          }
      keydn[key]=1;
      }
    }
  else
    {
    key &=127;
    if (keydn[key]!=0)
      {
      keydn[key]=0;
      for (loop=0;loop<keystkptr;loop++)
        if (keystack[loop].key==key) { keystack[loop].key=0xFF; }
         
      if ((key==curkey)&&(keystkptr>0)&&(!chr.sq[chr.cseq].ground))
        {
        while (keystack[keystkptr-1].key==0xFF) keystkptr--;
        curkey    = keystack[keystkptr-1].key;
        chr.cseq  = keystack[keystkptr-1].seq;
        chr.cframe= keystack[keystkptr-1].frame;
        ci.update=TRUE;
        ci.NextAni=clock;
        keystkptr--;
        }
      }
    }
  }

static void CleanKeyStk(void)
  {
  int from,to;

  for(from=0,to=0;from<keystkptr;from++)
    {
    if (keystack[from].key!=0xFF)  // Move this valid stack entry
      {
      if (from!=to)
        {
        keystack[to].key=keystack[from].key;
        keystack[to].seq=keystack[from].seq;
        keystack[to].frame=keystack[from].frame;
        }
      to++;
      }
    }
  keystkptr=to;
  }

static void PushKey(void)
  {
  keystack[keystkptr].key=curkey;
  keystack[keystkptr].seq=chr.cseq;
  keystack[keystkptr].frame=chr.cframe;
  keystkptr++;
  }

static void PopKey(unsigned char key)
  {
  int loop;

  for(loop=0;loop<keystkptr;loop++)
    if (keystack[loop].key==key) keystack[loop].key=0xFF;
  while ( (keystack[keystkptr-1].key==0xFF)&&(keystkptr>0)) keystkptr--;
  if (keystkptr==0)
    {
    curkey=0;
    chr.cseq=IDLESEQ;
    chr.cframe=0;
    }
  else
    {
    curkey    = keystack[keystkptr-1].key;
    chr.cseq  = keystack[keystkptr-1].seq;
    chr.cframe= keystack[keystkptr-1].frame;
    keystkptr--;
    if (chr.cseq==IDLESEQ) chr.cframe = 0;     // Kludge to restart the idle
                                               // seq every time 'cause users
                                               // wanted it like this.
    }
  ci.update=TRUE;
  ci.NextAni=clock;
  }

static void cleanup(void)
  {
  farfree(blks[0]);
  farfree(blks[1]);
  farfree(blks[2]);
  farfree(mon);
  farfree(ci.savetop);
  farfree(ci.savebottom);
  farfree(MonsterMem);
  if (BlkMon) farfree(BlkMon);
  #ifdef RECORDER
  farfree(Rec);
  #endif
  UnInitDirty();
  #ifdef DIGITALSND
  if (SongInfo) ShutSbVocDriver();
  #endif
  }

static void SlowKeys(void)
  {
  _AX = 0x0305;
  _BH = 3;
  _BL = 0x1F;
  geninterrupt(0x16);
  }

static void FastKeys(void)
  {
  _AX = 0x0305;
  _BH = 0;
  _BL = 0;
  geninterrupt(0x16);
  }

static unsigned char DoSound(unsigned char sndnum)
  {
  if (anysound&&soundon&&(sndnum<LASTSND))
    {
    #ifdef DIGITALSND
    if ((DigiSnd[sndnum][0]==0) || (DigiSnd[sndnum][0]==' ') || (!(SongInfo&CARDEXIST)))
      {
    #endif
      CurSnd=sndnum;
      AtNote=0;
    #ifdef DIGITALSND
      }
    else
      {
      PlaySbVocFile((char far *) DigiSnd[sndnum]);
      }
    #endif
    return(TRUE);
    }
  return(FALSE);
  }

static void SlowTimer(void)
  {
  outportb(0x43,0x36);    // Control reg., write LSB then MSB to counter 0
  outportb(0x40,65535%256); // Write the LSB - 18.2X/sec = 65535
  outportb(0x40,65535/256); // Write the MSB

  }

static void FastTimer(void)
  {
#ifndef WAYSLOW
  outportb(0x43,0x36);  // Control reg., write LSB then MSB to counter 0
  outportb(0x40,(65535/8)%256);  // Write the LSB - 18.2X/sec = 65535
  outportb(0x40,(65535/8)/256);  // Write the MSB - so / 8 for much faster
#endif
  }

static void StartGameInits(void)
  {
  register int l1;

  clock=1;
  SndClock=1;
  MoveCharGrav(1); // Reset gravity  timer
  HandleJStick(1); // Reset joystick timer

  ci.hitpts=-1;
  ci.lives=-1;
  ci.score=0;
  for (l1=0;l1<MAXINV;l1++) ci.inv[l1]=BACKBL+1;
  for (l1=0;l1<MAXSEQ+10;l1++) ci.meter[l1]=-1;
  }

static void StartSceneInits(int startx,int starty)
  {
  int loop;
  /**********************************************************/
  /* Init. all scene specific stuff about character         */
  /**********************************************************/

  if (ci.inv[0]==BACKBL+1)
    for (loop=0;loop<MAXINV;loop++)
      ci.inv[loop] = chr.inv[loop];
      
  for(loop=0;loop<MAXSEQ;loop++)
    ci.seqon[loop] = (chr.sq[loop].bloc==BACKBL);
  for (loop=0;loop<MAXINV;loop++)
    checkseq_fortie(ci.inv[loop],TRUE);

  if (ci.meter[0]==-1)  /* Impossible (would be dead) - so means init. data struct from chr.meter[] */
    for (loop=0;loop<MAXSEQ+10;loop++) ci.meter[loop]=chr.meter[loop];

  chr.x[0]=startx; //   Start Character in
  chr.y[0]=starty; // Proper spot.
  chr.x[1]=0;      //   Start Character in
  chr.y[1]=0;      // Proper spot.
  chr.cseq=0;      // idle sequence
  chr.cframe=-1;   // Current frame is the first one
  if (ci.hitpts==-1) ci.hitpts=ci.meter[0];
  if (ci.lives==-1)  ci.lives =ci.meter[1];
  ci.netx=5;  // Delta of character's last 2 moves
  ci.nety=0;
  ci.gravx=0; // Current velocity due to gravity
  ci.gravy=0;
  ci.scrx=ci.oldx=ci.nextx=OFFSCRN; // Where character is located on the screen
  ci.scry=ci.oldy=ci.nexty=OFFSCRN;
  ci.topthere   =FALSE; // Char not on screen, top+bottom not there
  ci.bottomthere=FALSE;
  ci.curtop    =CHARBL; // Not on screen, so no current block to draw
  ci.curbottom =CHARBL;
  ci.NextAni=clock;

  mx=startx-(SCRNBLEN/2);  // Set up screen variables around character
  my=starty-(SCRNBWID/2);
  if (mx<0) mx+=MLEN;      // Contend for the map wrap around
  if (my<0) my+=MWID;
  scroll.nx=scroll.ny=0;
  scroll.movx=scroll.movy=0;
  scroll.cont=FALSE;

  /***************************************************/
  /*      Loop to initialize monster array           */
  /***************************************************/
  for (loop=0;loop<LASTMM;loop++)
    {
    mi[loop].Reset();
    BirthMon(loop,mm[loop].monnum,mm[loop].curx,mm[loop].cury);
    mi[loop].save     =          NULL;
    mi[loop].fromchar =         FALSE; // If birthed from char, can't hurt it
    }

/**********************************************************/
/*      Loop to initialize the next time change array     */
/**********************************************************/
  for (loop=0;loop<BACKBL;loop++)       // Setup to loop for every block
    {                                  
    blkmap[loop]=loop;                        // Set current block index to itself
    if (blks[0][loop].ntime>0)                // Does block change over time?
      nxtchg[loop]=clock+blks[0][loop].ntime; // Set abs time, next block change
    else                                      // When block does not change on time
      nxtchg[loop]=0xFFFFFFFF;                // Set to change time to infinite
    }

  if (!NoMons)
    {
    InitBirthArray();
    for (loop=0;loop<MaxBlkMon;loop++)
      {
      if (blks[0][BlkMon[loop].blnum].Birthtime == 0)
        BlkMon[loop].nexttime=0xFFFFFFFF;
      else BlkMon[loop].nexttime=clock+blks[0][BlkMon[loop].blnum].Birthtime;
      }
    }
/**********************************************************/
/*      Loop to initialize the key down array             */
/**********************************************************/
  for (loop=0;loop<128;loop++)         /* Setup to loop key array */
    {                                  /* Start of "for" loop */
    keydn[loop]=0;                     /* Initialize the array element */
    }                                  /* End of "for" loop */
/**********************************************************/
/*      Loop to initialize the key stack array     */
/**********************************************************/
  for (loop=0;loop<20; loop++)         /* Setup to loop the key stack array */
    {                                  /* Start of "for" loop */
    keystack[loop].seq=0;              /* Zero "seq" array element field */
    keystack[loop].frame=0;            /* Zero "frame" array element field */
    keystack[loop].key=-1;             /* Seed "key" array element field */
    }                                  /* End of "for" loop */
  keystkptr=0;                         /* Zero the key stack pointer */
  keydn[CHRSEQ.key] = 1;
  curkey=0;
  }

static void ShowVidMem(void)
  {
  char far *memspot = (char far *) 0xC0000000;
  int l=0;  
  int done=0;
  char s[81];

  while (!done)
    {
    GetMem(s,memspot+(80*l));
    writestr(0,l%24,1,s);
    l++;
    if ((bioskey(0)&255)==27) done=1;
    }
  }

void GetMem(char *s,char *mem)
  {
  int l;
  for (l=0;l<80;l++) s[l]=mem[l];
  }

#define SNDCARDX 35
#define SNDCARDY 20
#define VGACARDX 192
#define VGACARDY 25
#define PORTX    22
#define PORTY    75
#define INTX     110
#define INTY     75

static void DrawConfigScrn(ConfigStruct *cs,int Vga,char *DrvrName,char *DrvrPath)
  {
  char VgaTypes[][14]={"Auto-Detect","Standard VGA","Paradise","Trident 8800","Trident 8900","Tseng ET3000","Tseng ET4000","ATI","ATI Wonder"};
  int l;
  int inkey;
  char str[5];

  DrawSurf(5,5,150,190);    // Sound and Music background
  DrawSurf(170,5,140,160);  // VGA Card background
  DrawSurf(170,180,40,10);  // Done button
  DrawSurf(220,180,40,10);  // Help button


  GWrite(190,8,7,"VGA Chip Type");
  GWrite(20,8,7,"Sound And Music");
  GWrite(20,60,7,"Port  Interrupt");
  GWrite(20,155,7,"Sound Driver");
  BoxFill(9,169,150,178,3);
  Line(9,169,150,169,1);
  Line(9,169,9,178,1);
  BoxFill(9,181,150,190,3);
  Line(9,181,150,181,1);
  Line(9,181,9,190,1);

  inkey=50;
  GWrite(SNDCARDX,SNDCARDY,inkey++,"None");
  GWrite(SNDCARDX,SNDCARDY+16,inkey++,"Sound Blaster");

  for(l=0;l<5;l++,inkey++)
    {
    sprintf(str,"%x",(l*0x10)+0x220);
    GWrite(PORTX,PORTY+(l*0x10),inkey,str);
    }
  GWrite(10,170,inkey++,DrvrName);
  GWrite(10,182,inkey++,DrvrPath);

  GWrite(INTX,INTY,inkey++,"2");
  for(l=0;l<3;l++,inkey++)
    {
    sprintf(str,"%d",(l*2)+3);
    GWrite(INTX,INTY+((l+1)*0x10),inkey,str);
    }
  GWrite(INTX,INTY+((l+1)*0x10),inkey++,"10");

  for (l=0;l<9;l++,inkey++) GWrite(VGACARDX,VGACARDY+(l*0x10),inkey,VgaTypes[l]);

  GWrite(171,181,inkey++,"Help");
  GWrite(221,181,inkey++,"Done");

  CuteBox(PORTX-3,PORTY-3+(cs->SndPort-0x220),PORTX+25,PORTY+9+(cs->SndPort-0x220),8,9);
  CuteBox(INTX-3,INTY-3+((cs->SndInt-1)*8),INTX+18,INTY+9+((cs->SndInt-1)*8),8,9);
  CuteBox(VGACARDX-3,VGACARDY-3+(Vga*16),VGACARDX+(12*8)+3,VGACARDY+9+(Vga*16),8,9);
  CuteBox(SNDCARDX-3,SNDCARDY-3+(cs->ForceSnd*16),SNDCARDX+(13*8)+3,SNDCARDY+9+(cs->ForceSnd*16),8,9);
  }


static int AskConfigData(ConfigStruct *cs)
  {
  unsigned char Cols[4];
  int  DelayTimer=0;
  char str[5];
  int l;
  int Vga=0;
  char VgaTypes[][14]={"Auto-Detect","Standard VGA","Paradise","Trident 8800","Trident 8900","Tseng ET3000","Tseng ET4000","ATI","ATI Wonder"};
  int Cursor=0,OldC=0;
  int inkey;
  char DrvrName[21]="";
  char DrvrPath[21]="";
  int PortC,IntC,DrvrC,VCardC,ButC;
  int mx=0,my=0,mbuts=0;
  int ox=0,oy=0,obuts=0;

//  SetAllPalTo(&colors[0]);

  if (strcmpi((const char far *) cs->SndDrvr,"NotInit")==0)
    DetectCard(&cs->ForceSnd,&cs->SndPort,&cs->SndInt,cs->SndDrvr);


  if (cs->ForceVGA==ATIWonder)     Vga=8;
  else if (cs->ForceVGA==ATI)      Vga=7;
  else if (cs->ForceVGA==NoType)   Vga=0;
  else if (cs->ForceVGA==Unknown)  Vga=1;
  else if (cs->ForceVGA>=Paradise) Vga= cs->ForceVGA-Paradise+2;

  if ((cs->SndPort<0x220)||(cs->SndPort>0x260)) cs->SndPort=0x220;

  if (cs->SndInt==2) cs->SndInt=1;   // Translate interrupts to an even count
  if (cs->SndInt==10) cs->SndInt=9;  // from 1 to 9.

  if ((cs->SndInt<1)||(cs->SndInt>9)||((cs->SndInt%2)==0)) cs->SndInt=7;

  for(l=strlen(cs->SndDrvr);l>=0;l--)
    {
    if (cs->SndDrvr[l]=='\\')
      {
      strcpy(DrvrName,&cs->SndDrvr[l+1]);
      cs->SndDrvr[l+1]=0;
      l=-100;
      }
    }
  if (l==-101) strcpy(DrvrPath,cs->SndDrvr);
  else
    {
    strcpy(DrvrName,cs->SndDrvr);
    DrvrPath[0]=0;
    }

  DrawSurf(5,5,150,190);    // Sound and Music background
  DrawSurf(170,5,140,160);  // VGA Card background
  DrawSurf(170,180,40,10);  // Done button
  DrawSurf(220,180,40,10);  // Help button

  GWrite(190,8,7,"VGA Chip Type");
  GWrite(20,8,7,"Sound And Music");
  GWrite(20,60,7,"Port  Interrupt");
  GWrite(20,155,7,"Sound Driver");

  BoxFill(9,169,150,178,3);
  Line(9,169,150,169,1);
  Line(9,169,9,178,1);
  BoxFill(9,181,150,190,3);
  Line(9,181,150,181,1);
  Line(9,181,9,190,1);

  inkey=50;
  GWrite(SNDCARDX,SNDCARDY,inkey++,"None");
  GWrite(SNDCARDX,SNDCARDY+16,inkey++,"Sound Blaster");

  PortC=inkey-50;
  for(l=0;l<5;l++,inkey++)
    {
    sprintf(str,"%x",(l*0x10)+0x220);
    GWrite(PORTX,PORTY+(l*0x10),inkey,str);
    }
  DrvrC=inkey-50;
  GWrite(10,170,inkey++,DrvrName);
  GWrite(10,182,inkey++,DrvrPath);

  IntC=inkey-50;
  GWrite(INTX,INTY,inkey++,"2");
  for(l=0;l<3;l++,inkey++)
    {
    sprintf(str,"%d",(l*2)+3);
    GWrite(INTX,INTY+((l+1)*0x10),inkey,str);
    }
  GWrite(INTX,INTY+((l+1)*0x10),inkey++,"10");

  VCardC=inkey-50;
  for (l=0;l<9;l++,inkey++) GWrite(VGACARDX,VGACARDY+(l*0x10),inkey,VgaTypes[l]);

  ButC=inkey-50;
  GWrite(171,181,inkey++,"Help");
  GWrite(221,181,inkey++,"Done");

  CuteBox(PORTX-3,PORTY-3+(cs->SndPort-0x220),PORTX+25,PORTY+9+(cs->SndPort-0x220),8,9);
  CuteBox(INTX-3,INTY-3+((cs->SndInt-1)*8),INTX+18,INTY+9+((cs->SndInt-1)*8),8,9);
  CuteBox(VGACARDX-3,VGACARDY-3+(Vga*16),VGACARDX+(12*8)+3,VGACARDY+9+(Vga*16),8,9);
  CuteBox(SNDCARDX-3,SNDCARDY-3+(cs->ForceSnd*16),SNDCARDX+(13*8)+3,SNDCARDY+9+(cs->ForceSnd*16),8,9);

  Cursor=ButC;
  for(l=50;l<75;l++)   // Set the color of the writing.
    {
    colors[l].blue =63-(abs((l-50)-Cursor))*2;
    colors[l].green=30;
    colors[l].red  =0;
    }
  insertcol(Cursor+50,63,63,63,colors);

  colors[7].Set(10,50,63);
  colors[8].Set(60,60,20);
  colors[9] = (colors[8]+colors[5])/2;
  colors[10].Set(20,60,20);
  colors[11].Set(10,40,10);

  for (l=0;l<4;l++) colors[l+1].Set(10+(l*4),0,10+(l*4));
  FadeTo(colors);

  DelayTimer=0;
  if (mouinstall)
    {
    moustats(&ox,&oy,&obuts);
    setmoupos(100,100);
    }
  while(1)
    {
    inkey=0;
    do
      {
      #ifdef MOUSE
      if (mouinstall)
        {
        moustats(&mx,&my,&mbuts);
          // Translate mouse into keyboard movements
        if (mx<60)        inkey=(LEFTKEY<<8);
        else if (mx>140)  inkey=(RIGHTKEY<<8);
        else if (my<90)   inkey=(UPKEY<<8);
        else if (my>110)  inkey=(DOWNKEY<<8);
        else if ((mbuts>0)&&(obuts==0)) inkey=13;
        if (inkey) { mx=100; my=100; }
        obuts=mbuts;
        setmoupos(mx,my);
        }
      #endif
      if (!inkey) if ((inkey=bioskey(1))!=0) bioskey(0);  // only do if mouse not changed
      delay(1);
      DelayTimer++;
      if (DelayTimer==400) Palette(Cursor+50,0,30,63);
      if (DelayTimer==900) { Palette(Cursor+50,63,63,63); DelayTimer=0; }
      } while (!inkey);
    OldC=Cursor;
    switch(inkey&255)
      {
      case 0:
        switch(inkey>>8)
          {
          case RIGHTKEY:
            if (Cursor>ButC)           Cursor=DrvrC+1;
            else if (Cursor==ButC)     Cursor=ButC+1;
            else if (Cursor>=VCardC+3) Cursor=PortC-3+Cursor-VCardC;
            else if (Cursor>=VCardC)   Cursor=Cursor-VCardC;
            else if (Cursor>=IntC)     Cursor=VCardC+3+(Cursor-IntC);
            else if (Cursor>=DrvrC+1)  Cursor=ButC;
            else if (Cursor>=DrvrC)    Cursor=VCardC+8;
            else if (Cursor>=PortC)    Cursor=IntC+(Cursor-PortC);
            else                       Cursor=VCardC+Cursor;
            break;
          case DOWNKEY:
            Cursor++;
            break;
          case LEFTKEY:
            if (Cursor>ButC)         Cursor=ButC;
            else if (Cursor==ButC)   Cursor=DrvrC+1;
            else if (Cursor>=VCardC+8) Cursor=DrvrC;
            else if (Cursor>=VCardC+3) Cursor=IntC-3+(Cursor-VCardC);
            else if (Cursor>=VCardC+2) Cursor=IntC;
            else if (Cursor>=VCardC)   Cursor=Cursor-VCardC;
            else if (Cursor>=IntC)     Cursor=PortC+(Cursor-IntC);
            else if (Cursor>=DrvrC+1)  Cursor=ButC+1;
            else if (Cursor>=DrvrC)    Cursor=VCardC+8;
            else if (Cursor>=PortC)    Cursor=VCardC+(Cursor-PortC)+3;
            else                       Cursor=VCardC+Cursor;
            break;
          case UPKEY:
            Cursor--;
            break;
          }
        break;
      case 13:
        int Sel;
        if (Cursor==DrvrC) { GGet(10,170,DrvrC+50,3,DrvrName,12); break;        }
        else if (Cursor==DrvrC+1) { GGet(10,182,DrvrC+51,3,DrvrPath,17); break; }
        else if (Cursor==ButC)
          {
          BoxFill(0,0,SIZEX,SIZEY,0);
          for(l=0;l<5;l++)
            Box(40-(64*4)/10-l,100-64-l,280+(64*4)/10+l,180+(64*2)/10+l,l);
          if (mouinstall) mouclearbut();
          Display_File("config.hlp",7);
          BoxFill(0,0,SIZEX,SIZEY,0);
          DrawConfigScrn(cs,Vga,DrvrName,DrvrPath);
          break;
          }
        else if ((Cursor!=ButC+1))
          {
          
          if (Cursor>=VCardC)     CuteBox(VGACARDX-3,VGACARDY-3+(Vga*16),VGACARDX+(12*8)+3,VGACARDY+9+(Vga*16),4,4);
          else if (Cursor>=IntC)  CuteBox(INTX-3,INTY-3+((cs->SndInt-1)*8),INTX+18,INTY+9+((cs->SndInt-1)*8),4,4);
          else if (Cursor>=PortC) CuteBox(PORTX-3,PORTY-3+(cs->SndPort-0x220),PORTX+25,PORTY+9+(cs->SndPort-0x220),4,4);
          else                    CuteBox(SNDCARDX-3,SNDCARDY-3+(cs->ForceSnd*16),SNDCARDX+(13*8)+3,SNDCARDY+9+(cs->ForceSnd*16),4,4);

          if (Cursor==0) cs->ForceSnd=None;
          else if (Cursor==1) cs->ForceSnd=SndBlaster;
          else if (Cursor>=VCardC) Vga=(Cursor-VCardC);
          else if (Cursor>=IntC)   cs->SndInt=((Cursor-IntC)*2)+1;
          else if (Cursor>=PortC)  cs->SndPort=((Cursor-PortC)*0x10)+0x220;

          if (Cursor>=VCardC)     CuteBox(VGACARDX-3,VGACARDY-3+(Vga*16),VGACARDX+(12*8)+3,VGACARDY+9+(Vga*16),8,9);
          else if (Cursor>=IntC)  CuteBox(INTX-3,INTY-3+((cs->SndInt-1)*8),INTX+18,INTY+9+((cs->SndInt-1)*8),8,9);
          else if (Cursor>=PortC) CuteBox(PORTX-3,PORTY-3+(cs->SndPort-0x220),PORTX+25,PORTY+9+(cs->SndPort-0x220),8,9);
          else                    CuteBox(SNDCARDX-3,SNDCARDY-3+(cs->ForceSnd*16),SNDCARDX+(13*8)+3,SNDCARDY+9+(cs->ForceSnd*16),8,9);
          break;
          }
      case 27:
         int t;
         if (DrvrPath[0]==0) strcpy(DrvrPath,".\\");
         t=strlen(DrvrPath);
         if (DrvrPath[t-1]!='\\') { DrvrPath[t]='\\'; DrvrPath[t+1]=0; }
         sprintf(cs->SndDrvr,"%s%s",DrvrPath,DrvrName);
         if (Vga==0) cs->ForceVGA=NoType;
         if (Vga==1) cs->ForceVGA=Unknown;
         else if (Vga==8) cs->ForceVGA=ATIWonder;
         else if (Vga==7) cs->ForceVGA=ATI;
         else if (Vga>=2) cs->ForceVGA=Paradise+(Vga-2);
         if (cs->SndInt==1) cs->SndInt=2;   // Translate interrupts from an
         if (cs->SndInt==9) cs->SndInt=10;  // even count to the real value.
         return(TRUE);
      }
    if (Cursor>ButC+1) Cursor=0;
    if (Cursor<0)  Cursor=ButC+1;

    if (OldC!=Cursor)
      {
      for(l=50;l<75;l++)
        {
        colors[l].blue =63-(abs((l-50)-Cursor))*2;
        colors[l].green=30;
        colors[l].red  =0;
        Palette(l,colors[l].red,colors[l].green,colors[l].blue);
        }
      Palette(Cursor+50,63,63,63);
      }
    }
  }

static void CuteBox(int x,int y,int x1,int y1,char col1,char col2)
  {
  Line(x+1,y,x1-1,y,col1);
  Line(x+1,y-1,x1-1,y-1,col2);

  Line(x+1,y1,x1-1,y1,col1);
  Line(x+1,y1+1,x1-1,y1+1,col2);

  Line(x,y+1,x,y1-1,col1);
  Line(x-1,y+1,x-1,y1-1,col2);

  Line(x1,y+1,x1,y1-1,col1);
  Line(x1+1,y+1,x1+1,y1-1,col2);

  Point(x+1,y+1,col1);
  Point(x+1,y1-1,col1);
  Point(x1-1,y+1,col1);
  Point(x1-1,y1-1,col1);
  Point(x,y,col2);
  Point(x,y1,col2);
  Point(x1,y,col2);
  Point(x1,y1,col2);
  }

static void DrawSurf(int x,int y,int xlen,int ylen)
  {
  int l;
  BoxFill(x,y,x+xlen,y+ylen,4);
  for (l=3;l>0;l--) Box(x-l,y-l,x+xlen+l,y+ylen+l,((unsigned char)(4-l)));
  }

static void StealKbd(void)
  {
  if (MicroChnl) setvect(0x9,NewMicroKbd);
  else setvect(0x9,NewATKbd);
  }

void Screen(int status)  /* turns the screen on(1) or off(0) */
  {
  static int scrn=1;
  if (status !=scrn)
    {
    outportb(0x3C4,1);
    if (status) { scrn=1; outportb(0x3C5,inportb(0x3C5)&223); }
    else        { scrn=0; outportb(0x3C5,inportb(0x3C5)|32); }
    outportb(0x3C4,0);
    }
  }


