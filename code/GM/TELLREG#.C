/*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
/* TellREG#.C       ณThis file returns the registration    */
/*                  ณnumber to send to customers who want  */
/* By: Oliver Stone ณto register.  Send it with their name */
/*                  ณand serial number to them.            */
/*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤมฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/

#include <dos.h>
#include<stdio.h>
#include<conio.h>
#include "gen.h"
#define ESC 27

void printstring(char *str);
void printchar(char ch);
void PrintInfo(char *name, unsigned long int sernum, unsigned long int RegNum);
void SaveInfo(char *name, unsigned long int sernum, unsigned long int RegNum);

struct date Date;

void main(void)
 {
  register int  j;
  char          name[30];            /* user name                      */
  char          num[12];             /* serial number string for f f   */
  unsigned long int sernum,checksum; /* serial #, name/number verification number */
  char          done=0,doover=0;
  unsigned long int first=0,second=0,third=0;

  getdate(&Date);

  do
    {
    clrscr();
    gotoxy(13, 9);  printf("ษออออออออออออออออออออออออออออป");
    gotoxy(13,10);  printf("บ GM V"GMVER" Tell Registration บ");
    gotoxy(13,11);  printf("ศออออออออออออออออออออออออออออผ");
    name[0]=0; num[0]=0;

    gotoxy(13,11);  printf("ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป");
    gotoxy(13,12);  printf("บEnter customer name (Return quits):                               บ");
    gotoxy(13,13);  printf("ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ");
    gotoxy(50,12);
    gets(name);
    if (name[0]==0) done = 1;
    else
      {
      j=0;                                
      while (name[j]!=0) j++;
      while (j<30) { name[j]=0; j++; }
      name[29]=0;
      }
    if (!done)
      {
      do {
        doover=0;
        gotoxy(13,11);  printf("ษอออออออออออออออออออออออออออออออออออออออป                                      ");
        gotoxy(13,12);  printf("บEnter user's serial number:            บ                                      ");
        gotoxy(13,13);  printf("ศอออออออออออออออออออออออออออออออออออออออผ                                      ");
        gotoxy(42,12);
        gets(num);
        if (num[0] == 0) done = 1;
        else
          {
          sscanf(num,"%U",&sernum);
          if ( (sernum-4)%1029 != 0)
            {
            clrscr();
            gotoxy(11,11); printf("*************************************");
            gotoxy(11,12); printf("* This in an invalid serial number! *");
            gotoxy(11,13); printf("*    Recheck and then re-enter it.  *");
            gotoxy(11,14); printf("*************************************");
            doover=1;
            getch();
            clrscr();
            }
          }
        } while ( (doover==1) && (done==0) );
      }
    if (!done)
      {
      first=0;second=0;third=0;
      for (j=0;j<7;j++)   first += (name[j]&255);
      for (j=7;j<18;j++)  second+= (name[j]&255);
      for (j=18;j<30;j++) third += (name[j]&255);
      checksum = (sernum*first) + (second*1143) + (third ^ 0x16AC);
      gotoxy(30,20);
      PrintInfo(name,sernum,checksum);
      SaveInfo(name,sernum,checksum);
      printf("Registration number is: %lu",checksum);
      }
    } while (!done);
  }

void SaveInfo(char *name, unsigned long int sernum, unsigned long int RegNum)
  {
  FILE *fp;

  if ((fp=fopen("tellreg.log","a+t")) == NULL)
    printf("The File tellreg.log cannot be added to!");
  else
    {
    fprintf(fp,"%12lu %12lu %30s %2d/%2d/%4d V"GMVER"\n",sernum,RegNum,name,Date.da_mon,Date.da_day,Date.da_year);
    fclose(fp);
    }
  }

void PrintInfo(char *name, unsigned long int sernum, unsigned long int RegNum)
  {
  int x;
  char temp[201];

  printchar(ESC);
  printchar('E');
  printchar(ESC);
  printstring("(10U");           // Ascii characters
  printchar(ESC);
  printstring("(s14V");           // 12 points

  printchar(ESC);
  printstring(")10U");           // Ascii characters
  printchar(ESC);
  printstring(")s14V");           // 12 points
  printchar(ESC);
  printstring("&l0O");    // print orientation 0=norm 1=portrait 2=rev norm 3= rev port 
  for(x=0;x<59;x++) printstring("\n\r");

  printstring( "      ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ\n\r");
  sprintf(temp,"      ณ           User Name: %30s           ณ\n\r",name);
  printstring(temp);
  sprintf(temp,"      ณ       Serial Number: %30lu           ณ\n\r",sernum);
  printstring(temp);
  sprintf(temp,"      ณ Registration Number: %30lu           ณ\n\r",RegNum);
  printstring(temp);
  printstring( "      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู\n\r");
  printstring("\n\r");
  printchar(ESC);      // reset printer command 
  printchar('E');      // ie. Form Feed
  }

void printstring(char *str)
  {
  register int loop;

  for (loop=0;str[loop] != 0; loop++)
    {
    printchar(str[loop]);
    }
  }

void printchar(char ch)
  {
  _AH = 0x5;
  _DL = ch;
  geninterrupt(0x21);
  }
