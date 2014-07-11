/*-------------------------------------------------------------------------*/
/* jstickc.c    Joy Stick Interface routines                               */
/*                                                                         */
/* Author:Peter Savage                   Created:02-Jan-1992               */
/* jan 7,1993 - change Readjoystick to unsigned int    -- Andy Stone       */
/*-------------------------------------------------------------------------*/

#include <dos.h>
#include <stdio.h>
#include <bios.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

#include "jstick.h"
//#include "sound.h"

#define  TRUE	  1
#define  FALSE    0
#define  SLEEPTIME 5

static void InitConfigData(ConfigStruct *cs);

char joyinstall=FALSE;

/*****************************************************************************/
/*                                                                           */
/*      GetJoyPos function returns a joy stick position from 0 to 8,         */
/*      as well as any of the joystick buttons that are being pushed.        */
/*                                                                           */
/*      This function requires that the joystruct structure has been         */
/*      loaded with this machies joy stick X and Y coordinates               */
/*                                                                           */
/*                        Joy Stick Position Map                             */
/*                                 1                                         */
/*                              8     2                                      */
/*                            7    0    3                                    */
/*                              6     4                                      */
/*                                 5                                         */
/*      Inputs/Outputs:                                                      */
/*         pos    - Pointer to a byte to return the joy stick position       */
/*         button - Pointer to a byte to return the pushed buttons           */
/*         js     - Pointer to a loaded joy stick coordinates structure      */
/*                                                                           */
/*      Call Format:                                                         */
/*           GetJoyPos(&pos, &button, &js);                                  */
/*                                                                           */
/*      Exception Conditions                                                 */
/*           Error:No game port is installed on the machine                  */
/*                 pos    - set to a value of -1                             */
/*                 button - set to a value of -1                             */
/*                                                                           */
/*****************************************************************************/


void GetJoyPos(char far *pos, int far *button, ConfigStruct far *js)
  {
  unsigned int x=0;
  unsigned int y=0;
  unsigned int butn=0;

  ReadJoyStick(&x,&y,&butn);
  if ((y==0xFFFF)&(x==0xFFFF))
    { *pos=-1;}
  if ((y>=js->joyy[1])&(y<=js->joyy[3])&(x>=js->joyx[1])&(x<=js->joyx[3]))
    { *pos=0; }
  if ((y<=js->joyy[1])&(x>=js->joyx[1])&(x<=js->joyx[3]))
    { *pos=1; }
  if ((y<=js->joyy[1])&(x>=js->joyx[3]))
    { *pos=2; }
  if ((y>=js->joyy[1])&(y<=js->joyy[3])&(x>=js->joyx[3]))
    { *pos=3; }
  if ((y>=js->joyy[3])&(x>=js->joyx[3]))
    { *pos=4; }
  if ((y>=js->joyy[3])&(x>=js->joyx[1])&(x<=js->joyx[3]))
    { *pos=5; }
  if ((y>=js->joyy[3])&(x>=js->joyx[0])&(x<=js->joyx[1]))
    { *pos=6; }
  if ((y>=js->joyy[1])&(y<=js->joyy[3])&(x>=js->joyx[0])&(x<=js->joyx[1]))
    { *pos=7; }
  if ((y<=js->joyy[1])&(x<=js->joyx[1]))
    { *pos=8; }

  *button=butn;
  }

int LoadConfigData(ConfigStruct *cs)
  {
  FILE *fp;

  fp=fopen(CONFIGFILE,"rb");
  if (fp==NULL) { InitConfigData(cs); return(FALSE); }
  fread((unsigned char far *)cs, sizeof(ConfigStruct), 1, fp);
  fclose(fp);
  return(TRUE);
  }

int SaveConfigData(ConfigStruct *cs)
  {
  FILE *fp;

  fp=fopen(CONFIGFILE,"wb");
  if (fp==NULL) return(FALSE);
  fwrite((unsigned char far *)cs, sizeof(ConfigStruct), 1, fp);
  fclose(fp);
  return(TRUE);
  }

static void InitConfigData(ConfigStruct *cs)
  {
  char *s;
  int loop;

  s = (char *) cs;

  for (loop=0;loop<sizeof(ConfigStruct);loop++) s[loop]=0;
  cs->ForceVGA = NoType;
  cs->ForceSnd = None;
  cs->SndInt   = 7;
  cs->SndPort  = 0x220;
  strcpy(cs->SndDrvr,"NotInit");
  }

char InitJStick(void)
  {
  unsigned int joyxpos,joyypos,joybutn;

  ReadJoyStick(&joyxpos,&joyypos,&joybutn); /* Read joystick, see if gameport */
  if ((joyxpos!=0xFFFF)&&(joyypos!=0xFFFF)) /*  or joystick exists on machine */
    {
    joyinstall=TRUE;                       /* Set joystick is installed flag*/
    return(TRUE);                          /* Return Joystick installed */
    }
  joyinstall=FALSE;                        /* Set joystick not installed flag */
  return(FALSE);                           /* Return not installed */
  }

#pragma loop_opt(off)

void joyclearbuts(void)
  {
  unsigned int butn,x,y;

  do
    {
    ReadJoyStick(&x,&y,&butn);
    delay(SLEEPTIME);
    } while (butn>0);
  }
  
#pragma loop_opt(on)


