#include <stdio.h>
#include <dir.h>
#include <string.h>
#include <bios.h>
#include <dos.h>
#include <alloc.h>
#include <process.h>
#include <stdlib.h>
#include <ctype.h>

#include "d:\drv\gen.h"
#include "gmgen.h"
#include "mousefn.h"
#include "jstick.h"
#include "scrnrout.h"
#include "windio.h"
#include "palette.h"
#include "bytenums.h"
#include "grator.h"
#include "sound.h"

//static void doutils(char ch);
#define  SLEEPTIME 50

static void cleanup(void);
static void back_em(void);
static void restore_em(void);
static void transfer_em(void);
static void TransferVocs(char *SndFile,char *PathToSave);
static void movegif(char *fina);
static int  write_menu(char *w,int attr);
static char CopyFile(const char *From,const char *To);
static void AddSlash(char *str);

static void JoySetup(void);
static void JoyDiags(void);
static void getjoydat(unsigned int *x, unsigned int *y,unsigned int *butn);
static void UpdateWindow(int dir, int x , int y, char buttons);
static void NoPortError(void);
static void NoFileError(void);

#define HELPFI  "backres.hlp"     // utilities help
#define EXEFILE "xferplay.exe"    // 'to be transferred' files
#define GMGIF   "gmtitle.gif"

static char interrupt far (*OldCritError) (...);
char             PathToSave[MAXFILENAMELEN] = "A:\\";
char             Fspec[7] = "";
char             FileName[MAXFILENAMELEN] = "";
extern char      curfile[MAXFILENAMELEN];
ConfigStruct     cs;
extern unsigned _stklen=6000U;  // Set the stack to 6000 bytes, not 4096

extern char WorkDir[];

QuitCodes main(int argc, char *argv[])
  {
  char choice=0;
  int attr=0;
  char w[1000];
  int x,y,buts;

#ifdef CHKREG
  if ((argc < 2) || (strcmp(argv[1],OKMSG)!=0)) RestorePal=True;
#endif

  SetTextColors();
  if (initmouse()==FALSE)
    {
    errorbox("GameMaker requires a Microsoft","compatible mouse! (Q)uit");
    return(quit);
    }
  if (argc>1) choice=argv[1][1];

  if ((choice<1)||(choice>7)) choice=1;
  mouclearbut();
  do
    {
    writestr(0,0,79,GMTOPBAR);
    writestr(0,1,31," Backup  Restore  Transfer  Joystick Setup   Joystick Diagnostic   Main   Help  ");

    choice=horizmenu(7,choice,1,0,8,17,27,43,66,72,80);
    switch(choice) 
      {
      case 1:
        if (!qwindow(3,10,40,"Enter Path to Backup files to: ",PathToSave)) break;
        strlen(PathToSave);
        AddSlash(PathToSave);
        do 
          {
          attr = openmenu(30,5,27,12,w);
          writestr(30,5,attr+4,"          BACKUP");
          choice = write_menu(w,attr); 
          if (choice) back_em();
          } while (choice);
        choice = 1;
        break;
      case 2:
        {
        char Temp [MAXFILENAMELEN];
        strcpy(Temp,WorkDir);
        strcpy(FileName,".\\");
        strcpy(PathToSave,WorkDir);
        if (!qwindow(3,10,35,"Enter game area to restore to: ",PathToSave))
          break;
        do 
          {
          attr   = openmenu(30,5,27,12,w);
          writestr(30,5,attr+4,"          RESTORE"); 
          choice = write_menu(w,attr);
          if (choice) 
            {
            AddSlash(FileName);
            restore_em();
            }
          } while (choice);
        strcpy(PathToSave,"A:\\");
        strcpy(WorkDir,Temp);
        choice = 2;
        }
        break;
      case 3:
        {
#ifdef CRIPPLEWARE
        StdCrippleMsg();
#else
        uchar done=0;
        while (!done)
          {
          char Temp[MAXFILENAMELEN];
          struct ffblk f;
          if (!qwindow(3,10,40,"Enter Path to Transfer Game to: ",PathToSave)) break;
          AddSlash(PathToSave);
          strcpy(Temp,PathToSave);
          strcat(Temp,"*.*");
          if (findfirst(Temp,&f,(FA_DIREC|FA_ARCH)) ==-1)
            {
            while(1)
              {
              char inkey=toupper(errorbox("I cannot access this Directory!","(C)reate  (A)bort  (R)eenter name")&255);
              if (inkey=='C')
                {
                strcpy(Temp,PathToSave);
                Temp[strlen(Temp)-1] = 0;
                mkdir(Temp); done=2; break;
                }
              if (inkey=='A') { done=True; break; }
              if (inkey=='R') break;
              }
            }
          else done=2;
          }
        if (done==2) transfer_em();
#endif
        } break;
      case 4:  // do joystick setup
        JoySetup();
        break;
      case 5:  // do joystick Diags
        JoyDiags();
        break;
      case 7:  //  help file
        DisplayHelpFile(HELPFI);
        while (bioskey(1)) bioskey(0);
        break;
     }
    mouclearbut();
    } while ((choice!=6)&&(choice!=0));
  return(menu);
  }

static char w[2000];
static unsigned char al;
char interrupt far NewCritError(...)
  {
  if (_AH&64)           // Not A disk Error
    {
    (*OldCritError)();
    return(_AL);
    }
  // Disk Error  
  al=_AL;
  savebox(5,9,67,13,w);
  clrbox(5,9,65,12,78);
  drawbox(5,9,65,12,78,2,56);
  if (al<2)  // Floppy drive error
    writestr(7,10,79,"I can't access the disk!  Did you remember to insert one?");
  else writestr(7,10,79,"I can't access the disk!");
  writestr(27,11,79,"(R)etry (A)bort");

  CritVal=100;
  while(CritVal==100)
    {
    al=toupper(bioskey(0)&255);
    if (al=='R') CritVal=1;
    if (al=='A') CritVal=3;
    }
  restorebox(5,9,67,13,w);
  return(CritVal);
  }



static int write_menu(char *w,int attr)
  {
  char FileSpecs[][7] = { "*.bbl\0","*.mbl\0","*.cbl\0","*.mon\0","*.chr\0","*.map\0","*.pal\0","*.snd\0","*.gam\0"};
  int choice = 1;

  writestr(31,7,attr+14, " Background Block Set");
  writestr(31,8,attr+14, " Monster Block Set");
  writestr(31,9,attr+14, " Character Block Set");
  writestr(31,10,attr+14," Monster Information Set");
  writestr(31,11,attr+14," Character Information Set");
  writestr(31,12,attr+14," Map");
  writestr(31,13,attr+14," Palette");
  writestr(31,14,attr+14," Sound Set");
  writestr(31,15,attr+14," Game Information");
  writestr(31,16,attr+14," Menu");
  choice = vertmenu(30,7,27,10,choice);
  closemenu(30,5,27,12,w);
  if ((choice>9)||(choice<1)) return(0);
  strcpy(Fspec,FileSpecs[choice-1]);
  return(1);
  }

static void back_em(void)
  {
  char FName[MAXFILENAMELEN];
  char Path[MAXFILENAMELEN];
  FileName[0]=0;
  if (getfname(5,10,"Enter File to Backup: ",Fspec,FileName))
    {
    ParseFile(FileName,FName,Path);  // Keeps the Extension
    sprintf(Path,"%s%s",PathToSave,FName);
    CopyFile(FileName,Path);
    }
  }

static void restore_em(void)
  {
  char Temp [MAXFILENAMELEN];
  char FName[MAXFILENAMELEN];
  char Path [MAXFILENAMELEN];
  if (getfname(5,10,"Enter File to Restore: ",Fspec,FileName))
    {
    ParseFile(FileName,FName,Path);
    sprintf(Temp,"%s%s",PathToSave,FName);
    CopyFile(FileName,Temp);
    strcpy(FileName,Path);
    }
  }

int FindFiles(const char *filespec,const char *path,char *result,uint max,uint *Idx);
char wind[700];
static void transfer_em(void)
  {
#ifdef CRIPPLEWARE
  StdCrippleMsg();
#else
  int   j;
  int   attr;
  char *temp;
  int   pathlen,scnum = 2;
  char  FromName[MAXFILENAMELEN] = "";
  char  ToName[MAXFILENAMELEN]   = "";
  char *Files = new char [4000];
  FILE *fp;
  int   NumFiles=0;
  uint  Last;

  if (Files==NULL) { errorbox("Out of Memory!","(R)eturn"); return; }

  if (!getfname(5,5,"Enter Game to Transfer: ","*.gam\0",ToName)) return;
  ParseFileName(ToName,curfile,FromName);  // Fromname is a dummy.
                                           // Get name W/out extension

  NumFiles = FindFiles("*.*\0",WorkDir,Files,4000,&Last);

  for (j=0;j<NumFiles;j++)
    {
    strcpy(ToName  ,PathToSave);
    strcpy(FromName,WorkDir);
    strcat(ToName  ,Files+(j*14));
    strcat(FromName,Files+(j*14));
    CopyFile(FromName,ToName);
    }

  temp=strrchr(curfile,'.');  // Chop off the file's extension
  if (temp!=NULL) *temp = 0;  

//  sprintf(ToName,"%sGMHELP.TXT",PathToSave);    // Transfer GM help file
//  CopyFile("gmhelp.txt",ToName);
  sprintf(ToName,"%sConfig.hlp",PathToSave);    // Transfer config help file
  CopyFile("config.hlp",ToName);
  sprintf(ToName,"%ssndblast.drv",PathToSave);  // Transfer sound driver
  CopyFile("sndblast.drv",ToName);

  attr = openmenu((80-15)/2,10,15,3,wind);
  writestr((80-15)/2,10,attr+4,   " Copying...");
  writestr((80-15)/2+1,12,attr+14,"Executable");

  temp=curfile;
  sprintf(ToName,"%s%s.exe",PathToSave,temp);
  CopyFile(EXEFILE,ToName);

  sprintf(ToName,"%sconfig.bat",PathToSave);
  if ((fp=fopen(ToName,"wt"))!=NULL)
    {
    fprintf(fp,"%s configure\n",temp);
    fclose(fp);
    }

  closemenu((80-15)/2,10,15,3,wind);
  errorbox("Transfer Done!", "(C)ontinue",10);
#endif
  }  


static char CopyFile(const char *From,const char *To)
  {
  char str[81];
  char worked=TRUE;
  char far *mem=NULL;
  unsigned int startat=64512;
  unsigned int BytesRead=0;
  FILE *fp_in,*fp_out;
  int attr=0;
  char w[1000];
  char size;

  OldCritError=(char interrupt (*)(...)) getvect(0x24);
  setvect(0x24,(void interrupt (*)(...)) NewCritError);
  
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
  closemenu((80-size)/2,10,size,3,w);
  setvect(0x24,(void interrupt (*)(...)) OldCritError);
  return(worked);
  }

static void AddSlash(char *str)
  {
  int leng=0;
  leng=strlen(str);
  if ((str[(leng-1)] != '\\')&&(leng<MAXFILENAMELEN)) 
    { str[leng]='\\'; str[leng+1]=0; }
  }

//-------JoySetup.

static void JoySetup(void)
  {
  int  wx=13,wy=8;
  char attr;
  char tempstr[81];
  char w[1000];
  unsigned int  x=0,y=0,butn=0;
  unsigned int filestatus;

  ReadJoyStick(&x,&y,&butn);

  if ((x==0xFFFF)&(y==0xFFFF))
    {
    butn=0;
    attr = openmenu(wx,wy,51,6,w);
    writestr(wx+1,wy+1,attr+14,"This machine does not have a game port installed!");
    writestr(wx+1,wy+3,attr+11,"You cannot configure a joystick until you have");
    writestr(wx+1,wy+4,attr+11,"      installed a game port and joystick.");
    while (bioskey(1)) bioskey(0);
    mouclearbut();
    while ((!bioskey(1))&&(!butn)) moustats(& (int)x,& (int)y,&(int)butn);
    if (bioskey(1)) bioskey(0);
    else mouclearbut(); 
    closemenu(wx,wy,51,6,w);
    return;
    }

  attr = openmenu(wx,wy,51,2,w);
  writestr(wx+1,wy,attr+14,"Move joystick all the way Forward and Left.");
  writestr(wx+1,wy+1,attr+11,"Press either joystick button when there. ");
  getjoydat(&x,&y,&butn);

  cs.joyy[0]=y;
  cs.joyx[0]=x;
  sprintf(tempstr,           "          Thanks...Stick minimum (%d,%d)       ",x,y);
  writestr(wx+1,wy+1,attr+14,tempstr);
  delay(500);
  joyclearbuts();

  writestr(wx+1,wy,attr+14,"Move joystick all the way Back and Right.  ");
  writestr(wx+1,wy+1,attr+11,"Press either joystick button when there.   ");
  butn=0;
  getjoydat(&x,&y,&butn);
  cs.joyy[4]=y;
  cs.joyy[2]=(((cs.joyy[4]-cs.joyy[0])/2)+cs.joyy[0]);
  cs.joyy[1]=(((cs.joyy[2]-cs.joyy[0])/2)+cs.joyy[0]);
  cs.joyy[3]=(((cs.joyy[4]-cs.joyy[2])/2)+cs.joyy[2]);
  cs.joyx[4]=x;
  cs.joyx[2]=(((cs.joyx[4]-cs.joyx[0])/2)+cs.joyx[0]);
  cs.joyx[1]=(((cs.joyx[2]-cs.joyx[0])/2)+cs.joyx[0]);
  cs.joyx[3]=(((cs.joyx[4]-cs.joyx[2])/2)+cs.joyx[2]);

  sprintf(tempstr,"          Thanks...Stick maximum (%d,%d)       ",x,y);
  writestr(wx+1,wy+1,attr+14,tempstr);
  delay(500);
  joyclearbuts();

  filestatus=SaveConfigData(&cs);
  closemenu(wx,wy,51,2,w);
  if (filestatus!=TRUE)
    {
    errorbox("An error occured when writing the joystick info. to your disk!",
             "Please check to make sure you have enough disk space. (E)xit.");
    }
  }

static void getjoydat(unsigned int *x, unsigned int *y,unsigned int *butn)
  {
  while ((*butn)==0)
    {
    ReadJoyStick(x,y,butn);
    delay(SLEEPTIME);
    }
  }

//------JoyDiags.

const  int  wx=20, wy=5;
static char attr;

static void JoyDiags(void)
  {
  char w[1000];
  unsigned int x,y;
  char pos;
  unsigned int button;
  unsigned int filestatus;

  ReadJoyStick(&x,&y,&button);

  if ((x==0xFFFF)&(y==0xFFFF))
    {
    NoPortError();
    return;
    }

  filestatus=LoadConfigData(&cs);
  if (filestatus!=TRUE)
    {
    NoFileError();
    cs.joyx[0]=0;
    cs.joyy[0]=0;
    cs.joyx[4]=500;
    cs.joyy[4]=500;
    cs.joyx[2]=(((cs.joyx[4]-cs.joyx[0])/2)+cs.joyx[0]);
    cs.joyx[1]=(((cs.joyx[2]-cs.joyx[0])/2)+cs.joyx[0]);
    cs.joyx[3]=(((cs.joyx[4]-cs.joyx[2])/2)+cs.joyx[2]);
    cs.joyy[2]=(((cs.joyy[4]-cs.joyy[0])/2)+cs.joyy[0]);
    cs.joyy[1]=(((cs.joyy[2]-cs.joyy[0])/2)+cs.joyy[0]);
    cs.joyy[3]=(((cs.joyy[4]-cs.joyy[2])/2)+cs.joyy[2]);
    }

  attr = openmenu(wx,wy,32,6,w);
  writestr(wx,wy,attr+14,  "Direction    Position   Buttons");
  writestr(wx,wy+1,attr+11,"             X:         1:     ");
  writestr(wx,wy+2,attr+11,"             Y:         2:     ");
  writestr(wx,wy+3,attr+14,"             Strange Directions?");
  writestr(wx,wy+4,attr+14," Use joystick setup to correct.");
  writestr(wx,wy+5,63,"      Hit any key to Stop       ");

  button=0;
  while (!bioskey(1))
    {
    unsigned int oldx=0,oldy=0;
    char oldbut=4,oldpos=5;
    
    ReadJoyStick(&x,&y,&button);
    delay(SLEEPTIME);
    GetJoyPos(&(char) pos, &(int)button, &cs);

    if ((x!=oldx)|(y!=oldy)|(pos!=oldpos)|(button!=oldbut)) 
      {
      UpdateWindow(pos,x,y,button);
      x=oldx;
      y=oldy;
      pos=oldpos;
      button=oldbut; 
      }  
    }
  bioskey(0);
  closemenu(wx,wy,32,6,w);
  return;
  }

static void UpdateWindow(int dir,int x,int y, char butn)
  {
  char temp[3][41];
  static const char UpDown[][5]={"Up  ","Down"};

  sprintf(temp[0],"             X:%3d      1:%s",x,UpDown[(butn&1)]);
  sprintf(temp[1],"             Y:%3d      2:%s",y,UpDown[((butn&2)>>1)]);
  sprintf(temp[2],"             ");
  
  switch(dir)
    {
    case 0:
      temp[1][4]='O';
      break;  
    case 1:
      temp[0][4]='^'; 
      break;
    case 2:
      temp[0][7]='Ä'; temp[0][8]='¿';  
      break;    
    case 3:
      temp[1][8]='>';
      break;
    case 4:
      temp[2][7]='Ä'; temp[2][8]='Ù';  
      break;
    case 5:
      temp[2][4]='v'; 
      break;
    case 6:
      temp[2][0]='À'; temp[2][1]='Ä';
      break;
    case 7:
      temp[1][0]='<';
      break;
    case 8:
      temp[0][0]='Ú'; temp[0][1]='Ä';
      break;
    }
  writestr(wx,wy+1,attr+11,temp[0]);
  writestr(wx,wy+2,attr+11,temp[1]);
  writestr(wx,wy+3,attr+11,temp[2]);
  }
         

static void NoFileError(void)
  {
  int wx=13, wy=8;
  char w[1000];
  int mx,my,butn=0;

  attr = openmenu(wx,wy,51,6,w);

  writestr(wx+1,wy+1,attr+14,"      The joystick Data file is missing!");
  writestr(wx+1,wy+2,attr+11,"To make a data file, choose the joystick setup");
  writestr(wx+1,wy+3,attr+11,"option in the utilities menu.  Since this file");
  writestr(wx+1,wy+4,attr+11,"is missing, the joystick directions may not be");
  writestr(wx+1,wy+5,attr+11,"accurate in this program.");
  while ((!bioskey(1))&&(!butn)) moustats(&mx,&my,&butn);
  if (bioskey(1)) bioskey(0);
  else mouclearbut(); 
  closemenu(wx,wy,51,6,w);
  }

static void NoPortError(void)
  {
  int wx=13, wy=8;
  char w[1000];
  int mx,my,butn=0;

  attr = openmenu(wx,wy,51,6,w);
  writestr(wx+1,wy+1,attr+14,"This machine does not have a game port installed!");
  writestr(wx+1,wy+3,attr+11,"You cannot configure a joystick until you have");
  writestr(wx+1,wy+4,attr+11,"      installed a game port and joystick.");
  while ((!bioskey(1))&&(!butn)) moustats(&mx,&my,&butn);
  if (bioskey(1)) bioskey(0);
  else mouclearbut(); 
  closemenu(wx,wy,51,6,w);
  }


/*
static void OLDtransfer_em(void)
  {
  int attr;  
  char *temp;
  register int j,k;
  int pathlen,scnum = 2;
  char ToName[MAXFILENAMELEN];
  char exts[LASTFNAME][5] = { ".PAL",".MAP",".BBL",".MON",".MBL",".CHR",".CBL",".SND",".CMF","M-ty"};
  char fpaths[LASTFNAME][5]={ "PAL\\","MAP\\","BLK\\","MON\\","BLK\\","CHR\\","BLK\\","SND\\","SND\\","M-ty"};
  char exts2[2][STARTENDFILES][5] =
                        { {".BKD", ".TXT", ".TXT", ".TXT", ".CMF" },
                          {".BKD", ".TXT", ".TXT", ".CMF", "M-ty" } };
  char fpaths2[2][STARTENDFILES][5]=
                        { {"GIF\\", "GAM\\", "GAM\\", "GAM\\", "SND\\" },
                          {"GIF\\", "GAM\\", "GAM\\", "SND\\", "M-ty"  } };
  char w[1000];
  FILE *fp;

  if (!loadany("Enter Game to Transfer: ",".gam","gam\\",sizeof(integratorstruct)*MAXSCENES,(char *)&scns[0],1) ) return;

  for (scnum=2;scnum<MAXSCENES;scnum++) 
    { 
    if (scns[scnum].gametype!=NOSCENE)
      {
      for (j=0;j<LASTFNAME;j++) 
        {
        if (strcmp (exts[j],"M-ty")!=0)
          {
          for (k=2;k<scnum;k++)
            {
            if (stricmp(scns[scnum].fnames[j],scns[k].fnames[j])==0)
              {  // Search through for other instances of the file 
                 // if found erase the file so it is not xfered twice
              scns[scnum].fnames[j][0] = 0;  
              }
            }
          if (scns[scnum].fnames[j][0] != 0) 
            {
            sprintf(FileName,"%s%s%s",fpaths[j],scns[scnum].fnames[j],exts[j]);
            sprintf(ToName,"%s%s%s",PathToSave,scns[scnum].fnames[j],exts[j]);
            if (!CopyFile(FileName,ToName)) return;
            if (j==SNDFILE) TransferVocs(FileName,PathToSave);
            }
          }
        }
      }
    }  // Done transferring all of the general data files. 

      // Copy the .gam file over.
  strcpy(FileName,curfile);             // Get its name
  temp = strrchr( (char*)FileName,'\\');        // find where the path ends
  sprintf(ToName,"%s%s",PathToSave,temp+1);
  CopyFile(FileName,ToName);

  temp=strrchr(curfile,'.');  // Chop off the file's extension
  if (temp!=NULL) *temp = 0;  

  for (scnum=0;scnum<2;scnum++) 
    {
    for (j=0;j<STARTENDFILES;j++)
      {
      if (scns[scnum].fnames[j][0] != 0) 
        {
        sprintf(FileName,"%s%s%s",fpaths2[scnum][j],scns[scnum].fnames[j],exts2[scnum][j]);
        sprintf(ToName,"%s%s%s",PathToSave,scns[scnum].fnames[j],exts2[scnum][j]);

        if (stricmp(exts2[scnum][j],".BKD")==0)
          {
          if ((scnum==1)||(stricmp(scns[0].fnames[j],scns[1].fnames[j]) != 0))
            movegif(FileName);
          }
        else CopyFile(FileName,ToName);
        }
      }
    }

  sprintf(ToName,"%s%s",PathToSave,GMGIF);     // Transfer GM presents Gif
  sprintf(FileName,"gif\\%s",GMGIF);
  CopyFile(FileName,ToName);
  sprintf(ToName,"%sGMHELP.TXT",PathToSave);   // Transfer GM help file
  CopyFile("gam\\gmhelp.txt",ToName);
  sprintf(ToName,"%sConfig.hlp",PathToSave);   // Transfer config help file
  CopyFile("config.hlp",ToName);
  sprintf(ToName,"%ssndblast.drv",PathToSave);   // Transfer config help file
  CopyFile("sndblast.drv",ToName);


  attr = openmenu((80-15)/2,10,15,3,w);
  writestr((80-15)/2,10,attr+4,   " Copying...");
  writestr((80-15)/2+1,12,attr+14,"Executable");

  temp=strrchr(curfile,'\\');
  if (temp==NULL) temp=curfile;
  else temp++;
  sprintf(ToName,"%s%s.exe",PathToSave,temp);
  CopyFile(EXEFILE,ToName);

  sprintf(ToName,"%sconfig.bat",PathToSave);
  if ((fp=fopen(ToName,"wt"))!=NULL)
    {
    fprintf(fp,"%s configure\n",temp);
    fclose(fp);
    }

  closemenu((80-15)/2,10,15,3,w);
  errorbox("Transfer Done!", "(C)ontinue",10);
  }  

void TransferVocs(char *SndFile,char *PathToSave)
  {
  FILE *Sndfp;
  register int loop;
  char FromFile[MAXFILENAMELEN],ToFile[MAXFILENAMELEN];

  if ( (Sndfp=fopen(SndFile,"rb"))==NULL) return;

  fseek(Sndfp,LASTSND*sizeof(sndstruct),SEEK_SET);
  for (loop=0;loop<9*LASTSND;loop++) *(DigiSnd[0]+loop)=0;
  fread(DigiSnd,LASTSND,9,Sndfp);
  fclose(Sndfp);
  for (loop=0;loop<LASTSND;loop++)
    {
    if ((DigiSnd[loop][0]!=0) && (DigiSnd[loop][0]!=' '))
      {
      sprintf(FromFile,"snd\\%s.voc",DigiSnd[loop]);
      sprintf(ToFile,"%s%s.voc",PathToSave,DigiSnd[loop]);
      CopyFile(FromFile,ToFile);
      }
    }
  }


static void movegif(char *fina)
  {
  int leng;
  FILE *fp;
  char giffile[MAXFILENAMELEN],giffile2[MAXFILENAMELEN];
  char ToName[MAXFILENAMELEN];
  int start[2];
  char *temp;

  if ((fp=fopen(fina,"rb"))==NULL)
    { errorbox("Error Opening Backdrop File!","(C)ontinue"); return; }
  fread(start,sizeof(int),2,fp);
  fgets(giffile,40,fp);
  fclose(fp);

  temp = strrchr(giffile,10);
  if (temp!=NULL) *temp=0;

  temp=strrchr(giffile,(int) '\\');
  if (temp==NULL) { temp=giffile; leng=strlen(giffile); }
  else { leng=strlen(giffile)-strlen(temp)+1; temp++; }
  sprintf(ToName,"%s%s",PathToSave,temp);  
  strcpy(giffile2,temp);
  CopyFile(giffile,ToName);
  leng=strlen(PathToSave); 
  if ((fp=fopen(strcat(PathToSave,&fina[4]),"wb"))==NULL)
    { errorbox("Trouble Writing Backdrop File!","(C)ontinue"); return; }
  fwrite(start,sizeof(int),2,fp);
  fprintf(fp,"%s",giffile2);
  fclose(fp);
  PathToSave[leng]=0;
  return;
  }
*/


