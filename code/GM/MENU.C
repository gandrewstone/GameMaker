//------------------------------------------------------------------
// Game Maker Menu Program              by Ollie and Andy Stone    |
// Commercial Version                                              |
// Last Edited:  7-27-92  (Andy Stone)                             |
//------------------------------------------------------------------

#include <process.h>
#include <stdio.h>
#include <bios.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#ifdef CHKREG
#include <io.h>
#endif

#include "gen.h"
#include "gmgen.h"
#include "mousefn.h"
//#include "jstick.h"
#include "scrnrout.h"
#include "windio.h"
#include "palette.h"
#include "bytenums.h"
#define C(x,y) (x+(y*16))

#define DBGPRT(x) printf(x)

static void dodesign(char ch);
static void gmmain  (char ch);

QuitCodes main(int argc,char *argv[])
  {
  int ch;

#ifdef CHKREG
  if ((argc < 3) || (strcmp(argv[1],OKMSG)!=0))
    writestr(19,20,14,"This program must be run under Game-Maker!");
  else
    {
#else
    {
#endif
    if (initmouse()==FALSE) return(quit);  // Can't happen, 'cause mouse inited in gm.
    SetTextColors();
    mouclearbut();

    QuitCodes Prog=argv[2][0]-'A';

    if ((Prog>=palchos)&&(Prog<=grator)) { dodesign(Prog-palchos+1); ch=2; }
    else if (Prog==utility)  ch=3;
    else if (Prog==playgame) ch=1;
    else ch=5;
    gmmain(ch);
    clrbox(0,0,79,24,7);
    }
  return(quit);
  }

static void gmmain(char ch)
  {  
  if ((ch<1)||(ch>6)) ch=1;
  do
    {
    writestr(0,0,C(15,4),GMTOPBAR);
    writestr(0,1,C(15,1),"     Play        Design      Utilities      About        Quit         Help      ");
    ch=horizmenu(6,ch,1, 0,14,27,40,53,66,80);
    mouclearbut();
    switch (ch)
      {
      case 1:
        exit(playgame);
        //execl(Progs[9],Progs[9],VidBoard,OKMSG,NULL);
        break;
      case 2:
        dodesign(1);
        break;
      case 3:
        exit(utility);
        //execl(Progs[0],Progs[0],OKMSG,NULL);
        break;
      case 4:
        DisplayHelpFile("about.hlp");
        break;
      case 6:
        DisplayHelpFile("gmmain.hlp");
        break;
      }
    } while ((ch!=5)&&(ch!=0));
  exit(quit);
  }

static void dodesign(char ch)
  {
  if ((ch<1)||(ch>10)) ch=1;
  do
    {
    writestr(0,0,C(15,4),GMTOPBAR);
    writestr(0,1,C(15,1),"  Palette  Block  Monster  Map  Character  Image  Sound  Integrator  Main  Help ");
    ch=horizmenu(10,ch,1,0,10,17,26,31, 42,49,56,68,74,80);
    if ((ch>0)&&(ch<9)) exit(ch+1);
    if (ch==10) DisplayHelpFile("design.hlp");
    } while ((ch!=9)&&(ch!=0));
  mouclearbut();
  }



