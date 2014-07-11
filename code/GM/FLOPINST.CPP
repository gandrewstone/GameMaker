//---------------------------------------------------------------------------
// install.cpp                Programmer: Andy Stone            7/31/92
//  Program that installs GameMaker.
//
//---------------------------------------------------------------------------

#include <conio.h>
#include <dir.h>
#include <string.h>
#include <direct.h>
#include <stdlib.h>
#include <dos.h>
#include <bios.h>
#include <process.h>
#include <stdio.h>
#include <alloc.h>
#include <time.h>
#include "scrnrout.h"
#include "windio.h"

#define C(x,y) (x+(y*16))
unsigned long int  NEEDEDSPACE  =   8500000;    // 8.5 megs
#define TRUE    1
#define FALSE   0
#define MAXFILENAMELEN  80
#define ARCNAME "LHARC.EXE "

static void MySetVect(int num,char interrupt far (*fn)(...));       
static char *MyGetVect(int num); 
extern "C" char interrupt far NewDOS(...);
static char CopyFile(const char *From,const char *To);
int FindFiles(const char *filespec,const char *path,char *result,unsigned int max,unsigned int *Idx);

static char interrupt far NewCritError(...);

char interrupt far (*OldCritError) (...);
//extern char interrupt far (*OldDOS) (...);
extern unsigned long int far OldDOS;

unsigned long int GetFreeBytes(unsigned char drive);

char upcase(char c);
int  MakeFileName(char *out, const char *path, const char *name);
int  ParseFileName(const char *in,char *name,char *path);
char *_fullpath(char *b, const char *b1, int buflen);
int  InstallIt(char *src, char *dest);
char FirstScreen(char *path);

int NumFloppies=0;
char Files[5000];

int main(int argc, char *argv[])
  {
  char *tst;
  char temp[81];
  int key;
  unsigned long int free=0;
  unsigned char drive=0;
  char frompath[81];
  char topath[81];
  
  ParseFileName(argv[0],temp,frompath);

  MakeFileName(topath,frompath,"install.dat");

  FILE *fp;
  if ( (fp=fopen(topath,"rt"))==NULL)
    {
    printf("Disk Error:I cannot find install.dat!\n");
    return(2);
    }

  fscanf(fp,"%d %lu",&NumFloppies,&NEEDEDSPACE);
  fclose(fp);

  clrbox(0,1,79,25,4);
  writestr(0,0,C(11,1),"              G A M E  -  M A K E R         I N S T A L L A T I O N             ");
  drawbox (0,1,79,24,11,1,0);
  clrbox (1,2,78,23,9);
  if (!FirstScreen(frompath))
    {
    clrbox(0,0,79,25,7);
    printf("GameMaker installation aborted!");
    exit(1);
    }
  clrbox (1,2,78,23,9);
  clrbox (4,12,75,22,27);

  drive=getdisk();
  free=GetFreeBytes(drive+1);
  sprintf(temp,"Bytes Free:%10lu.",free);
  writestr(57,3,14,temp);
  sprintf(temp,"Bytes Needed:%10lu.",NEEDEDSPACE);
  writestr(55,4,15,temp);

  if (free<NEEDEDSPACE) 
    writestr(6,3,11,"You do not have enough space in this drive!");

  writestr(6,6,11,"GameMaker files located at:");
  writestr(35,6,12,frompath);

  writestr(10,23,12,"Hit <ESC> to abort installation, or <RETURN> to continue.");
  writestr(4,11,63,"     File Compression is "ARCNAME"Copyright (C) Haruyasu Yoshizaki     ");
  
  writestr(25,14,30,"   Floppy Install program   ");
  writestr(25,15,30,"Copyright 1994 Gregory Stone");
  writestr(25,16,30,"August 24,1994  Version 3.00");


  temp[0]=0;
  getcwd(temp,80);
  _fullpath(topath,temp,80);
  if (topath[strlen(topath)-1]!='\\')   // Add an ending \ if it needs one
    {
    topath[strlen(topath)+1]=0;
    topath[strlen(topath)]='\\';
    }
  writestr(6,7,11,"GameMaker will create a directory called GM off of:");
  writestr(57,7,12,topath);

  key=bioskey(0)&255;

  if ((free>=NEEDEDSPACE)&&(key!=27)) 
    {
    gotoxy(1,1);
    writestr(10,23,12,"                                                          ");
    OldDOS      =(unsigned long int) MyGetVect(0x21);
    OldCritError=(char interrupt (*)(...)) MyGetVect(0x24);
    MySetVect(0x21,NewDOS);
    MySetVect(0x24,NewCritError);
    InstallIt(frompath,topath);
    MySetVect(0x21,(char interrupt far (*)(...)) OldDOS);
    MySetVect(0x24,OldCritError);  
    clrbox(0,1,79,25,7);
    printf("GameMaker installation complete. Type gm in the gm directory to run.\n");
    exit(1);
    }
  clrbox(0,1,79,25,7);
  if (free<NEEDEDSPACE) printf("GameMaker does not have enough room to install!\nPlease choose another disk (by making it your current directory),\nor free up some space.\n");
  return(1);
  }

char FirstScreen(char *path)
  {
  int y=4;
  char LineIn[81];
  FILE *fp;
  sprintf(LineIn,"%sinstall.txt",path);  
  if ((fp=fopen(LineIn,"rt"))==NULL) return(TRUE);  // Missing file-keep going
  while (feof(fp)==0)
    {
    fgets(LineIn,81,fp);
    if (LineIn[strlen(LineIn)-1]==10) LineIn[strlen(LineIn)-1]=0; 
    writestr(1,y,9,LineIn);
    y++;
    }
  fclose(fp);
  if ((bioskey(0)&255)==27) return(FALSE);
  return(TRUE);
  }

void CopyDirectory(char *src,char *dest)
  {
  char cmd[101];
  sprintf(cmd,"copy %s %s /v\n",src,dest);
  printf(cmd);
  if (system(cmd)==-1) perror("Unable to Copy!\n ");
  }

char *FileExt(const char *File)
  {
  while ( (*File !=0)&&(*File != '.')) File++;
  return((char*)File);
  }


int InstallIt(char *src, char *dest)
  {
  char t[3][81];
  char DeArcer[81];
  int NumFiles;
  int CurFile;
  int CurDisk;
  unsigned int Dummy;

  strcat(dest,"GM");
  mkdir(dest);
  strcat(dest,"\\");

  sprintf(t[0],"%s%s",src,ARCNAME);
  sprintf(DeArcer,"%s%s",dest,ARCNAME);
  CopyFile(t[0],DeArcer);

  for (CurDisk=0;CurDisk<NumFloppies;CurDisk++)
    {
    NumFiles = FindFiles("*.lzh",(const char *) src,Files,4000,&Dummy);
    for (CurFile=0;CurFile<NumFiles;CurFile++)
      {
      strcpy (t[2],Files+(CurFile*14));   // Yea, so its ugly...
      *FileExt(t[2])=0;
      if (strcmpi(t[2],"gm")!=0)
        {
        strcpy(t[0],dest);      // Create the new directory
        strcat(t[0],t[2]);
        mkdir(t[0]);
        strcat(t[0],"\\");
        }
      else                      // Use the existing GM Directory.
        {
        strcpy(t[0],dest);
        }
      sprintf(t[1],"%s%s",src,Files+(CurFile*14));
      printf("%s e %s %s\n",DeArcer,t[1],t[0]);
      if (spawnl(P_WAIT,DeArcer,DeArcer,"e",t[1],t[0],NULL)==-1) perror("De-arcing Error ");
      }
    if (CurDisk+1<NumFloppies)
      {
      sprintf(t[0],"Please insert Game-Maker disk #%d.",CurDisk+2);
      errorbox(t[0],"(C)ontinue");
      }
    }
  printf ("INSTALLATION SUCCESSFUL!");
  bioskey(0);
  return(TRUE);
  }


char upcase(char c)
  {
  if ((c>='a')&&(c<='z')) return(c-('a'-'A'));
  return(c);
  }


unsigned long int GetFreeBytes(unsigned char drive)
  {
  struct dfree ds;
  
  getdfree(drive,&ds);
  return (  ((unsigned long int) ds.df_avail) * 
            ((unsigned long int) ds.df_sclus) * 
            ((unsigned long int) ds.df_bsec)       );
  }

int MakeFileName(char *out, const char *path, const char *name)
  {
  sprintf(out,"%s%s",path,name);
  return(TRUE);
  }

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

static void MySetVect(int num,char interrupt far (*fn)(...))  // Restore Old DOS interrupt
  {
  long int far **ptr=0;
  ptr=(long int far **) (num*4);
  (*ptr) = (long int far *)  fn;
  }

static char far *MyGetVect(int num)  
  {
  return ( *((char far **)(num*4)) );
  }

char w[4000];

int errorbox(char *l1,char *l2,unsigned int pauselen)
  {                          // ^^ OPTIONAL ARGUEMENT - nothing means wait for key.
  time_t curtime=0,starttime=0;
  int ink;
  int x,y,x1,y1;
  int oldx,oldy;
  int len;
  char w[1000];

  len = strlen(l1);
  if (strlen(l2) > len) len = strlen(l2);

  x=40-(len/2)-1;
  y=8;
  y1=9;
  x1=x+len+2;

  savebox(x-1,y-1,x1+3,y1+2,w);
  drawbox(x-1,y-1,x1+1,y1+1,ERRORATTR,2,SHADATTR);
  clrbox(x,y,x1,y1,ERRORATTR);
  writestr(x+1,y,ERRORATTR,l1);
  writestr(x+1,y+1,ERRORATTR,l2);

  time(&starttime);
  do
    { 
    time(&curtime);
    } while ((bioskey(1)==0)&&(curtime<starttime+pauselen));
  if (curtime>=starttime+pauselen) ink=0;
  else if (bioskey(1)) ink=bioskey(0);
  restorebox(x-1,y-1,x1+3,y1+2,w);
  return(ink);
  }



static char interrupt far NewCritError(...)
  {
  if (_AH&64)           // Not A disk Error
    {
    (*OldCritError)();
    return(_AL);
    }
  // Disk Error  
 
  savebox(14,7,72,16,w);
  clrbox(14,7,70,15,78);
  drawbox(14,7,70,15,78,2,56);
  writestr(36,7,78,"DISK ERROR!");
  writestr(16,9,78,"Please insert the correct disk into the drive.");
  writestr(16,10,78,"If the disk is already in the drive, a disk");
  writestr(16,11,78,"error has probably occured.  If GameMaker does");
  writestr(16,12,78,"not work after installation you can attempt to");
  writestr(16,13,78,"reinstall, or contact your dealer for new disks.");
  writestr(30,14,76,"Hit any key to continue:");
  bioskey(0);
  restorebox(14,7,72,16,w);
  return(0);                                    // Ignore the error
  }

static char CopyFile(const char *From,const char *To)
  {
  char *mem=NULL;
  unsigned int startat=64001;
  unsigned int BytesRead=0;
  FILE *fp_in,*fp_out;

  printf("Copying %s to %s.  ",From, To);
  if ( ((fp_in=fopen(From,"rb")) == NULL) || ((fp_out=fopen(To,"wb"))==NULL) )
    { printf("Copy Error!\n"); return(0); }

  while(mem==NULL) { startat-=512; mem=(char *) malloc(startat); }
  do
    {
    BytesRead=fread(mem,sizeof(char),startat,fp_in);
    fwrite(mem,sizeof(char),BytesRead,fp_out);
    } while (BytesRead==startat); // if not equal, an EOF has occured. 
  fclose(fp_in);
  fclose(fp_out);
  printf("Copy successful!\n");
  free(mem);
  return(1);
  }
