//ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ|
// update.C       Registration 2.0 registers PLAYGAME.EXE |
//                for both phone and commercial orders.   |
// By: Oliver     For commercial orders, leave the name   |
//     Stone      Blank (Just press <ENTER>).             |
// Last Edited: Andy Stone 1/28/93                        |
//ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ|

#include<dos.h>
#include<stdio.h>
#include<conio.h>
#include<stdlib.h>
#include<time.h>
#include<bios.h>
#include "gen.h"
#include "bytenums.h"

void insertstr(char *filename,char *str,long bytnum,int bytes);
static char GetSerialNum(unsigned long int *num);
char WriteReg(char *name, char *num, char *encoded,char drive);

struct date Date;

typedef union 
  {
  unsigned long ser;
  struct 
    {
    char fir;
    char sec;         /* to be written in order 1,3,4,2 */
    char thi;         /* ser-4 must be a multiple of 1029 */
    char fou;
    } ord;
} combination;

void main(void)
 {
  register int  j;
  char          name[31];            /* user name                      */
  char          num[12];             /* serial number string for f f   */
  char          cnum[12];             /* check sum string for fake file */
  char          final[35];        /* final form of info to go to playgame */
  combination   comb;
  char          doover=0;   /* re-get serial number if entered wrong */
  unsigned long int checksum;    /* name/number verification number */
  FILE          *fp;
  char          done=0,skip=0;
  char          drive;
  char          fname[30];
  unsigned long int first=0,second=0,third=0;

  getdate(&Date);

  do
    {
    do
      {  
      doover=0;
      clrscr();
      gotoxy(13, 9);  printf("ษออออออออออออออออออออออออป");
      gotoxy(13,10);  printf("บ GM V"GMVER" Update Programบ");
      gotoxy(13,11);  printf("ฬออออออออออออออออออออออออน");
      gotoxy(13,12);  printf("บDisk a,b, or b(o)th:    บ");
      gotoxy(13,13);  printf("ศออออออออออออออออออออออออผ");
      drive = getch();
      switch (drive) 
        {
        case 'o': case 'O':
          drive='O';
        case 'a': case 'A':
        case 'b': case 'B':  break;
        case  13:
        case  27:            done=1; break;
        default :            doover=1; break;
        }
      } while ( (done==0) && (doover==1) );
    if (!done) 
      {
      int i=0;
      num[0]=0;
      gotoxy(13,11);  printf("ศออออออออออออออออออออออออผ");
      gotoxy(13,12);  printf("ษอออออออออออออออออออออออออออออออออออออออออออออออออออป");
      gotoxy(13,13);  printf("บEnter customer name:                               บ");
      gotoxy(13,14);  printf("ศอออออออออออออออออออออออออออออออออออออออออออออออออออผ");
      gotoxy(35,13);
      for (i=0;i<31;i++) name[i]=0;             // Initialize name
      gets(name);
      if (name[0]==0) skip = 1;
      else
        {
        j=0;
        while (name[j]!=0) j++;
        while (j<30) { name[j]=0; j++; }
        }
      }
    if (!done) 
      {
      done = GetSerialNum(&comb.ser);
      sprintf(num,"%lu",comb.ser);
      }
    if (!done)
      {
      if (!skip) 
        {
        first=0; second=0; third=0;
        for (j=0;j<7;j++) first  +=(name[j]&255);
        for (j=7;j<18;j++) second+=(name[j]&255);
        for (j=18;j<30;j++) third+=(name[j]&255);
        checksum = (comb.ser*first) + (second*1143) + (third ^ 0x16AC);
        }
      else checksum = ((unsigned long) rand() + ((unsigned long)rand() * 237));
      sprintf(cnum,"%lu",checksum);

      // Write Registration info
      if (drive=='O') 
        {
        done = !(WriteReg(name,num,cnum,'a'));
        if (!done) done = !(WriteReg(name,num,cnum,'b'));
        }
      else done = !(WriteReg(name,num,cnum,drive));
      }

    if (!done)                  // Write to Playgame and Xferplay
      {
      final[0]='Ü';
      if (skip) for (j=1;j<29;j++) final[j]=j+64;
      else for (j=1;j<29;j++) final[j]=(21+name[j-1]+5*j);
      final[29]=0;
      final[30]=comb.ord.fir;
      final[31]=comb.ord.thi;
      final[32]=comb.ord.fou;
      final[33]=comb.ord.sec;
      final[34]=0;
      if (drive == 'O')
        {
        insertstr("a:playgame.exe",final,PLAYBYTE,34);
        insertstr("a:xferplay.exe",final,XFERBYTE,34);
        insertstr("b:playgame.exe",final,PLAYBYTE,34);
        insertstr("b:xferplay.exe",final,XFERBYTE,34);
        }
      else
        {
        sprintf(fname,"%c:playgame.exe",drive);
        insertstr(fname,final,PLAYBYTE,34);
        sprintf(fname,"%c:xferplay.exe",drive);
        insertstr(fname,final,XFERBYTE,34);
        }        
      }


    if (!done)          // Write registration log file.
      {
      if ((fp=fopen("regist.log","a+t")) == NULL)
        {
        printf("The File %s cannot be added to!",fname);
        done=1;
        }
      else
        {
        fprintf(fp,"%12lu %12lu %30s %2d/%2d/%4d V"GMVER"\n",comb.ser,checksum,name,Date.da_mon,Date.da_day,Date.da_year);
        fclose(fp);
        }
      }
    } while (!done); 
  }

void insertstr(char *filename,char *str,long bytnum,int bytes)
  {
  FILE *fp;
  register int j;

  if ((fp = fopen(filename,"r+b")) == NULL)
    {
    printf("The File %s cannot be opened!",filename);
    bioskey(0);
    }
  else
    {
    fseek(fp,bytnum,SEEK_SET);
    fwrite(str,bytes,1,fp);
    fclose(fp);
    }
  }

char WriteReg(char *name, char *num, char *encoded,char drive)
  {
  FILE *fp;
  char fname[81];
  sprintf(fname,"%c:playgame.reg",drive);
  if ((fp=fopen(fname,"wt"))==NULL)
    {
    printf("The File %s cannot be created!",fname);
    return(0);
    }
  else
    {  
    fprintf(fp,"%s\n%s\n%s\n",name,num,encoded);
    fclose(fp);
    return(1);
    }
  }

static char GetSerialNum(unsigned long int *num)
  {
  char str[81];
  char done=0,doover=0;

  do {
    doover=0;
    gotoxy(13,12);  printf("ษอออออออออออออออออออออออออออออออออออออออป            ");
    gotoxy(13,13);  printf("บEnter serial number on box:            บ            ");
    gotoxy(13,14);  printf("ศอออออออออออออออออออออออออออออออออออออออผ            ");
    gotoxy(42,13);
    gets(str);
    if (str[0] == 0) done = 1;
    else {
      sscanf(str,"%lu",num);
      if ( (*num-4)%1029 != 0 ) {
        clrscr();
        gotoxy(11,11); printf("*************************************");
        gotoxy(11,12); printf("* This in an invalid serial number! *");
        gotoxy(11,13); printf("*    Recheck it and re-enter it.    *");
        gotoxy(11,14); printf("*************************************");
        doover=1;
        while ( !bioskey(1) );
        bioskey(0); clrscr();
        }
      }
    } while ( (doover==1) && (done==0) );
  return (done);
  }

