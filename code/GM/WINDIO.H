/*  windio.c header file  by Andy Stone Dec 26,1990 */
#include "gmgen.h"
#include <stdio.h>

#ifdef CRIPPLEWARE
void StdCrippleMsg(void);
#endif

extern int windlvl;
extern char CritVal;

char interrupt far NewCritError(...);

int  getfname(int x,int y,const char *q,const char *fspec,char *ans);
  /* stick default path in ans */
void closemenu(int x,int y,int len,int wid,char *buffer=NULL);
int openmenu(int x,int y,int lenx,int widy,char *w=NULL);
     /* returns the window's background attribute (already multiplied) */

int errorbox(char *l,char *l1,unsigned int maxpause=30000);
  /* returns the character selected.       ^^OPTIONAL ARGUEMENT */

int horizmenu(int numitems, int curitem, int y,...); /* ... is x of each item and final x of end */
int vertmenu(int x,int y,int lenOFitems,int numitems, int curitem);
      /* returns the item number selected, first item is # 1 */

int qwindow(int x,int y,int len, char *q,char *ans);
     /* question window - returns answer in 'ans'.  Returns TRUE if  */
     /* questioned answered, FALSE if user hit ESC */

#define REMNAME 1
       /* remember file name in case saved */
int   loadany(char *prompt,char *ext,char *path,unsigned int bytes,char *buffer,int cansave);
int   saveany(char *prompt,char *ext,char *path,unsigned int bytes,char *buffer);
      /* returns TRUE - load successful, or FALSE - load unsuccessful */
int   delany(const char *prompt,const char *fspec,char *path);

int   getstr(int x,int y, int maxlen, char *ans);
FILE *GetSaveFile(char *prompt,char *ext,char *path,char *curfil);

#define FOREATTR    14
#define BACKATTR    1
#define MENUATTR    (FOREATTR+(BACKATTR*16))
#define MENUBOX     2
#define SHADATTR    7*16
#define ERRORFORE   12
#define ERRORBACK   4
#define ERRORATTR   (ERRORFORE+(ERRORBACK*16))
#define CURATTR     (7*16)+15
#define BASE        8

// End windio.c header file
