
// Turbo C subroutine that finds a file
#include <dos.h>
#include <dir.h>
#include <string.h>
#include <stdio.h>

#include "gen.h"
//#define STAND_ALONE

int FindDirs(char *dirspec,char *path,char *result,uint max,uint *Idx)
  {
  int addon = 0;
  int ctr   = 0;
  int ret   = 0;
  struct ffblk f;
  char pf[81];

  strcpy (pf,path);
  strcat (pf,dirspec);

  ret = findfirst(pf,&f,FA_DIREC);

  while ((ret==0)&&(addon+13<max))
    {
    boolean Clear=False;
    if (((f.ff_attrib&FA_DIREC)>0)&&(strcmp(f.ff_name,".") !=0))
      {
      for (int i=0;i<14;i++)  // Copy the Directory Name over, zero fill xtra space.
        {
        if (!Clear)
          {
          if (f.ff_name[i]==0) { *(result+addon+i)='\\'; Clear=True; }
          else *(result+addon+i) = f.ff_name[i];
          }
        else *(result+addon+i)=0;
        }
      addon += 13;
      ctr++;                  // ctr is the total # of files found.
      }
    ret = findnext(&f);
    }

  *Idx = addon;
  return(ctr);  // Fn returns # of file names found
  }

int FindFiles(char *filespec,char *path,char *result,uint max,uint *Idx)
  {
  int addon = 0;
  int ctr   = 0;
  int ret   = 0;
  struct ffblk f;
  char pf[81];

  strcpy (pf,path);
  strcat (pf,filespec);

  ret = findfirst(pf,&f,FA_ARCH|FA_RDONLY);    // find all files in this directory.
  while((ret==0)&&(addon+13<max))
    {
    boolean Clear=False;
    for (int i=0;i<14;i++)      // Copy the file over, zero fill xtra space.
      {
      if (!Clear)
        {
        *(result+addon+i) = f.ff_name[i];
        if (f.ff_name[i]==0) Clear=True;
        }
      else *(result+addon+i)=0;
      }
    addon += 13;
    ctr++;                     // ctr is the total # of files found.
    ret=findnext(&f);
    }
  *Idx = addon;
  return(ctr);  // Fn returns # of file names found

/*  strcpy (pf,path);       // Find all other directories

  strcat (pf,"*.");
  f.ff_attrib=FA_DIREC;

  ret = findfirst(pf,&f,16);

  while (ret==0)
    {
    if ((f.ff_name[0]!='.')&(f.ff_attrib==FA_DIREC))
      {
      sprintf(pf,"%s%s\\",path,f.ff_name);
      ctr+=dofindfile(pf,filespec,result+addon);
      }
    ret = findnext(&f);
    }
*/
  }



#ifdef STAND_ALONE
void main(void)
    {
    int l;
    int num;
    char *i;
    char filename[3000];
    i= &filename;
    num = FindFile("e:\\","*.bbs",filename);
    printf("< %d matches >\n",num);
    for (l=0;l<num;l++)
      {
      printf("%s\n",i);
      i += strlen(i)+1;
      }
    }
#endif



