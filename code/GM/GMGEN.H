/* --------------------------------------------------------------------*/
/* 320x200x256   By Andy Stone                                         */
/* generic graphics declarations - source in gengrph.c and genrout.asm */
/*---------------------------------------------------------------------*/
#ifndef GENERIC_DEFINES
#define GENERIC_DEFINES

#include "gen.h"

//#define TRANSFER  // defined means that transfer playgame will be compiled

//#define DEBUG 1
#ifdef DEBUG
extern int AbortMSGLength;
extern char AbortMSG[120];
extern int ExitValue;
extern char NewAbortMSG(char *str);
#endif
#define MENUPROG "menu.exe"
//#define CHKREG
#define PGMTITLECOL 47
#define GMVER "3.00"
#define VERSION 300
#define OKMSG "cOoL"
#define REGOK   0x45
#define GMTOPBAR "                            G A M E  -  M A K E R  V"GMVER"                        "
#define VGATYPEFILE "VGATYPE.DAT"

#define MAXFILENAMELEN 51

#define NEWLINE   10
#define UPKEY     72
#define DOWNKEY   80
#define RIGHTKEY  77
#define LEFTKEY   75

#define CRIPPLEWARE

extern void GraphMode(void);
extern void TextMode(void);
extern void SetTextColors(void);
extern void Screen(int OnOff);
extern void RestoreScrn(char far *mem);
extern void GetScrn(char far *mem);

extern void swap(int *a, int *b);
extern int  ParseFileName(const char *string,char *gamename,char *gamepath);
extern int  ParseFile(const char *string,char *gamename,char *gamepath);  // This one keeps the file extension.
extern int  MakeFileName(char *out, const char *path, const char *name, const char *ext);
extern char *FileExt(const char *File);
extern int  DisplayHelpFile(char *filename);
int loadspecany(const char *file,const char *fspec,const char *path,unsigned int bytes, char *addr);

int PauseTimeKey(uint seconds=0xFFFF);
extern void NewBombOut(void);

typedef enum
  {
  quit=0, utility,palchos,blocedit,monedit,mapmaker,
  charedit,image,sndedit,grator, playgame, menu, chkreg
  } QuitCodes;

typedef enum
  {
  NoDesignation=0,PalFile,BackBlkFile,MonBlkFile,ChrBlkFile,
  MonFile,MapFile,ChrFile,GamFile,MenuSystem
  } FileTypes;


#ifndef SVGA
extern "C"
  {
  extern void far Gwritestr(int x,int y,int col,char *str,int len);
  extern void interrupt NewTimerAsm(...);
  }
#endif

#include <stdio.h>

class DataFileHeader
  {
  public:
  char GMId[2];
  int  Ver;
  char Encrypt;
  char FileType;
  private:
  char Xtra[256-6];
  public:
  DataFileHeader()
    {
    for (int i=0;i<256-6;i++) Xtra[i]="RSD"[i%3];  // Fill up the space.
    GMId[0] = 'G';
    GMId[1] = 'M';
    Ver     = VERSION;
    Encrypt = FALSE;
    FileType= NoDesignation;
    }

  boolean Validate(void)  { return((boolean) ((GMId[0]=='G')&&(GMId[1]=='M')) ); }
  boolean Load(FILE *fp)
    {
    if (fread(this,sizeof(DataFileHeader),1,fp)!=1) { GMId[0]=0; return(FALSE); }
    return(TRUE);
    }
  boolean Save(FILE *fp)
    {
    if (fwrite(this,sizeof(DataFileHeader),1,fp)!=1) return(FALSE);
    return(TRUE);
    }
  };

extern char    WorkDir[];
extern boolean RestorePal;

void Personalize(void);
void SavePersonal(void);

#endif
