/*--------------------------------------------------------------------*/
/* grator.c             Programmer: Andy Stone   Created: 7/12/91     */
/* Program that combines all of the maps in a game into a cohesive    */
/* Whole.                                                             */
/*                                                                    */
/*--------------------------------------------------------------------*/

#include <dos.h>
#include <dir.h>
#include <bios.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <alloc.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>

#include "gen.h"
#include "pixel.hpp"
#include "coord2d.hpp"

#include "scrnrout.h"
#include "windio.h"

#include "gmgen.h"
#include "palette.h"
#include "mon.h"
#include "jstick.h"
#include "graph.h"
#include "facelift.h"
#include "tranmous.hpp"

#include "grator.h"

#define TRANSCOL 255     // The value that is transparent in _pic fns
#define BCOL     100     // Default color #
#define NOTUSED  0xFFFF  // Number in a scene variable when scene not used

#define BACKLINK   20
#define MENUX      10
#define MENUY      3
#define HELPX      50
#define HELPY      3
#define SELECTX    MENUX
#define SELECTY    15
#define DELETEX    HELPX
#define DELETEY    15
#define WORLDTOP   29
#define WORLDCOL   FaceCols[GREY2]
#define TITLECOL   12
#define WX         5
#define WLEN       310
#define WWID       165
#define SELDESCY   5
#define NORMDESCY  (SELDESCY+12)
#define TEXTX      200
#define IN(x1,y1,x2,y2) ((x>x1)&&(x<x2)&&(y>y1)&&(y<y2))
        // IN() is true if the mouse is in the box of (x1...y2)


/*********************************************************************/
/*                        F U N C T I O N S                          */
/*********************************************************************/

static char CopyFile(const char *From,const char *To);
static void DealWithDirectories(char *ans);

static void initalldata(void);  // Sets all structures to initial values
static void edit(void);         // Called from Main Menu - edit the world
static void cleanup(void);      //   Called automatically when prog.
                                // uninitializes variables & int vects.

static void seleb(int x,int y,unsigned char *col);
static void deleb(int x,int y,unsigned char *col);

static int unpress(int bnum);   // Pop a button (SELECT,DELETE,etc.) up and set relevant variables
static int domouse(int x,int y,int buts,int fn);  // Handles all mouse fns in graphical graph screen

// LINK FUNCTIONS
static uint createlink  (uint from,uint to);     // Creates a link from scene to another
static void deletelink   (uint scene,int link);  // Delete a link
static uint retlink     (int x,int y);           // Returns index of link which is in the current scene and located under x,y
int expandlink (int scene,int link);             // Link map to map defined in Gramap.c

// SCENE FUNCTIONS
static void expandscene (uint scene);  // Text scrn - allows entry of file names
static void expandadv   (uint scene);  // Allows user to enter in FILE NAMES
static void expandstart (uint scene);
static void expandend   (uint scene);
static void movescene   (uint scnum);  // reposition a scene in the world map

static void drawall     (void); // Draws background stuff (buttons,boxes,etc)
static void ConvWorld   (void); // Squeeze world into current world box

static void DoScroll(int scnum);
static void ScrollChange(unsigned char *sinfo);
static void MakeScrollDir(char *out, const char *prefix, int scroll);
static void ScrollChange(unsigned char *sinfo);
static int  ForgetChanges(void);


/*********************************************************************/
/*               G L O B A L      V A R I A B L E S                  */
/*********************************************************************/

extern unsigned _stklen=10000U;  // Set the stack to 10000 bytes, not 4096

GameClass  Game;
extern int lastbl;
int ispal  =FALSE;
int acol   =BCOL;               // Accent color (yellow)  (FaceCols[GREEN])
int bcol   =BCOL+1;             // Background color (blue) (FaceCols[RED1])
int speccol=BCOL+2;             // Special color (green) - used for start and end scenes  (FaceCols[GREY3])

uint curworld=NOTUSED;
int  curlink =0;

static char          saved=TRUE;
extern unsigned char FaceCols[MAXFACECOL];      //Screen colors
extern char          *FontPtr;
extern unsigned char HiCols[4];
RGBdata              colors[256];

extern int           xor;

char DummyStr  [MAXFILENAMELEN]  = "";
char GratorFile[MAXFILENAMELEN]  = "";
char GameArea  [MAXFILENAMELEN]  = "";

const char GraphicExts[]="*.gif\0*.fli\0*.txt\0*.lst\0*.bkd\0";

/*********************************************************************/
/*                            C O D E                                */
/*********************************************************************/

Pixel SceneColor(uint Scene)
  {
  if (Scene==curworld) return(acol);
  else switch(Game.scns[Scene].gametype)
    {
    case STARTTYPE: return(speccol);
    case ENDTYPE:   return(speccol);
    case ADVTYPE:   return(bcol);
    case NOSCENE:
    default:        return(WORLDCOL);
    }
  }


QuitCodes main(int argc,char *argv[])
  {
  int  attr   = 0;
  char done   = FALSE;
  int  choice = -1;
  char *w     = NULL;
/*  printf("Scene Size:%d\n",sizeof(Scene));
  printf("Link  Size:%d\n",sizeof(Link));
  printf("Scroll Size:%d\n",sizeof(ScrollInfo));
  printf("Coord2d Size:%d\n",sizeof(Coord2d));
  printf("%d",MAXSCENELINKS);
*/
#ifdef DEBUG
  printf("Initialization finished.\n");
  NewAbortMSG("GM:ATE in main procedure.\n");
#endif
#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif
  if (!initmouse()) return(quit);   //--Quit if no mouse
  atexit(cleanup);
  FontPtr=GetROMFont();
  initalldata();
  SetTextColors();
  do
    {
    Cur.Off();
    if ((choice==5)||(choice==-1))
      {
      writestr(0,0,79,GMTOPBAR);
      writestr(0,1,31,"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
      if (w==NULL) w = new char [1000];
      attr = openmenu(50,3,27,12,w);
      writestr(50,3,PGMTITLECOL,"         INTEGRATOR        ");
      writestr(50,4,PGMTITLECOL,"        Version "GMVER"       ");
      writestr(50,5,PGMTITLECOL,"      By Gregory Stone     ");
      writestr(50,6,PGMTITLECOL,"     Copyright (C) 1994    ");
      writestr(51,8,attr+14, "Choose a game");
      writestr(51,9,attr+14, "New game");
      writestr(51,10,attr+14,"New game area (directory)");
      writestr(51,11,attr+14,"Edit the chosen game");
      writestr(51,12,attr+14,"Save the chosen game");
      writestr(51,13,attr+14,"Delete a game");
      writestr(51,14,attr+14,"Quit");
      if (choice==-1) choice=1;
      }
    mouclearbut();
    choice=vertmenu(50,8,27,7,choice);
    switch (choice)
      {
      case 1:
        if ((saved)||(ForgetChanges()))
          {
          FILE *fp;
          initalldata();
          strcpy(GratorFile,WorkDir);
          getfname(5,5,"Enter game to load: ","*.gam\0",GratorFile);
          if ( (fp=fopen(GratorFile,"rb"))==NULL) break;
          Game.Load(fp);
          fclose(fp);
          choice=4;
          }
        break;
      case 2:
        if ((saved)||(ForgetChanges()))
          {
          initalldata();
          GratorFile[0]=0;
          errorbox("New game created!","(C)ontinue",2);
          choice=4;
          }
        break;
      case 3:
        if ((saved)||(ForgetChanges()))
          {
          char k;
          do
            {
            k=0;
            strcpy(DummyStr,GratorFile);
            GratorFile[0]=0;
            if (!qwindow(2,7,21,"Enter game area name: ",GratorFile))
              { k='A'; strcpy(GratorFile,DummyStr); break; }
            if (mkdir(GratorFile)==-1)
              {
              char s[81];
              int key=0;
              sprintf(s,"I cannot create the area %s.",GratorFile);
              do {
                key=errorbox(s,"(A)bort     (E)nter name again");
                k=toupper(key&255);
                if (k==27) k='A';
                } while ((k!='A')&&(k!='E'));
              if (k=='A') break;
              }
            } while (k=='E');
          if (k==0)
            {
            initalldata();
            errorbox("New game area created!","(C)ontinue",2);
            strcpy(DummyStr,GratorFile);
            sprintf(WorkDir,"%s\\",GratorFile);
            strcpy(GratorFile,WorkDir);
            strcat(GratorFile,DummyStr);
            choice=4;
            }
          }
        break;
      case 4:
        closemenu(50,3,27,12,w);
        if (w) { delete w; w=NULL; }
        strcpy(GameArea,WorkDir);
        edit(); TextMode();
        choice=5;
        break;
      case 5:
        {
        choice=4;
#ifdef CRIPPLEWARE
        StdCrippleMsg();
#else
        FILE *fp;
        *FileExt(GratorFile)=0;
        if (qwindow(2,7,MAXFILENAMELEN-1,"Save game as:",GratorFile))
          {
          strcat(GratorFile,".gam");
          if ( (fp=fopen(GratorFile,"wb"))==NULL) break;
          if (Game.Save(fp)) { saved=TRUE; choice=7; }
          fclose(fp);
          }
#endif
        }
        break;
      case 6:
        delany("Delete which game: ","*.gam\0",WorkDir);
        break;
      case 0:
      case 7:
        if ((saved)||(ForgetChanges())) done=TRUE;
        break;
      }
    } while (!done);
  closemenu(50,3,27,12,w);
  if (w) { delete w; w=NULL; }
  return(menu);
  }

  // Allows a user to make scenes, connections, and jump to either the
  // map screen, or the screen where one specifies the files in a scene.
static void edit (void)
  {
  int done=FALSE,changed=FALSE;  
  int mx,my,mbuts,ox=MENUX+20,oy=MENUY,obuts=0;

  mouclearbut();          // Wait until mouse button is released
  domouse(mx,my,mbuts,1); // Reset static variables! - mx,my,mbuts not used
  drawall();              // Set up the screen
  Cur.Goto(ox,oy);        // Position the mouse on the menu button
  Cur.On();               // Turn on mouse

  while (!done)
    {
    changed=FALSE;     // not False if user has changed an input device
    while(!changed)
      {
      if (bioskey(1)) changed=1;
      moustats(&mx,&my,&mbuts);
      if ((mx!=ox)|(my!=oy)) changed |= 2;
      if (mbuts!=obuts) changed |=4;
      if (mbuts>0) changed |=8;
      } 
    if ((mbuts>0)|((changed&2)==2)|((changed&4)==4)) 
      done=domouse(mx,my,mbuts,0);
    if ((changed&1)==1) bioskey(0);
    ox=mx;oy=my;obuts=mbuts;
    } 
  }

  /* Domouse is where all functions in the graphical screen which are mouse */
  /* controlled are implemented.  This includes creating and erasing scenes, */
  /* choosing menu items, selecting scenes, and drawing links. */
static int domouse(int x,int y,int buts,int fn)
  {
  register uint lo;
  static uint line=NOTUSED;      // Link being created flag - TRUE = set to the scene # which is the start of the link
  static int ox=0,oy=0,obuts=0;
  static uint ognum=NOSCENE;
  static int olink =NOLINK;
  static int buttonpress=FALSE; // which button on left (SELECT, DELETE, etc.) is pressed
  static uint unselscene=NOTUSED;
  char   str[15];
  boolean inbox=FALSE;

  if (fn==1)
    {
    line       =NOTUSED; // Link being created flag - TRUE = set to the scene # which is the start of the link
    ox=oy=obuts=0;
    ognum      =NOSCENE;
    olink      =NOLINK;
    buttonpress=FALSE; // which button on left (SELECT, DELETE, etc.) is pressed
    unselscene =NOTUSED;
    return(TRUE);
    }

  if (line!=NOTUSED)   // if a Link between 2 worlds is being created
    {
    inbox=TRUE;
//    if (y>199)      { Cur.Goto(x,199); y=199; }
                // Don't allow off of screen

    if ((x!=ox)|(y!=oy)|(buts!=obuts))  
      { // If mouse has changed position or status, redraw line
      xor=1;  // xor is a global var, which, when 1 will cause graphics to be logically XORed to the scrn
      Line (Game.scns[line].Pos.x+2,Game.scns[line].Pos.y+2,ox,oy,acol);
              // erase old line (xor on same color = black)
      xor=0;
      if (buts>0) // if Link still being moved (or mouse button still pressed)
        {
        xor=1;
        Line (Game.scns[line].Pos.x+2,Game.scns[line].Pos.y+2,x,y,acol); // Draw link
        xor=0;
        ox=x; oy=y; obuts=buts;
        lo=Game.SceneAt(x,y); // returns the scene at x,y if there is one
        if (lo!=NOSCENE)
          {
          sprintf(str,"%-14s",Game.scns[lo].desc);
          BoxWithText(TEXTX,NORMDESCY,str,FaceCols);
          }
        }  
      else     // Link ready to be placed - connect with a world
        {
        lo=Game.SceneAt(x,y); // returns the scene at x,y if there is one
           
        if ((lo!=NOSCENE)&&(lo!=line))
          {  /* If there is a scene under the cursor, and it is not the scene
                from which the link originated (in variable 'line')... */
          if (createlink(line,lo)!=NOLINK) Game.CleanDrawLink(line,lo,bcol);
          } 
        line=NOTUSED;  // Link placed, so unset link being drawn pointer (line)
        Cur.Limit(0,0,SIZEX-5,SIZEY-5);
        Cur.On();
        }
      }
    } // Link betw 2 worlds functions done
  

  if ( (line==NOTUSED)&&IN(WX,WORLDTOP,WX+WLEN,WORLDTOP+WWID) )   
    {  // if no links currently being created

    if ((lo=Game.SceneAt(x,y))!=NOSCENE)
      {   /* If the mouse is in a scene box select, delete or expand the scene */
          /* sets variable 'lo' to the scene box.        */  
      inbox=TRUE;
      if ((ognum!=lo)|(obuts!=buts))   /* If scene has not already had something done to it. */
        {
                                            // SELECT SCENE
        if ((buttonpress==1)&&(buts>0)&&(curworld==NOTUSED))
          {   /* SELECT this scene if SELECT (1) down, and mouse button being pushed, and nothing is currently selected */ 
          Game.DrawSceneLinks(lo,bcol); // redraw links in blue color
          Game.DrawScene(lo,acol);      // redraw scene in accent color (yellow)
          sprintf(str,"%-14s",Game.scns[lo].desc);
          BoxWithText(TEXTX,SELDESCY,str,FaceCols);
          curworld=lo;                          // set current world flag
          }
                                           // DELETE SCENE
        else if ((buttonpress==2)&&(buts>0)) 
          {  /* ERASE the world DELETE button (2) down, and mouse button being pushed */ 
          buttonpress=unpress(buttonpress);  /* delete button pops up every time a scene is deleted */ 
          Game.EraseScene(lo);
          if (curworld==lo) curworld=NOTUSED; 
          Game.Draw();
          mouclearbut();
          }
                                               // MOVE SCENE
                                                  
        else if (buts==2) { movescene(lo); }
                                              // EXPAND SCENE
        else if ((buts>0)&&(obuts==0)) expandscene(lo);
                     // if mouse button not pressed, draw the scene title
        sprintf(str,"%-14s",Game.scns[lo].desc);
        BoxWithText(TEXTX,NORMDESCY,str,FaceCols);
        unselscene=lo;
        }  
      }                         // End of all scene functions
      
                                                // LINKS
    else                        //  Mouse not in a scene - must be a link
      {
      if ( (lo=retlink(x,y))!=NOLINK) // Return link if there's one under mouse
        {
        if (buts>0) 
          { 
          if (buttonpress==2) 
            {
            deletelink(curworld, lo);   // Delete a link
            lo=NOLINK; olink=lo;
            Game.Draw();
            }
          else  // allow user to set exactly where in the two scenes the link goes from and to.
            {
            saved=TRUE;
            expandlink(curworld,lo); 
            drawall();
            Cur.Goto(x,y);
            } 
          } 
        else if (olink!=lo) /* Only let this code execute once per time in link */
          {   /* User must leave link and come back for olink to be different */
          Game.CleanDrawLink(curworld,Game.scns[curworld].links[lo].ToScene,acol); /* highlight the link */
          olink=lo; /* Set current link so prog will only highlight link once per time in the link*/
          }
        }
// CREATE LINK
      else if ( (buts>0)&&IN(WX,WORLDTOP,WX+WLEN,WORLDTOP+WWID)
              &&(curworld != NOTUSED) )
        {     // if Mouse button pressed, in world box, and a scene is selected
        Cur.Off();
        inbox=TRUE; 
        line=curworld;    /* Origin of new link is always selected world */
        Cur.Limit(WX,WORLDTOP,WX+WLEN,WORLDTOP+WWID);
        xor=1;
        Line (Game.scns[line].Pos.x+2,Game.scns[line].Pos.y+2,x,y,acol); /* draw line */
        xor=0;
        ox=x;oy=y; // Set up previous variables so line can be erased
        }
      if ((lo==NOLINK)&(olink!=NOLINK))  /* If a link was selected, but cursor has moved away change its color back to bcol */
        {
        Game.CleanDrawLink(curworld,Game.scns[curworld].links[olink].ToScene,bcol);
        olink=NOLINK;
        }
      }
    ognum=lo; 
    /* Reset (or set) ognum to insure that some link code is only 'done' once per time selected */
    }

// DESCRIPTION - edit selected box's (yellow) description
  if ( (buts>0)&&(curworld!=NOTUSED)&&IN(TEXTX,SELDESCY,315,SELDESCY+9))
    {
    if (unselscene!=NOTUSED)
      {
      Cur.Off();
      GGet(TEXTX+1,SELDESCY+1,FaceCols[BLACK],FaceCols[GREEN],Game.scns[curworld].desc,14);
      if (curworld==unselscene)
        {
        sprintf(str,"%-14s",Game.scns[unselscene].desc);
        BoxWithText(TEXTX,NORMDESCY,str,FaceCols);
        }
      Cur.On();
      }
    }

// DESCRIPTION - edit unselected box's (blue) description
  if ( (buts>0)&&IN(TEXTX,NORMDESCY,315,NORMDESCY+9))
    {
    if (unselscene!=NOTUSED)
      {
      Cur.Off();
      GGet(TEXTX+1,NORMDESCY+1,FaceCols[BLACK],FaceCols[GREEN],Game.scns[unselscene].desc,14);
      if (curworld==unselscene)
        {
        sprintf(str,"%-14s",Game.scns[curworld].desc);
        BoxWithText(TEXTX,SELDESCY,str,FaceCols);
        }
      Cur.On();
      }
    }
// SELECT - Choose or unchoose select button.
  if ((obuts==0)&(buts>0)&IN(SELECTX,SELECTY,SELECTX+36,SELECTY+10))
    {
    unpress(buttonpress);
    if (buttonpress !=1)
      {
      Cur.Off();
      seleb(SELECTX,SELECTY+1,HiCols);
      Cur.On();
      buttonpress=1;
      }
    else buttonpress=FALSE;
    }

// DELETE       button
  if ((obuts==0)&(buts>0)&IN(DELETEX,DELETEY,DELETEX+36,DELETEY+9))
    {
    if (buttonpress !=2)
      {
      Cur.Off();
      deleb(DELETEX,DELETEY+1,HiCols);
      Cur.On();
      buttonpress=2;
      }
    else { unpress(buttonpress); buttonpress=FALSE; }
    }

/* MENU      button */
  if (IN(MENUX,MENUY,MENUX+30,MENUY+9)) 
    {
    if ((buts>0)&(obuts==0))   /* press button */
      {
      Cur.Off();
      unpress(buttonpress);
      menub(MENUX,MENUY+1,HiCols);
      Cur.On();
      }
    if ((buts==0)&(obuts!=0))              /* goto menu when released */
      { 
      ognum=NOTUSED; 
      buttonpress=FALSE; 
      return (TRUE);
      }
    }

  if (IN(HELPX,HELPY,HELPX+30,HELPY+9))              /* HELP Button */
    {
    if ((buts>0)&(obuts==0))  /* press button */
      {
      Cur.Off();
      unpress(buttonpress);
      helpb(HELPX,HELPY+1,HiCols);
      Cur.On();
      }
    if ((buts==0)&(obuts !=0))  /* Redraw screen */
      {
      Cur.Off();
      TextMode();
      DisplayHelpFile("grat.hlp");
      Cur.Off();
      drawall();
      mouclearbut();
      Cur.Goto(HELPX+20,HELPY+5);
      Cur.On();
      }
    }

  // Create a scene if button pushed on nothing with no scene selected
  if ( (buttonpress==0)&&IN(WX+2,WORLDTOP+2,WX+WLEN-3,WORLDTOP+WWID-3)
     &&(olink==NOLINK)&&(inbox==FALSE)&&(buts>0)&&(obuts==0) )
    {
    Game.CreateScene(x,y);
    mouclearbut();
    }  

  ox=x;oy=y;obuts=buts;
  return (FALSE);  /* do NOT go back to menu */
  }

static void ConvWorld(void)
  {
  register int lo;
  char flag=0;

  do {
    flag=0;
    for (lo=0;lo<Game.NumScenes;lo++)
      if (Game.scns[lo].gametype!=NOSCENE)
        {
        if ( (Game.scns[lo].Pos.y<=WORLDTOP)||(Game.scns[lo].Pos.y+5>=WORLDTOP+WWID) )
          { flag=1; saved=0; }
        }
    if (flag)
      {
      for (lo=0;lo<Game.NumScenes;lo++)
        if (Game.scns[lo].gametype!=NOSCENE)
          {
          if (Game.scns[lo].Pos.y<WORLDTOP+WWID/2) Game.scns[lo].Pos.y++;
          else Game.scns[lo].Pos.y--;
          }
      }
    } while(flag);
  }

static void drawall(void)
  {
  int lo;
  boolean m=((boolean) Cur.Show);
  if (m) Cur.Off();
  GraphMode();
  InitStartCols(colors);
  SetCols(colors,FaceCols);
  SetAllPalTo(&colors[BKGCOL]);
  bcol=FaceCols[RED1];
  acol=FaceCols[GREEN];
  speccol=FaceCols[GREY4];

  drawsurface(WX-3,WORLDTOP-27,WLEN+6,WWID+30,FaceCols);
  Cur.Limit(2,1,318,198);
  BoxWithText(TEXTX,NORMDESCY,"              ",FaceCols);
  BoxWithText(TEXTX,SELDESCY,"              ",FaceCols);
  BoxFill(TEXTX,NORMDESCY,TEXTX+112,NORMDESCY+8,FaceCols[GREY2]);
  BoxFill(TEXTX,SELDESCY,TEXTX+112,SELDESCY+8,FaceCols[GREY2]);
  GWrite(TEXTX-99,SELDESCY+1,FaceCols[RED1],"  Selected  ");
  GWrite(TEXTX-99,NORMDESCY+1,FaceCols[RED1],"Last Touched");
  Game.Draw();

  menub(MENUX,MENUY,FaceCols);
  helpb(HELPX,HELPY,FaceCols);
  deleb(DELETEX,DELETEY,FaceCols);  
  Cur.Goto(MENUX+20,MENUY+5);
  FadeTo(colors);
  if (m) Cur.On();
  }

static void deletelink(uint scene,int link)
  {
  Game.scns[scene].links[link].Clear();
  saved=FALSE;
  }
  
static uint createlink(uint from,uint to)
  {
  register int lo;
  
  if ((Game.scns[from].gametype==ENDTYPE)||(Game.scns[to].gametype==STARTTYPE)) return(NOLINK);

  saved=FALSE;
  for (lo=0;lo<MAXSCENELINKS;lo++)
    {
    if (Game.scns[from].links[lo].ToScene==NOSCENE)
      {
      Game.scns[from].links[lo].ToScene=to;
      return(lo);
      }
    }
  return(NOLINK);
  }
  
static void movescene(uint scnum)
  {
  int  x,y,buts,ox,oy,x2,y2;
  uint linkto;
  register int lo,ll;
    
  Cur.Limit(WX+3,WORLDTOP+3,WX+WLEN-4,WORLDTOP+WWID-4);
  ox=(x2=Game.scns[scnum].Pos.x)+2; oy=(y2=Game.scns[scnum].Pos.y)+2;
  Cur.Goto(ox,oy);
  moustats(&x,&y,&buts);
  Game.scns[scnum].Pos.x=x-2;  Game.scns[scnum].Pos.y=y-2;
  while (buts==2)
    {
    if ( (x!=ox)||(y!=oy) )
      {
      Cur.Off();
      for (ll=0;ll<Game.NumScenes;ll++)
        if ((Game.scns[ll].gametype!=NOSCENE)&&(scnum!=ll)) Game.DrawLink(scnum,ll,WORLDCOL);

      BoxFill(ox-2,oy-2,ox+3,oy+3,WORLDCOL);
      Game.scns[scnum].Pos.x=x-2;  Game.scns[scnum].Pos.y=y-2;
      for (ll=0;ll<Game.NumScenes;ll++)
        if ( (Game.scns[ll].gametype!=NOSCENE)&&(ll!=curworld) )
          {
          for (lo=0;lo<MAXSCENELINKS;lo++)
            if ((linkto=Game.scns[ll].links[lo].ToScene)!=NOSCENE) // link active - draw it.
              Game.DrawLink(linkto,ll,BACKLINK);
          }
      if ((curworld!=NOTUSED )&&(Game.scns[curworld].gametype!=NOSCENE))
        {
        for (lo=0;lo<MAXSCENELINKS;lo++)
          if ((linkto=Game.scns[curworld].links[lo].ToScene)!=NOSCENE) // link active - draw it.
            Game.DrawLink(curworld,linkto,bcol);
        }

      for (ll=0;ll<Game.NumScenes;ll++)
        if (Game.scns[ll].gametype!=NOSCENE)
          Game.DrawScene(ll,( (ll==curworld) ? acol : ((Game.scns[ll].gametype==ADVTYPE) ? bcol : speccol) ) );

      Cur.On();
      ox=x; oy=y;
      }    
    moustats(&x,&y,&buts);
    }

  Game.scns[scnum].Pos.x=SIZEX;  Game.scns[scnum].Pos.y=SIZEY;   // Temporarily remove from map
  if (Game.SceneAt(x,y)==NOSCENE) { Game.scns[scnum].Pos.x=x-2; Game.scns[scnum].Pos.y=y-2; saved=0; }
  else { Game.scns[scnum].Pos.x=x2; Game.scns[scnum].Pos.y=y2; }

  Game.Draw();
  Cur.Limit(0,0,SIZEX-1,SIZEY-1);
  }

static void expandscene(uint scnum)
  {
  switch(Game.scns[scnum].gametype)
    {
    case NOSCENE:
      break;
    case STARTTYPE:
      expandstart(scnum);
      break;
    case ENDTYPE:
      expandend(scnum);
      break;
    case ADVTYPE:
      expandadv(scnum);
      break;
    default:
      printf("NO KNOWN SCENE!");
      break;
    }
  GraphMode();
  drawall();
  Cur.Goto(Game.scns[scnum].Pos.x+2,Game.scns[scnum].Pos.y+2);
  Cur.On();
  }

static char *StripDir(char *str)
  {
  int len;
  for (len=strlen(str);len>=0;len--) if ((str[len]=='\\')||(str[len]==':')) break;
  return(&str[len+1]);
  }

static void expandstart(uint scnum)  // Allows user to enter in FILE NAMES
  {
  register int l;
  static int choice=1;
  int mx,my,mbuts;
  int attr;
  char ans[MAXFILENAMELEN];

  const char qs[][41] = {
                   "Enter Title Graphic:",
                   "Enter Title Song:",
                   "Enter Menu Graphic:",
                   "Enter Menu Song:",
                   "Enter Storyline Graphic:",
                   "Enter Storyline Song:",
                   "Game Instructions Graphic:",
                   "Enter Credits Graphic:",
                   "High Score Background:",
                   "Load Game Background:",
                   "Save Game Background:",
                   "Enter Demo Recording:",
                   };
  const char *exts[] = { GraphicExts,"*.cmf\0",GraphicExts,"*.cmf\0",GraphicExts,"*.cmf\0",GraphicExts,GraphicExts,GraphicExts,GraphicExts,GraphicExts,"*.rec\0"};
 
  for (l=0;l<MAXFILENAMELEN;l++) ans[l]=0;   // initalize answer to nothing

  Cur.Off();
  moustats(&mx,&my,&mbuts);
  TextMode();
  attr=openmenu(2,5,40,15);
  writestr(2,5,attr+TITLECOL,Game.scns[scnum].desc);
  writestr(2,7,attr+15,"Title Graphic");
  writestr(2,8,attr+15,"Title Song");
  writestr(2,9,attr+15,"Menu Graphic");
  writestr(2,10,attr+15,"Menu Song");
  writestr(2,11,attr+15,"Storyline Graphic");
  writestr(2,12,attr+15,"Storyline Song");
  writestr(2,13,attr+15,"Game Instructions");
  writestr(2,14,attr+15,"Beginning Credits");
  writestr(2,15,attr+15,"High Score Backdrop");
  writestr(2,16,attr+15,"Load Game Background");
  writestr(2,17,attr+15,"Save Game Background");
  writestr(2,18,attr+15,"Demo Recording");
  writestr(2,19,attr+15,"Back To Edit Screen");

  do
    {
    for (l=0;l<12;l++)
      {
      writestr(28,7+l,attr+15,"             ");
      writestr(28,7+l,attr+15,Game.Files(Game.scns[scnum].Files[l]));
      }
  
    choice=vertmenu(2,7,40,13,choice);
    
    if (choice==0) choice=13;
    if (choice<13)
      {
      ans[0]=0;
      if(getfname(3,11,qs[choice-1],exts[choice-1],ans)==0) ans[0]=0; // No file chosen so clear buffer
      else DealWithDirectories(ans);
      if (ans[0]!=0)
        {
        Game.scns[scnum].Files[choice-1] = Game.Files.Add(StripDir(ans));
        saved=False;
        }
      }
    choice++;
    } while (choice!=14);
  }

static void expandend(uint scnum)  // Allows user to enter in FILE NAMES.
  {
  register int l;
  static int choice=1;
  int mx,my,mbuts;
  int attr;
  char ans[MAXFILENAMELEN];

  const char qs[][41] = {"Enter Game Won Graphic:",
                         "Enter Game Won Song:",
                         "Enter Game Lost Graphic:",
                         "Enter Game Lost Song:" };

  const char *exts[4] = { GraphicExts,"*.cmf\0",GraphicExts,"*.cmf\0"};
  const char fpaths[][5]={ "","","","" };
 
  for (l=0;l<MAXFILENAMELEN;l++) ans[l]=0;   // initalize answer to nothing

  Cur.Off();
  moustats(&mx,&my,&mbuts);
  TextMode();
  attr=openmenu(2,5,35,7);
  writestr(2,5,attr+TITLECOL,Game.scns[scnum].desc);
  writestr(2,7,attr+15,"Game Won Graphic");
  writestr(2,8,attr+15,"Game Won Song");
  writestr(2,9,attr+15,"Game Lost Graphic");
  writestr(2,10,attr+15,"Game Lost Song");
  writestr(2,11,attr+15,"Back To Edit Screen  ");

  do
    {
    for (l=0;l<4;l++)
      {
      writestr(23,7+l,attr+15,"             ");
      writestr(23,7+l,attr+15,Game.Files(Game.scns[scnum].Files[l]));
      }
    choice=vertmenu(2,7,35,5,choice);

    if (choice==0) choice=5;
    if (choice<5)
      {
      strcpy(ans,fpaths[choice-1]);
      if (getfname(10,10,qs[choice-1],exts[choice-1],ans)==0) ans[0]=0;
      else DealWithDirectories(ans);
      if (ans[0]!=0)
        {
        Game.scns[scnum].Files[choice-1] = Game.Files.Add(StripDir(ans));
        saved=False;
        }
      }
    choice++;
    } while (choice!=6);
  closemenu(2,5,40,7);
  }

static void expandadv(uint scnum)  // Allows user to enter in FILE NAMES.
  {
  register int l;
  static int choice=1;
  int mx,my,mbuts;
  int attr;
  char ans[MAXFILENAMELEN];

  const char qs[][41] = { "Enter Palette:","Enter Map Name","Enter Background Block Set:",
                   "Enter Monster File:","Enter Monster Block Set:", "Enter Character Set:",
                   "Enter Character Block Set:","Enter Sound Set:","Enter SB Music Title:","Enter Level Intro:","Enter Level Ending:"};
  const char *exts[] = { "*.pal\0","*.map\0","*.bbl\0","*.mon\0","*.mbl\0","*.chr\0","*.cbl\0","*.snd\0","*.cmf\0",GraphicExts,GraphicExts};
  const char fpaths[][5]={ "","","","","","","","","","",""};
 
  for (l=0;l<MAXFILENAMELEN;l++) ans[l]=0;   // initalize answer to nothing

  strcpy(GameArea,WorkDir);

  Cur.Off();
  moustats(&mx,&my,&mbuts);
  TextMode();
  attr=openmenu(2,5,45,15);
  writestr(2,5,attr+TITLECOL,Game.scns[scnum].desc);
  writestr(2,7,attr+15,"Palette");  
  writestr(2,8,attr+15,"Map");
  writestr(2,9,attr+15,"Background Block Set");
  writestr(2,10,attr+15,"Monster");
  writestr(2,11,attr+15,"Monster Block Set");
  writestr(2,12,attr+15,"Character");
  writestr(2,13,attr+15,"Character Block Set");
  writestr(2,14,attr+15,"Sound Set");
  writestr(2,15,attr+15,"Sound Blaster Music");
  writestr(2,16,attr+15,"Level Intro Graphic");
  writestr(2,17,attr+15,"Level Exit  Graphic");
  writestr(2,18,attr+15,"Change Scrolling Info");
  writestr(2,19,attr+15,"Back To Edit Screen");
  do
    {
    for (l=0;l<11;l++)
      {
      writestr(30,7+l,attr+15,"             ");
      writestr(30,7+l,attr+15,Game.Files(Game.scns[scnum].Files[l]));
      }
    choice=vertmenu(2,7,45,13,choice);
    if (choice==0) choice=14;
    if (choice<13)
      {
      saved=FALSE;
      if (choice==12) DoScroll(scnum);
      else
        {
        strcpy(ans,fpaths[choice-1]);
        if (getfname(10,10,qs[choice-1],exts[choice-1],ans)==0) ans[0]=0;
        else DealWithDirectories(ans);
        if (ans[0]!=0)
          {
          Game.scns[scnum].Files[choice-1] = Game.Files.Add(StripDir(ans));
          saved=False;
          }
        }
      }
    choice++;
    } while (choice!=14);

  closemenu(2,5,45,15);
  }

void DealWithDirectories(char *ans)
  {
  char Name[15]="";
  ParseFileName(ans,Name,DummyStr);  // Parse file name removes extension.

  if ((Name[0]!=0)&&(strcmp(WorkDir,GameArea)!=0))
    {
    int result = toupper(errorbox("All of a game's files must be in its game area.",
                                  "[A]bort, or (C)opy this file into the game area?")&255);
    if (result=='C')
      {
      MakeFileName(DummyStr,GameArea,Name,FileExt(ans));
      CopyFile(ans,DummyStr);            // Copy it over.
      }
    else ans[0]=0;
    strcpy (WorkDir,GameArea);
    }
  }


static void MakeScrollDir(char *out, const char *prefix, int scroll)
  {
  if (scroll==0)          sprintf(out,"%sdisabled!                         ",prefix);
  else if (scroll==0xFF)  sprintf(out,"%swhen the character is at the edge.",prefix);
  //else if (scroll<20)     sprintf(out,"%s%d pixels per second.               ",prefix,scroll);
  }

static void DoScroll(int scnum)
  {
  int choice=1;
  unsigned char tempc;
  char attr=0;
  char w[2000];
  char temp[61];

  attr=openmenu(10,7,50,7,w);
  writestr (11,8,attr+14,"Scrolling");
  writestr (11,13,attr+7,"Done");
    
  do
    {
    MakeScrollDir(temp,"   UP    :",Game.scns[scnum].Scroll.up);
    writestr (10,9,attr+7,temp);
    MakeScrollDir(temp,"   DOWN  :",Game.scns[scnum].Scroll.down);
    writestr (10,10,attr+7,temp);
    MakeScrollDir(temp,"   LEFT  :",Game.scns[scnum].Scroll.left);
    writestr (10,11,attr+7,temp);
    MakeScrollDir(temp,"   RIGHT :",Game.scns[scnum].Scroll.right);
    writestr (10,12,attr+7,temp);
    choice=vertmenu(10,9,50,5,choice);
    if (choice==0) choice=5;
    if (choice<5)
      {
      switch (choice)
        {
        case 1:
          ScrollChange(&Game.scns[scnum].Scroll.up);
          break;
        case 2:
          ScrollChange(&Game.scns[scnum].Scroll.down);
          break;
        case 3:
          ScrollChange(&Game.scns[scnum].Scroll.left);
          break;
        case 4:
          ScrollChange(&Game.scns[scnum].Scroll.right);
          break;
        }
      }
    choice++;
    } while (choice!=6);
  closemenu(10,7,50,7,w);
  }

static void ScrollChange(unsigned char *sinfo)
  {
  int choice=1;
  //int tempc=0;
  //char temp[]={0,0,0,0,0};
  char attr=0;
  char w[1500];

  attr=openmenu(15,17,50,2,w);
  writestr (15,17,attr+7,"Character at edge of screen.");
  writestr (15,18,attr+7,"Scrolling in this direction not allowed.");
  //writestr (15,19,attr+7,"Scroll at 1 to 20 pixels per second.");

  choice=vertmenu(15,17,50,2,choice);

  switch (choice)
    {
    case 1:
      *sinfo=0xFF;
      break;
    case 2:
      *sinfo=0;
      break;
/*  case 3:
      if (qwindow(16,12,2,"Enter number of pixels to scroll per second: ",temp))
        {
        sscanf(temp,"%d",&tempc);
        if ((tempc<=20)&&(tempc>1)) *sinfo=tempc; 
        }
      break;
*/
    }
  closemenu(15,17,50,2,w);
  }
   
static int unpress(int butnum)
  {
  int m;

  m=Cur.Show;
  if (m) Cur.Off();

  if (butnum==2) deleb(DELETEX,DELETEY,FaceCols); // unpress Delete button
  if (butnum==1)
    {
    if (curworld!=NOTUSED)
      {
      Game.DrawScene(curworld,bcol);
      BoxFill(TEXTX,SELDESCY,TEXTX+112,SELDESCY+8,WORLDCOL);
      Game.DrawSceneLinks(curworld,BACKLINK);
      if (curworld>1) Game.DrawScene(curworld,bcol);   // User defined scene
      else Game.DrawScene(curworld,speccol);           // special start or end scene
      }
    curworld=NOTUSED;
    seleb(SELECTX,SELECTY,FaceCols);
    }

  if (m) Cur.On();
  return(FALSE);
  }

static uint retlink(int x, int y)
  {
  register int lo;
  float slope=0;
  uint ow=0;
  int liney=0,cury=0;
  int dx=0,dy=0;
  int yerr;

  if (curworld!=NOTUSED)  // Only looking for links in currently selected scene
    {
    for (lo=0;lo<MAXSCENELINKS;lo++)  // Check every link
      {
      ow=Game.scns[curworld].links[lo].ToScene;
      if (ow!=NOSCENE)  // If link is in use, check to see if mouse is pointing at it
        {
        dx=Game.scns[curworld].Pos.x-Game.scns[ow].Pos.x;
        dy=Game.scns[curworld].Pos.y-Game.scns[ow].Pos.y;
        if (((dx>0)&(x<=Game.scns[curworld].Pos.x+2)&(x>=Game.scns[ow].Pos.x+2))|
            ((dx<0)&(x>=Game.scns[curworld].Pos.x+2)&(x<=Game.scns[ow].Pos.x+2)))
          {
          slope= (float) dy/dx;
          liney=(int) (slope*((float)(Game.scns[curworld].Pos.x+2-x)));
          cury = (Game.scns[curworld].Pos.y+2)-y;
          if (slope>0) yerr= ((int) slope)+1;
          else yerr= ( (int) (slope*(-1)))+1;          

          if (abs(liney-cury)<yerr)
            return (lo);
          }
        if (dx==0)          // vertical line
          {
          if (x==Game.scns[ow].Pos.x)  // if x is correct...
            {                     // and y is between the two, then return it
            if ((dy>0)&(y<Game.scns[curworld].Pos.y+2)&(y>Game.scns[ow].Pos.y+2)) return(lo);
            if ((dy<0)&(y>Game.scns[curworld].Pos.y+2)&(y<Game.scns[ow].Pos.y+2)) return(lo);
            }
          }
        }
      }
    }
  return(NOLINK);
  }

  // Cleanup is executed when the program ends in any way other then an abort
static void cleanup(void) 
  {
  if (Game.scns!=NULL) delete Game.scns;
  Time.TurnOff();
  }

static void initalldata(void)
  {
  Game.Init();
  saved = TRUE;
  }

static void deleb(int x,int y, unsigned char col[4])
  {
  static char delar[] = "         UUUUUUU@UUUUUUUTõÿu_÷ÿÔW]Õu]UuuUW]ýu_ÕuUW]Õu]UuuU—õÿßõuÖUUUUUUUT)UUUUUUUhªªªªªªª€";
  draw4dat(x,y,delar,35,10,col);
  }

static void seleb(int x,int y, unsigned char col[4])
  {
  static char selar[] = "         UUUUUUU@UUUUUUUTõÿu_õõÿÔWUÕu]W]]UUõýu_×U]UU]Õu]W]]U—õÿßõõ]VUUUUUUUT)UUUUUUUhªªªªªªª€";
  draw4dat(x,y,selar,35,10,col);
  }

uint StrInBuf(char *str,char *Buf,int Blen)
  {
  for (uint idx=0;idx<Blen;idx++)
    {
    if (str[0]==Buf[idx]) if (strcmpi(str,&Buf[idx])==0) return(idx);
    }
  return(0xFFFF);
  }

void GameClass::CleanDrawLink(int from,int to,Pixel col)
  {
  Pixel col1,col2;

  boolean m=((boolean)Cur.Show);

  if (m) Cur.Off();
  col1=GetCol(scns[from].Pos.x,scns[from].Pos.y);
  col2=GetCol(scns[to].Pos.x,scns[to].Pos.y);
  DrawLink(from,to,col);
  DrawScene(from,col1);
  DrawScene(to,col2);
  if (m) Cur.On();
  }

void GameClass::DrawLink(int from,int to,Pixel col)
  {
  if (from>to) swap(from,to);
  Line(scns[from].Pos.x+2,scns[from].Pos.y+2,scns[to].Pos.x+2,scns[to].Pos.y+2,col);
  }

void GameClass::DrawScene(uint scene, Pixel col)
  {
  int m;
  m=Cur.Show;
  if (m) Cur.Off();
  BoxFill(scns[scene].Pos.x,scns[scene].Pos.y,scns[scene].Pos.x+4,scns[scene].Pos.y+4,col);
  if (m) Cur.On();
  }


void GameClass::DrawSceneLinks(uint snum,Pixel col)
  { 
  int    lo;
  boolean m=((boolean)Cur.Show);

  if (m) Cur.Off();

  for (lo=0;lo<MAXSCENELINKS;lo++)       // Draw new lines
    {
    if (scns[snum].links[lo].ToScene!=NOSCENE)      // Link active - draw it.
      {
      uint Dest=scns[snum].links[lo].ToScene;
      Game.DrawLink(snum,Dest,col);
      Game.DrawScene(Dest,SceneColor(Dest));
      }
    }
  Game.DrawScene(snum,SceneColor(snum));
  if (m) Cur.On();
  }



void GameClass::Draw(void)
  {
  int lo;
  char str[15];

  boolean m=((boolean)Cur.Show);

  if (m) Cur.Off();

  Line(WX-1,WORLDTOP,WX-1,WORLDTOP+WWID,FaceCols[GREY3]);  // Erase the World
  Line(WX-1,WORLDTOP-1,WX+WLEN,WORLDTOP-1,FaceCols[GREY4]);
  BoxFill(WX,WORLDTOP,WX+WLEN,WORLDTOP+WWID,WORLDCOL);
  for (lo=0;lo<Game.NumScenes;lo++)                      // Draw grey links
    {
    if ((scns[lo].gametype!=NOSCENE)&&(lo!=curworld)) DrawSceneLinks(lo,BACKLINK);
    }

  if ((curworld!=NOTUSED )&&(Game.scns[curworld].gametype!=NOSCENE)) DrawSceneLinks(curworld,bcol);

  for (lo=0;lo<Game.NumScenes;lo++)                      // Draw scene boxes
    {
    if (Game.scns[lo].gametype!=NOSCENE)  Game.DrawScene(lo,SceneColor(lo));
    }
                                   // Draw selected world and info
  if ((curworld!=NOTUSED )&&(Game.scns[curworld].gametype!=NOSCENE))
    {
    sprintf(str,"%-14s",scns[curworld].desc);
    BoxWithText(TEXTX,SELDESCY,str,FaceCols);
    seleb(SELECTX,SELECTY+1,HiCols);
    }
  else seleb(SELECTX,SELECTY,FaceCols);
  if (m) Cur.On();
  }


void GameClass::EraseScene(uint scene)
  {
  register int lnum,snum; 

  if ( (scene!=NOTUSED)&&(scns[scene].gametype>=ADVTYPE)) // is Scene erasable?
    {
    for (snum=0;snum<NumScenes;snum++)  // Erase all links TO this scene.
      for (lnum=0;lnum<MAXSCENELINKS;lnum++)
        if (Game.scns[snum].links[lnum].ToScene == scene) Game.scns[snum].links[lnum].Clear();
    scns[scene].Clear();
    if (curworld==scene) curworld=NOTUSED;
    saved=FALSE;
    }
  }

uint GameClass::SceneAt(int x,int y)
  {
  register uint lo;
  
  for (lo=0;lo<Game.NumScenes;lo++)
    if ( (Game.scns[lo].gametype !=NOSCENE)&&(IN(Game.scns[lo].Pos.x-2,Game.scns[lo].Pos.y-2,Game.scns[lo].Pos.x+7,Game.scns[lo].Pos.y+7)) ) return (lo);
  return(NOSCENE);
  }

uint GameClass::CreateScene(int x,int y)                 // make a scene
  {
  register int lo;

  for (lo=0;lo<MaxScenes;lo++)
    {
    if (Game.scns[lo].gametype==NOSCENE)
      {
      saved             = FALSE;
      scns[lo].gametype = ADVTYPE;
      scns[lo].Pos.x    = x-2;
      scns[lo].Pos.y    = y-2;
      DrawScene(lo,bcol);
      if (lo>=NumScenes) NumScenes=lo+1;
      return(lo); 
      }
    }
  return(NOSCENE);
  }


void GameClass::Init(void)
  {
  for (int lx=0;lx<MaxScenes;lx++) scns[lx].Clear();
  scns[0].Pos.x    = (WLEN/2)+WX-3;
  scns[0].Pos.y    = WORLDTOP+7;
  scns[0].gametype = STARTTYPE;
  strcpy(scns[0].desc,"Title Screen");
  scns[1].Pos.x    = (WLEN/2)+WX-3;
  scns[1].Pos.y    = WORLDTOP+WWID-10;
  scns[1].gametype = ENDTYPE;
  strcpy(scns[1].desc,"Game Won");
  NumScenes=2;
  }

void GameClass::FileConverter(int oldsc,int oldfi,int newsc,int newfi,char *Ext,OLDintegratorstruct *old)
  {
  char Temp[MAXFILENAMELEN];

  if (old[oldsc].fnames[oldfi][0] == 0) scns[newsc].Files[newfi] = 0;
  else
    {
    strcpy(Temp,strlwr(old[oldsc].fnames[oldfi]));
    strcat(Temp,Ext);
    scns[newsc].Files[newfi] = Files.Add(Temp);
    }
  }

boolean SceneFiles::Load(FILE *fp)
  {
  uint Amt;
  fread(&Amt,sizeof(uint),1,fp);        // Read the buffer length
  ulongi Total = Amt+10000;
  if (Total>0xFFF0) Total=0xFFF0;
  if (Total>MaxMem)
    {
    if (Mem)  delete Mem;
    MaxMem  = Total;
    Mem     = new char [MaxMem];
    }
  fread(Mem,sizeof(char),Amt,fp);       // Read the buffer.
  FreePtr=Amt;
  return(TRUE);
  }

boolean GameClass::Load(FILE *fp)
  {
  GratorHeader   Head;
  DataFileHeader Id;

  Id.Load(fp);
  if (!Id.Validate()) // Load an old .gam file up.
    {
    char exts[][5] = { ".pal",".map",".bbl",".mon",".mbl",".chr",".cbl",".snd",".cmf"};
    char Temp[MAXFILENAMELEN];
    rewind(fp);
    OLDintegratorstruct *old = new OLDintegratorstruct [OLDMAXSCENES];
    if (old==NULL) return(FALSE);
    NumScenes=fread(old,sizeof(OLDintegratorstruct),OLDMAXSCENES,fp);
    Files.Clear();

    // Convert the start scene.
    // First 2 #s are the OLD scene and file idx, 2nd 2 #s are the NEW ones.
    FileConverter(0,0,0,2,".bkd",old);  // set the Menu gifs =
    FileConverter(0,1,0,4,".txt",old);  // set the storyline gifs =
    FileConverter(0,2,0,6,".txt",old);  // set the game instruction =
    FileConverter(0,3,0,7,".txt",old);  // set the credits text =
    FileConverter(0,4,0,3,".cmf",old);  // set the menu song =
    scns[0]=old[0];

    // Convert the End scene.
    FileConverter(1,0,1,0,".bkd",old);  // set the game won gif =
    FileConverter(1,3,1,1,".cmf",old);  // set the end .cmf
   scns[1]=old[1];

    for (int i=2;i<NumScenes;i++)
      {
      scns[i]=old[i];
      for (int j=0;j<OLDLASTFNAME-1;j++) FileConverter(i,j,i,j,exts[j],old);

      }
    for (i=Game.NumScenes;i<Game.MaxScenes;i++) scns[i].Clear();
    ConvWorld();
    return(TRUE);
    }
  fread((void *) &Head,sizeof(GratorHeader),1,fp);
  NumScenes = Head.NumScenes;
  MaxScenes = NumScenes+EXTRASCENEBUFFER;
  if (scns!=NULL) delete scns;
  if ((scns = new Scene [MaxScenes])==NULL) return(FALSE);
  if (fread(scns,sizeof(Scene),NumScenes,fp) != NumScenes) return(FALSE);
  if (!Files.Load(fp)) return(FALSE);
  for (int i=NumScenes;i<MaxScenes;i++) scns[i].Clear();
  return(TRUE);
  }

boolean GameClass::Save(FILE *fp)
  {
  GratorHeader   Head;
  DataFileHeader Id;

#ifdef CRIPPLEWARE
  StdCrippleMsg();
#else
  Id.FileType = GamFile;
  Id.Save(fp);
  Head.NumScenes     = NumScenes;
  Head.FilesPerScene = SCENEFILES;
  Head.LinksPerScene = MAXSCENELINKS;

  fwrite((void *) &Head,sizeof(GratorHeader),1,fp);
  if (fwrite(scns,sizeof(Scene),NumScenes,fp) != NumScenes) return(FALSE);
  if (!Files.Save(fp)) return(FALSE);
#endif
  return(TRUE);
  }

void GameClass::Optimize(void) {}

static int ForgetChanges(void)
  {
#ifdef CRIPPLEWARE
  return(TRUE);
#else
  char choice;
  choice = errorbox("Forget unsaved changes?","  (Y)es  (N)o")&0x00FF;
  if ( (choice!='Y') && (choice!='y') ) return(FALSE);
  saved=TRUE;
  return(TRUE);
#endif
  }

static char CopyFile(const char *From,const char *To)
  {
  char str[81];
  char worked=TRUE;
  char far *mem=NULL;
  uint startat=64512;
  uint BytesRead=0;
  FILE *fp_in,*fp_out;
  int attr=0;
  char *w = new char [1000];
  char size;

//  OldCritError=(char interrupt (*)(...)) getvect(0x24);
//  setvect(0x24,(void interrupt (*)(...)) NewCritError);
  
  size = strlen(From)+4;
  attr = openmenu((80-size)/2,10,size,3,w);
  writestr((80-size)/2,10,attr+4," Copying...");
  writestr((80-size)/2+1,12,attr+14,From);

  if ((fp_in=fopen(From,"rb")) == NULL)
    {
    if (CritVal==100)
      {
      sprintf(str,"File (%s) is missing!",From);
      errorbox(str,"(C)ontinue");
      }
    CritVal=100;
    worked=FALSE;
    }
  else if ((fp_out=fopen(To,"wb"))==NULL)
    { 
    if (CritVal==100) errorbox("I cannot open the destination file!","(C)ontinue");
    CritVal=100;
    worked=FALSE; 
    fclose(fp_in);
    }
  else
    {  
    while((mem==NULL)&&(worked!=FALSE)) 
      { 
      startat-=512; 
      if (startat>1024) mem=(char far *) farmalloc(startat); 
      else { errorbox("Trouble copying file","(C)ontinue"); worked=FALSE; }
      }
    if (worked==TRUE) do
      {
      BytesRead=fread(mem,sizeof(char),(size_t) startat,fp_in);
      if (fwrite(mem,sizeof(char),(size_t) BytesRead,fp_out)<BytesRead)
        {
        errorbox("Out of Disk space!","(C)ontinue");
        BytesRead=startat-1;
        worked=FALSE;
        }
      } while (BytesRead==startat); // if not equal, an EOF has occured. 

    fclose(fp_in);
    fclose(fp_out);
    farfree(mem);
    }
  if (worked) PauseTimeKey(1);
  closemenu((80-size)/2,10,size,3,w);

//  setvect(0x24,(void interrupt (*)(...)) OldCritError);
  delete w;
  return(worked);
  }

