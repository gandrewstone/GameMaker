#include <stdio.h>
#include "gen.h"
#include "scrnrout.h"
#include "windio.h"
#include "bytenums.h"

static unsigned long GetMoreData(void);


void main(void)
  {
  register int  j;
  char          name[31];            // user name
  char          num[12];             // serial number string for f f
  char          cnum[12];            // check sum string for fake file
  FILE          *fp;
  char          done=0;

  clrbox(0,1,79,23,1);
  drawbox (27,5,51,7,1,2,8);
  writestr(29,6,2,"GM "GMVER" Registration");
  name[0]=0; num[0]=0; cnum[0]=0;
  if ((!qwindow(5,14,30,"Enter your name (Exactly as on card): ",name))||(name[0]==0))
    {
    errorbox("ABORTING REGISTRATION","(O)k");
    done = 1;
    }
  else                          // zero the rest of the name.
    {
    j=0;                                
    while (name[j]!=0) j++;
    while (j<30) { name[j]=0; j++; }
    name[29]=0;
    }

  if (!done)
    {
    unsigned long int temp=GetMoreData();
    if (temp==0)
      {
      errorbox("Please run this program from","your GameMaker directory.");
      done=TRUE;
      }
    sprintf(num,"%lu",temp);
    }
  if (!done)
    {
    if ((!qwindow(13,14,12,"Enter registration number from card: ",cnum))||(cnum[0] == 0))
      {
      errorbox("ABORTING REGISTRATION","(O)k");
      done = 1;
      }
    }
  if (!done)
    {
    fp=fopen("playgame.reg","wt");
    fprintf(fp,"%s\n%s\n%s\n",name,num,cnum);
    fclose(fp);
//    errorbox("REGISTRATION   COMPLETE","Hit any key to start GM");
    TextMode();
    printf("Registration Finished\n");
    }
  }

#include <dos.h>
#define TMODE 3
void TextMode(void)
  {
  union REGS r;

  r.h.ah = 0;
  r.h.al = TMODE;
  int86(0x10,&r,&r);
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


