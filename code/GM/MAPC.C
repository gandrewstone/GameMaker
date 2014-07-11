/*----------------MAP specific routines--------------------------------*/
/* Andy Stone            JULY 2,1991                                   */
/* Last Edited: 7/2/91                                                 */
/*---------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "gen.h"
#include "gmgen.h"
#include "scrnrout.h"
#include "mousefn.h"
#include "windio.h"
#include "map.h"

#define sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))

int loadmap(void);
int savemap(void);

extern mapstruct       map[MWID][MLEN];
extern monmapstruct    mm[LASTMM];


static char ext[]="*.map\0";
char mapcurfile[41]="";

int loadmap(void)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN]="";
 
  if (!getfname(5,7,"Enter map file to load:",ext,fname)) return(FALSE);
  fp=fopen(fname,"rb"); // must exist because getfname searches for it.
  fread( (char*) &map,sizeof(mapstruct),MLEN*MWID,fp);
  fread( (char*) &mm,sizeof(monmapstruct),LASTMM,fp); 
  fclose(fp);
  strcpy((char*)mapcurfile,(char*)fname);
  return(TRUE);
  }

int savemap(void)
  {
  FILE *fp;

//#ifdef CRIPPLEWARE
//  StdCrippleMsg();
//#else
  if ( (fp=GetSaveFile("Save map as:",".map",WorkDir,mapcurfile))==NULL) return(False);

  fwrite( (char*) &map,sizeof(mapstruct),MLEN*MWID,fp);
  fwrite( (char*) &mm,sizeof(monmapstruct),LASTMM,fp); 
  fclose(fp);
//#endif
  return(TRUE);
  }
