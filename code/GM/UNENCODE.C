#include <stdio.h>


void main(void)
  {
  unsigned char encodedat[1510];
  unsigned char count=0;
  unsigned char lin,wor;
  unsigned int head,count2=0;
  unsigned char answer[14] = "",temp[10] = "";
  register int i;
  char str[25];
  FILE *fpout;
  
  if ((fpout=fopen("encoded.dat","rb"))==NULL) printf ("Error!\n");
  else
    {
    fread(encodedat,1,970,fpout);
    fclose(fpout);
    if ((fpout = fopen ("copytect.dat","wt")) != NULL)
      {
      for (count=1;count<88;count++)    // (random(87)+91);
        {
        count2=0;
        while ( (encodedat[count2] != count) && (count2<969) )
          {
          if (encodedat[count2] < 88)
            {                    /* eliminate possibility of head(int) bytes */
            count2+=5;
            } 
          else count2++;  
          }
        if (count2>=970)
          {
          printf("Data Corrupted!  ,  Reinstall from original disks.");
          break;
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
        sprintf((char *)temp,"%d",head);
        i=0;
        while (temp[i] != 0) i++;
        temp[2*i-1]=0;
        while (i != 0)
          {
          i--;
          temp[2*i]=temp[i];
          temp[2*i-1]='.';
          }
        printf("Section: %-10s  ",temp);
        printf("Line: %2d  ",lin);
        printf("Word: %2d\n",wor);
        printf("Answer: %s\n",(char *)answer);
        }
      fclose(fpout);
      }
    }
  }
