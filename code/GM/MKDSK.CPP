#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <bios.h>
#include "gen.h"
#include "bytenums.h"
#define TRUE    1
#define FALSE   0
#define MAXFILENAMELEN  81

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


char upcase(char c);
char LoadSerial(char *from);
char SaveSerial(char *from);
int  ParseFileName(const char *in,char *name,char *path);
void insertstr(char *filename,char *str,long bytnum,int bytes);
char Register(char *path);
void CopyOver(char *src, char *dest,char disks);


unsigned long int CurCtr=0;
struct date Date;

main(int argc, char *argv[])
  {
  char done=0;
  char path[81];
  char temp[81];
  
  getdate(&Date);
  ParseFileName(argv[0],temp,path);
  if (!LoadSerial(path)) 
    {
    printf("The Serial # file has dissappeared!");
    exit(1);
    }

  if (argc<2)
    {
    printf("Syntax : mkdsk [src dir] (ie 'mkdsk d:\gm' or 'mkdsk c:\gm')\n");
    exit(1);
    }

  while (!done)
    {
    printf("Ready to make V"GMVER" disks! Hit 'a','b', b(o)th, or ESC to quit.\n");
    printf("Insert a new RSD-GM disk into whatever drives you're copying to.\n"); 
    done=upcase(bioskey(0)&255);
    if (done==27) done=1; 
    else if ((done=='A')||(done=='B')||(done=='O')) 
      {
      CopyOver(argv[1],argv[2],done); 
      done=0;
      }
    }
  SaveSerial(path);
  }


char Register(char *path)
  {
  int j;
  char done=0;
  FILE *fp;
  char cnum[81], fname[81], name[40],final[40];
  combination comb;
  unsigned long int checksum;  

  for (j=0;j<35;j++) { name[j]=0; final[j]=0; }

  comb.ser = (CurCtr*1029)+4;           // calculate the serial #
  checksum = ((unsigned long) rand() + ((unsigned long)rand() * 237));

  sprintf(cnum,"%lu",checksum);
  sprintf(fname,"%s\\playgame.reg",path);
  printf("\nRegistering %s as number %lu\n\n",path,comb.ser);
  
  if ((fp=fopen(fname,"wt"))==NULL)
    {
    printf("The File %s cannot be created!",fname);
    done=1;
    }
  else
    {  
    fprintf(fp,"%s\n%lu\n%s\n",name,comb.ser,cnum);
    fclose(fp);
    }
  if (!done)
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
  if (!done)
    {
    final[0]='Ü';
    for (j=1;j<29;j++) final[j]=j+64;
    // else for (j=1;j<29;j++) final[j]=(21+name[j-1]+5*j);
    final[29]=0;
    final[30]=comb.ord.fir;
    final[31]=comb.ord.thi;
    final[32]=comb.ord.fou;
    final[33]=comb.ord.sec;
    final[34]=0;
    sprintf(fname,"%s\\playgame.exe",path);
    insertstr(fname,final,PLAYBYTE,34);
    sprintf(fname,"%s\\xferplay.exe",path);
    insertstr(fname,final,XFERBYTE,34);
    }
  return(done);
  }

void insertstr(char *filename,char *str,long bytnum,int bytes)
  {
  FILE *fp;
  register int j;

  if ((fp = fopen(filename,"r+b")) == NULL)
    {
    printf("The File %s cannot be opened!",filename);
    }
  else
    {
    fseek(fp,bytnum,SEEK_SET);
    fwrite(str,bytes,1,fp);
    fclose(fp);
    }
  }


void CopyOver(char *src, char *dest,char disk)
  {
  char temp[81];
  
  if ((disk=='A')||(disk=='O'))
    {
    printf("------------------------COPYING TO THE A: DISK-------------------\n");
    sprintf(temp,"xcopy %s a:", src);
    system(temp);  
    Register("a:");
    }

  if ((disk=='B')||(disk=='O'))
    {
    printf("------------------------COPYING TO THE B: DISK-------------------\n");
    sprintf(temp,"xcopy %s b:", src);
    system(temp);  
    Register("b:");
    }
  CurCtr++;
  }

char LoadSerial(char *path)
  {
  char fname[81];
  FILE *fp;  

  sprintf(fname,"%sserial.dat",path);
  
  if ((fp=fopen(fname,"rt"))==NULL) return (0);
  fscanf(fp,"%lu",&CurCtr);
  return(1);
  }
  
char SaveSerial(char *path)
  {
  char fname[81];
  FILE *fp;  

  sprintf(fname,"%sserial.dat",path);
  
  if ((fp=fopen(fname,"wt"))==NULL) return (0);
  fprintf(fp,"%lu",CurCtr);
  return(1);
  }

  
int ParseFileName(const char *in,char *name,char *path)
  {
  int LastSlash=0;
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



char upcase(char c)
  {
  if ((c>='a')&&(c<='z')) return(c-('a'-'A'));
  return(c);
  }
