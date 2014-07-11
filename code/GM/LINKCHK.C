#include<stdio.h>
#include<stdlib.h>
#include<conio.h>
#include<bios.h>


void insertstr(char *str,long bytnum,int bytes);
char FindByteNum(long int *byten);

void main(void)
  {
  register int j;
  char         redata[1000];         /* registration data              */
  char         temp[2];
  FILE         *fp;
  int          done=0;
  long         byten=0;

  clrscr();
  
  printf("ษออออออออออออออออออออออออป\nบ GM  Data Registration  บ\nศออออออออออออออออออออออออผ\n");
  if (!FindByteNum(&byten)) { printf ("Error - Can't find place!"); exit(0); }

  if ((fp=fopen("encoded.dat","rb"))==NULL)
    {
    printf("\ntrouble with encoded.dat\n");
    exit(1);
    }
  j=0;
  while (!feof(fp))
    {
    redata[j]=fgetc(fp);
    j++;
    }
  fclose(fp);
  printf("\n%d\n",j);
  printf("%ld\n",byten);
  insertstr(redata,byten,j-1);
  }

void insertstr(char *str,long bytnum,int bytes)
 {
  FILE *fp;
  register int j;

  if ((fp = fopen("chk.exe","r+b"))==NULL)
    {
    printf("\nTrouble opening chk.exe");
    return;
    }
  fseek(fp,bytnum,SEEK_SET);
  fwrite(str,bytes,1,fp);
  fclose(fp);
 }


char FindByteNum(long int *byten)
  {
  char Find[]="Put the copy protection here!";
  int end;
  int at=0;
  char CurChar;
  unsigned long int ctr=0;
  FILE *fp;
 
  end=strlen(Find); 
  printf("Searching for '%s' - length %d\n",Find,end);
 
  if ((fp = fopen("chk.exe","rb"))==NULL)
    {
    printf("ERROR - NO CHK.EXE!");
    exit(0);
    }
  while ((at<end)&&(feof(fp)==0))
    {
    if (at==0) *byten=ctr;
    if ((CurChar=fgetc(fp))==Find[at]) at++;
    else at=0;
    ctr++;
    }
  fclose(fp);
  if (at==end) return(1);
  return(0);
  }

