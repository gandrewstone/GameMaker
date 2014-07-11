/*---------------------------------------------------------------------*/
/* genc.C  - generic graphics routines                                 */
/* FEB 10, 1991           Programmer:  Andy Stone                      */
/* Last edited: 6/26/93                                                */
/*---------------------------------------------------------------------*/

#include <dos.h>
#include <alloc.h>
#include <new.h>
#include <stdio.h>
#include <string.h>
#include <bios.h>
#include <stdlib.h>
#include <time.h>
#include "gen.h"
#include "gmgen.h"
#include "mousefn.h"
#include "graph.h"
#include "palette.h"

void swap(int *,int*);
int scrn=0;
unsigned int CurMode=TMODE;

char WorkDir[MAXFILENAMELEN]="";

static char CopyFile(const char *From,const char *To);


#ifdef MOUSE    // That is -- playgame is being compiled.
extern int GetKey(char Peek);
extern int MouseMaxy,MouseMaxx;
extern unsigned int MXlatx,MXlaty;   
#else
int GetKey(char Peek) { return(bioskey(Peek)); }
#endif

extern int mouinstall;

boolean  RestorePal=False;

class ConfigData
  {
  public:
  char WorkingDirectory[MAXFILENAMELEN];
  void Read(FILE *fp)  { fread((char *)this,sizeof(ConfigData),1,fp);  }
  void Write(FILE *fp) { fwrite((char *)this,sizeof(ConfigData),1,fp); }

  ConfigData();
  ~ConfigData();
  };

static ConfigData Cfg;

ConfigData::ConfigData()
  {
  FILE *fp;
  set_new_handler(0);
  if ((fp=fopen("gm.cfg","rb"))!=NULL)
    {
    Cfg.Read(fp);
    fclose(fp);
    strcpy(WorkDir,WorkingDirectory);
    }
  else
    {
    for (uint i=0; i<MAXFILENAMELEN;i++)
      { WorkDir[i]=0; WorkingDirectory[i]=0; }
    }
  }

ConfigData::~ConfigData()
  {
  FILE *fp;

  if ((fp=fopen("gm.cfg","wb"))!=NULL)
    {
    strcpy(Cfg.WorkingDirectory,WorkDir);
    Cfg.Write(fp);
    fclose(fp);
    }
  if (RestorePal)
    {
    union REGS r;
    r.h.ah = 0;
    r.h.al = TMODE;
    int86(0x10,&r,&r);
    CurMode=TMODE;
    }
  }



int PauseTimeKey(uint seconds)
  {
  time_t cur,start;
  int mx,my,mbuts=0;

  time(&start);
  do
    {
    time(&cur);
    if (mouinstall) moustats(&mx,&my,&mbuts);
    } while ((!mbuts)&&(GetKey(1)==0)&&(cur<start+seconds));
  if (GetKey(1)) return(GetKey(0));     // Return the key
  if (mbuts) { mouclearbut(); return(1); }
  return(FALSE);                        // Time out! - return FALSE
  }

char *FileExt(const char *File)
  {
  while ( (*File !=0)&&(*File != '.')) File++;
  return((char*)File);
  }



int MakeFileName(char *out, const char *path, const char *name, const char *ext)
  {
  sprintf(out,"%s%s%s",path,name,ext);
  return(TRUE);
  }

#pragma loop_opt(off)

int ParseFileName(const char *in,char *name,char *path)
  {
  int LastSlash=-1;
  int l=0,pathctr=0,namectr=0;

  name[0]=0;
  path[0]=0;
  while ((in[l]!=0)&&(in[l]!='.'))
    {
    path[pathctr]=in[l];
    name[namectr]=in[l];
    if (in[l]=='\\')
      {
      LastSlash=pathctr;
      namectr=-1;
      }
    if (in[l]=='.') break;
    l++; 
    pathctr++;
    namectr++;
    if (l>MAXFILENAMELEN) return(FALSE); 
    }
  path[LastSlash+1]=0;
  name[namectr]=0;
  return(TRUE);
  }

int ParseFile(const char *in,char *name,char *path)
  {
  int LastSlash=-1;
  int l=0,pathctr=0,namectr=0;

  name[0]=0;
  path[0]=0;
  while (in[l]!=0)
    {
    path[pathctr]=in[l];
    name[namectr]=in[l];
    if (in[l]=='\\')
      {
      LastSlash=pathctr;
      namectr=-1;
      }
    l++; 
    pathctr++;
    namectr++;
    if (l>MAXFILENAMELEN) return(FALSE); 
    }
  path[LastSlash+1] = 0;
  name[namectr]     = 0;
  return(TRUE);
  }



#pragma loop_opt(on)

void GetScrn(char far *mem)
  {
  memcpy(mem,(void far *) 0x0A0000000, (unsigned) 64000);
  }

void RestoreScrn(char far *mem)
  {
  memcpy((void far *) 0xA0000000,mem,(unsigned) 64000);
  }
  
#pragma loop_opt(off)

int DisplayHelpFile(char *filename)
  {
  FILE *fp;
  uchar far *addr = (unsigned char far *) 0xB8000000;
  uint l;
  char ch;

  fp=fopen(filename,"rb");
  if (fp==NULL) return(FALSE);
  for(l=0;l<4000;l++) { ch=fgetc(fp); *(addr+l)=ch; }
  fclose(fp);

  if (mouinstall) mouclearbut();
  PauseTimeKey();
  return(TRUE);
  }
  
#pragma loop_opt(on)

/*      Changes the Video mode                                          */

#pragma loop_opt(on)

void TextMode(void)
  {
  union REGS r;


#ifdef MOUSE
  int m;

  if (mouinstall)
    {
    m=moucur(2);
    if (m) moucur(0);
    }
#endif
  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
  CurMode=TMODE;

#ifdef MOUSE
  if (mouinstall)
    {
    initmouse();
    MXlatx=MouseMaxx/79;
    if (MXlatx==0) MXlatx=1;
    MXlaty=MouseMaxy/24;
    if (MXlaty==0) MXlaty=1;
    if (m) moucur(TRUE);
    }
#endif
  SetTextColors();
  }

void SetTextColors(void)
  {
#ifndef DEBUG
  Palette(5,10,10,10);
  Palette(7,16,16,16);
  Palette(6,40,40,40);
  Palette(1,0,0,30);
  Palette(2,0,0,50);
  Palette(3,10,10,63);
#endif
  }


void GraphMode(void)
  {
  union REGS r;
#ifdef MOUSE
  int m;

  if (mouinstall)
    {
    m=moucur(2);
    if (m) moucur(0);
    }
#endif

  r.h.ah = 0;
  r.h.al = GMODE;
  CurMode=GMODE;
  int86(0x10,&r,&r);

#ifdef MOUSE
  if (mouinstall)
    {
    initmouse();
    MXlatx= MouseMaxx/319;     //    MXlatx=2;
    if (MXlatx==0) MXlatx=1;
    MXlaty= MouseMaxy/199;
    if (MXlaty==0) MXlaty=1;
    if (m) moucur(TRUE);
    }
#endif
  }

int loadspecany(const char *file,const char *ext,const char *path,unsigned int bytes, char *addr)
  {
  FILE *fp;
  char result;
  char srchfor[6];
  char fname[MAXFILENAMELEN];
  
  MakeFileName(fname,path,file,ext);

  if ( (fp=fopen(fname,"rb")) ==NULL) return(FALSE);
  fread(addr,bytes,1,fp);
  fclose(fp);
  return(TRUE);
  }

void swap(int *v1,int *v2)
  {
  int v3;

  v3=*v1;
  *v1=*v2;
  *v2=v3;
  }
