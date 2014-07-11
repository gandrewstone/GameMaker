#include <stdio.h>
#include <string.h>
#include <bios.h>
#include "gen.h"
#include "gmgen.h"
#include "Palette.h"
#include "graph.h"
#include "mousefn.h"


extern RGBdata colors[256];
extern char    WorkDir[];

typedef struct
  {
  long int sc;
  char yourname[30];
  } scorelist;

#pragma loop_opt(off)

RGBdata Red  (63,10,10);
RGBdata Blue (10,10,63);
RGBdata Green(10,63,10);


void ShowHighScores(char *gamename,long int score=-1)
  {
  register int j;
  scorelist top[10];
  char str[50];
  FILE *fp;
  char hifile[MAXFILENAMELEN];

  strcpy(hifile,WorkDir);
  strcat(hifile,gamename);
  for (j=0; ( (j<31)&(hifile[j]!='.')) ;j++); // Find extension
  hifile[j]=0;                                // Chop extension
    
  strcat(hifile,".his");

  for (j=0;j<sizeof(scorelist)*10;j++) *( ((char *)&top[0])+j )=0;

  if ( (fp=fopen(hifile,"rb")) != NULL)
    {
    fread( (char *)&top[0],sizeof(scorelist),10,fp);
    fclose(fp);
    }

  Pixel RedCol  = Red  .Match(colors);
  Pixel BlueCol = Blue .Match(colors);
  SetAllPal(colors);

  GWrite(104,0,RedCol,"Top Ten Scores");
  uint Space = 22;
  uint YPos  = 27;
  for (j=0;j<10;j++)
    {
    if (top[j].sc!=0)
      {
      sprintf(str,"%10ld %s",top[j].sc,top[j].yourname);
      GWrite(20,YPos,BlueCol,str);
      }
    YPos+=Space;
    if ((j%2)==0) Space--;
    Space--;
    }
  if (score>0)  // Print your stats on screen.
    {
    sprintf(str,"%10ld Your Latest Score",score);
    GWrite(20,YPos,BlueCol,str);
    }

  GWrite(116,192,RedCol,"Hit any key");
  PauseTimeKey(30);
  }


int CheckHighScores(char *gamename,long int score)
  {
  int RetVal=0;
  register int j,k;
  scorelist top[10];
  char str[50];
  FILE *fp;
  char hifile[MAXFILENAMELEN];

  strcpy(hifile,WorkDir);
  strcat(hifile,gamename);
  for (j=0; ( (j<31)&(hifile[j]!='.')) ;j++); // Find extension
  hifile[j]=0;   // Chop extension
  strcat(hifile,".his");

  for (j=0;j<sizeof(scorelist)*10;j++) *( ((char *)&top[0])+j )=0;

  if ( (fp=fopen(hifile,"rb")) != NULL)
    {
    fread( (char *)&top[0],sizeof(scorelist),10,fp);
    fclose(fp);
    }

  Pixel RedCol  = Red  .Match(colors);
  Pixel BlueCol = Blue .Match(colors);

  for (j=0;j<10;j++)
    {
    if (score>top[j].sc)
      {
      for (k=9;k>j;k--) { top[k]=top[k-1]; }
      top[j].sc = score;
      GWrite(96,40,RedCol,"Congratulations!");
      sprintf(str,"You scored number %2d on the Top Ten!",j+1);
      GWrite(0,100,BlueCol,str);
      top[j].yourname[0]=0;
      GWrite(0,120,RedCol,"Enter Your Name:");
      GGet(136,120,BlueCol,0,top[j].yourname,22);
      RetVal=j+1;
      break;
      }
    }
  if ( (fp=fopen(hifile,"wb")) != NULL)
    {
    fwrite( (char *)&top[0],sizeof(scorelist),10,fp);
    fclose(fp);
    }
  return(RetVal);
  }

#pragma loop_opt(on)
