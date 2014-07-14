// --------------------------------
//  Copyright 1994 G. Andrew Stone
//  Not public domain!
// --------------------------------

#include <bios.h>
#include <dos.h>
#include <dir.h>
#include "gen.h"
#include "debug.hpp"
#include "fileclss.hpp"
#include "viddrive.hpp"
#include "fli.hpp"

#define TMODE 3
void TextMode(void)
  {
  union REGS r;
  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  }

FLI *Fli;
VideoMode *Vid;
int dofindfile(char path[80],char *filespec,char *result);

void main(int argc,char *argv[])
  {
  char         Name[81]   = "e:\\";
  char         Driver[51] = "stlth64.drv";
  char         Temp[81]   = "*.fli",Temp1[81];
  int          ModeNum    =  0;
  VideoDriver *Drvr;

  if (argc>2) ModeNum=argv[2][0]-'0';
  if (argc>1) strcpy(Name,argv[1]);
  printf("Using Driver Mode #%d\n",ModeNum);
  printf("Fli:%s\n",Name);
  bioskey(0);

  if ((Drvr=LoadVideoDriver(Driver))==NULL) { printf("Error Loading Driver.\n"); return; }
  Vid=Drvr->GetMode(ModeNum);
  Vid->SetMode(Vid);
  Fli = new FLI;
  Fli->Vid=Vid;
  Fli->Pos.Set(0,0);
  dofindfile(Name,Temp,Temp1);
  TextMode();
  }

char str[81];
int dofindfile(char path[80],char *filespec,char *result)
  {
  static int addon=0;
  int ctr=0;
  struct ffblk f;
  int ret=0;
  char pf[80];

  strcpy (pf,path);
  strcat (pf,filespec);

     // find all files in this directory
  ret = findfirst(pf,&f,15);
  while(ret==0)
    {
    sprintf(str,"%s%s\0",path,f.ff_name);
      {
      FILE *fp=fopen("flis.ok","at+");
      fputs(str,fp);
      fputs("\n",fp);
      fclose(fp);
      }
    if (Fli->File.Open(str))
      {
      Vid->Clear(0,Vid);
      Fli->View();
      }
    if (bioskey(1))
      {
      char key=bioskey(0)&255;
      switch(key)
        {
        case ' ': bioskey(0); break;
        case 'q': return(0);
        }
      }
    addon += strlen(result+addon)+1;
    ctr++;                 // ctr is the # of files found total.
    ret=findnext(&f);
    }
  // Find all other directories
  strcpy (pf,path);
  strcat (pf,"*.");
  f.ff_attrib=FA_DIREC;

  ret = findfirst(pf,&f,16);

  while (ret==0)
    {
    if ((f.ff_name[0]!='.')&(f.ff_attrib==FA_DIREC))
      {
      sprintf(pf,"%s%s\\",path,f.ff_name);
      ctr+=dofindfile(pf,filespec,result);
      }
    ret = findnext(&f);
    }
  return(ctr);             // Fn returns # of file names found
  }


