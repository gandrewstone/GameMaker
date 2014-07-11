//------------------------------------------------------------------
// Game Maker Menu Program              by Ollie and Andy Stone    |
// Commercial Version                                              |
// Last Edited:  7-27-92  (Andy Stone)                             |
//------------------------------------------------------------------
#include "gen.h"

#include <process.h>
#include <stdio.h>
#include <bios.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#define CHKREG
//#undef CHKREG
#ifdef CHKREG
#include <io.h>
#endif

#include "mousefn.h"
//#include "jstick.h"
#include "scrnrout.h"
#include "windio.h"
#include "pal.h"
#include "bytenums.h"
#define C(x,y) (x+(y*16))

static void gmmain(void);

static void firststart(void);
//static void secondstart(char *initdata);
static char GetRegistData(char *name,unsigned long *num);
static unsigned long GetMoreData(void);
static void Unwravel(char *name);
static char compare_em(char *name1,char *name2,unsigned long One,unsigned long Two);
static void manualcheck(char skip);
static void SetDate(void);
static void regi(void);

char        UserName[41];

/*
char        Progs[][9] =
  {
  "utility", "palchos", "blocedit", "monedit", "mapmaker",
  "charedit", "image", "sndedit", "grator", "playgame"
  };
*/

void main(int argc,char *argv[])
  {
  if (initmouse()==FALSE)
    { 
    errorbox("GameMaker requires a Microsoft","compatible mouse! (Q)uit",30);
    exit(quit);
    }
#ifndef DEBUG
  Palette(5,10,10,10);
  Palette(7,16,16,16);
#endif
  mouclearbut();
  randomize();

#ifdef CHKREG
  if ((argc >= 2)&&(strcmpi(argv[1],"REGISTER") == 0)) regi();
  firststart();
#else
  gmmain();
#endif
  Palette(5,42,0,42);
  Palette(7,42,42,42);
  clrbox(0,0,79,24,7);
  exit(quit);
  }

static void gmmain(void)
  {
//  char str[3];
//  str[0]='M';
//  str[1]=1;
//  str[2]=0;

  drawbox (14,11,32+strlen(UserName),13,1,2,8);
  writestr(16,12,2,"Registered To: ");
  writestr(31,12,2,UserName);
  exit(REGOK);
//  execl(MENUPROG,MENUPROG,str,OKMSG,NULL);
//  printf("Unable to run overlay file m.exe.\n");
  }


#ifdef CHKREG
static void manualcheck(char skip)
  {
  static unsigned char encodedat[1510]="Put the copy protection here! ";
  unsigned char count=0;
  unsigned char lin,wor;
  unsigned int head,count2=0;
  unsigned char answer[14] = "",answeren[14]= "",temp[10] = "";
  register int i;
  char str[25];
  FILE *fp;
  
  count=(random(87)+91);
  if (skip)
    {
    gmmain();
    return;
    }
// Why chmodded?
/*  if (chmod("playgame.exe",S_IREAD|S_IWRITE))
    {
    if (errno == ENOENT) errorbox("Please run this application in the GM directory","                 (E)xit");
    else errorbox("Cannot Run PLAYGAME.EXE","     (E)xit");
    return;
    }
*/
  if ((fp = fopen ("playgame.exe","r+b")) == NULL)
    {
    errorbox("This application must be run from","the GM directory!     (E)xit");
    return;
    }
  fseek(fp,PLAYBYTE,SEEK_SET);
  count = fgetc(fp);
  count-=90;
  if ( (count<1) || (count>88) ) count=(random(87)+1);
  fseek(fp,PLAYBYTE,SEEK_SET);
  count2=(random(87)+91);
  if (fputc(count2,fp) == EOF)
    {
    errorbox("Cannot Find PLAYGAME.EXE","     (E)xit");
    fclose(fp);
    return;
    }
  fclose(fp);
  SetDate();
  count2=0;
  while ( (encodedat[count2] != count) && (count2<970) )
    {
    if (encodedat[count2] < 88)
      {                    /* eliminate possibility of head(int) bytes */
      count2+=5;
      }
    else count2++;  
    }
  if (count2>=970)
    {
    errorbox("Data Corrupted!","Reinstall from original disks.");
    return;
    }
  lin=encodedat[count2+1]-101;
  wor=encodedat[count2+2]-150;
  head=(*(unsigned int *) (&encodedat[count2+3]))/10;
  count2+=5; i=0;
  while(encodedat[count2+i]!=(count+1))
    {
    answer[i] = (encodedat[count2+i]-11-(count%57))/2;
    i++;
    }
  answer[i]=0;
  sprintf((char far *)temp,"%d",head);
  i=0;
  while (temp[i] != 0) i++;
  temp[2*i-1]=0;
  while (i != 0)
    {
    i--;
    temp[2*i]=temp[i];
    temp[2*i-1]='.';
    }
  count2 = openmenu(14,3,50,10,(char far *)encodedat);
  for (i=0;i<3;i++)
    {
    writestr(15,3,count2+15,"   Please enter the following informantion.");
    writestr(15,4,count2+14,"       OR send in your registration card");
    writestr(15,5,count2+14,"             to skip this hassle!");
    writestr(15,7,count2+15,"Count every line with text, not including");
    writestr(15,8,count2+15,"numbered headings. (\"Game-maker\" is one word).");
    sprintf(str,"Section: %s",temp);
    writestr(15,10,count2+14,str);
    sprintf(str,"Line: %d",lin);
    writestr(15,11,count2+14,str);
    sprintf(str,"Word: %d",wor);
    writestr(15,12,count2+14,str);
    qwindow(24,15,15,"Word Found: ",(char far *)answeren);
    if (stricmp((char far *)answeren,(char far *)answer) != 0)
      {
      if (i<2) 
        {
        errorbox("THAT IS INCORRECT","(R)etry");
        answeren[0]=0;
        }
      else
        {
        closemenu(14,3,50,10,(char far *)encodedat);
        errorbox("THAT IS INCORRECT","(E)xit");
        if ((fp = fopen ("playgame.exe","r+b")) == NULL)
          {
          errorbox("This application must be run from","the GM directory!     (E)xit");
          return;
          }
        fseek(fp,PLAYBYTE,SEEK_SET);
        count+=90;
        if (fputc(count,fp) == EOF)
          errorbox("Cannot Find PLAYGAME.EXE","     (E)xit");
        fclose(fp);
        SetDate();
        return;
        }
      }
    else i=3;
    }
  closemenu(14,3,50,10,(char far *)encodedat);
  gmmain();
  }

static void SetDate(void)
  {
  struct ftime filet;
  FILE *fp;

  if ((fp = fopen ("playgame.exe","r+b")) == NULL)
    {
    errorbox("Cannot Find PLAYGAME.EXE","     (E)xit");
    return;
    }
  filet.ft_tsec=00;
  filet.ft_min=04;
  filet.ft_hour=2;
  filet.ft_day=11;
  filet.ft_month=11;
  filet.ft_year=11;
  setftime(fileno(fp), &filet);
  fclose(fp);
  }

static void regi(void)
  {
  char          ErrorMsg[]="Aborting Registration";
  char          Emsg1[]   ="(O)k";
  register int  j;
  char          name[31];            // user name
  char          num[12];             // serial number string for f f
  char          cnum[12];            // check sum string for fake file
  FILE          *fp;
  unsigned long int temp;

  clrbox(0,1,79,23,1);
  drawbox (27,5,51,7,1,2,8);
  writestr(29,6,2,"GM V1.06 Registration");
  writestr(0,0,C(15,4),GMTOPBAR);
  writestr(0,1,C(15,1),"     Play        Design      Utilities      About        Quit         Help      ");
  name[0]=0; num[0]=0; cnum[0]=0;
  if ((!qwindow(5,14,30,"Enter your name (Exactly as on card): ",name))||(name[0]==0))
    {
    errorbox(ErrorMsg,Emsg1);
    return;
    }
  else                          // zero the rest of the name.
    {
    j=0;                                
    while (name[j]!=0) j++;
    while (j<30) { name[j]=0; j++; }
    name[29]=0;
    }

  temp=GetMoreData();
  sprintf(num,"%lu",temp);
  if ((!qwindow(13,14,12,"Enter registration number from card: ",cnum))||(cnum[0] == 0))
    {
    errorbox(ErrorMsg,Emsg1);
    return;
    }

  fp=fopen("playgame.reg","wt");
  fprintf(fp,"%s\n%s\n%s\n",name,num,cnum);
  fclose(fp);
  errorbox("REGISTRATION   COMPLETE","Hit any key to start GM");
  }

static char GetRegistData(char *name,unsigned long int *num)
  {
  unsigned long int num2;
  FILE *fp;
  register int j;
  unsigned long int first=0,second=0,third=0;


  if ( (fp=fopen("playgame.reg","rt")) == NULL) return(0);  // Missing File
  else
    {
    for (j=0;j<31;j++) name[j]=0;
    fgets(name,30,fp);                   // Get the user name.
    name[strlen(name)-1]=0;              // Cut off the ending newline character
    fscanf(fp,"%lu\n%lu\n",num,&num2);   // Get other info.
    fclose(fp);

    for (j=0;j<7;j++)   first+= (name[j]&255);   // Do Encoding scheme
    for (j=7;j<18;j++) second+= (name[j]&255);
    for (j=18;j<30;j++) third+= (name[j]&255);    
    if (num2 == (( (*num)*first)+(second*1143)+(third^0x16AC)) ) return(2);
    else { name[0]=0; return(1); }
    }
  }

static unsigned long int GetMoreData(void)
  {
  FILE *fp;
  union 
   {
   unsigned long int ser;
   struct 
     {
     char fir;
     char sec;         // to be read in order 1,3,4,2      
     char thi;         // ser-4 must be a multiple of 1029 
     char fou;
     } ord;
  } comb;

  comb.ser=0;
  if ( (fp=fopen("playgame.exe","rb"))==NULL) return(0);

  fseek(fp,(PLAYBYTE+30),SEEK_SET);             // Go to registration info bytes
  comb.ord.fir=fgetc(fp);                       // Get the bytes which make up the long int
  comb.ord.thi=fgetc(fp);
  comb.ord.fou=fgetc(fp);
  comb.ord.sec=fgetc(fp);
  fclose(fp);
  return(comb.ser);
  }  

static void firststart(void)
  {
  unsigned long int num[2];
  char registered;
  register int j;
  
  clrbox(0,0,79,23,0);
  if ( (registered = GetRegistData( UserName, &(num[0]) )) == 0 )
    {
    errorbox("Missing File:  PLAYGAME.REG","(Q)uit");
    }
  else
    {
    num[1] = GetMoreData();
    if (num[0]!=num[1]) errorbox("Bad Registration File!","(Q)uit");
    else
      {
      if (UserName[0]==0) sprintf(UserName,"%lu",num[0]);
      if (registered==2) manualcheck(1);
      else manualcheck(0);
      }
    }
  }

/*  This will put the username in Menu if we want it.
static void secondstart(char *initdata)
  {
  unsigned long num;

  if (GetRegistData(UserName,&num)==0)
    errorbox("Missing File:  PLAYGAME.REG","(Q)uit");
  else
    {    
    if (UserName[0]==0) sprintf(UserName,"%lu",num);
    gmmain();
    }
  }
*/
#endif

