
// Turbo C subroutine that finds a file
#include <dos.h>
#include <dir.h>
#include <string.h>

void main(void)
  {
  int l;
  int num;
  char *i;
  char filename[3000];
  i= &filename;
  num = dofindfile("e:\\","*.bbs",filename);
  printf("< %d matches >\n",num);
  for (l=0;l<num;l++)
    {
    printf("%s\n",i);
    i += strlen(i)+1;
    }
  }

int dofindfile(char path[80],char *filespec,char *result)
  {
  static int addon=0;
  int ctr=0;
  struct ffblk f;
  int ret=0;
  char pf[80];

  strcpy (pf,path);
  strcat (pf,filespec);

     /* find all files in this directory  */
  ret = findfirst(pf,&f,15);
  while(ret==0)
    {
    sprintf((result+addon),"%s%s\0",path,f.ff_name);
    addon += strlen(result+addon)+1;
    ctr++;                     /* ctr is the # of files found total. */
    ret=findnext(&f);
    }
    /* Find all other directories */
  strcpy (pf,path);
  strcat (pf,"*.");
  f.ff_attrib=FA_DIREC;

  ret = findfirst(pf,&f,16);

  while (ret==0)
    {
    if ((f.ff_name[0]!='.')&(f.ff_attrib==FA_DIREC))
      {
      sprintf(pf,"%s%s\\",path,f.ff_name);
      ctr+=dofindfile(pf,filespec,result);
      }
    ret = findnext(&f);
    }
  return(ctr);  /* Fn returns # of file names found */
  }
