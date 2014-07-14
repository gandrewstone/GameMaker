/*-------------------------------------------------------------------------*/
/* soundc.c    Sound Blaster Voice File Interface routines                 */
/*                                                                         */
/* Author:Peter Savage                   Created:02-Jan-1993               */
/*-------------------------------------------------------------------------*/

#include <alloc.h>
#include <io.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "gen.h"
#include "gmgen.h"
#include "jstick.h"
#include "windio.h"
#include "sound.h"

static char far * pointertodriver   =0; // Declare driver in memory pointer
static char far * pointertovoice    =0; // Declare voice file pointer
static char far * pointertodrivermem=0; // Declare driver in memory pointer
static ulongi vocfilesize           =0; // Declare variable for file size
static boolean driverloaded     =FALSE; // Declare driver not loaded variable
static unsigned int soundcardstatus =0; // Declare variable for SB card status

static void WaitSbVocComplete(void);    // Function to wait sound completion

static char ParseBlasterEnv(unsigned int *portaddress, unsigned int *intaddress);
static char curfile[MAXFILENAMELEN];
static char SetupAndTestCard(unsigned int portaddress, unsigned int intaddress);

int DetectCard(SoundCards *s,unsigned int *Port, unsigned int *Interrupt, char *drvr)
  {
  static char DefaultDrvr[]="sndblast.drv"; // Declare voice file driver name
  char * soundpointer=0;                // Declare variable for sound path

//  soundpointer=getenv("SOUND");     // Fetch location of SB software tree
//  if (soundpointer==0) strcpy(drvr,".\\"); // Was SB software tree found?
//  else strcpy(drvr,soundpointer);   // yes, Copy location of SB tree disk path

  // Use the standard GameMaker Driver.
  strcpy(drvr,".\\");
  strcat(drvr,DefaultDrvr);         // Add driver sub tree and file name

  if (ParseBlasterEnv(Port,Interrupt))
    {
    *s=SndBlaster;
    return(TRUE);
    }
  return(FALSE);
  }


char InitSbVocDriver(ConfigStruct *cs)
  {

  int filehandle=0;                     /* Declare a file handle */

  unsigned int driversegment=0;         /* Declare variable for driver segment*/
  unsigned int driveroffset=0;          /* Declare variable for driver offset */
  unsigned int statussegment=0;         /* Declare variable for status segment*/
  unsigned int statusoffset=0;          /* Declare variable for status offset */
  unsigned long driversize=0;           /* Declare variable for file size */

  if (driverloaded==TRUE)               /* Have we already loaded the driver */
    {
    return(TRUE);                       /* Driver is loaded exit with truth */
    }

  statussegment=FP_SEG(&soundcardstatus); /* Fetch status word segment addres*/
  statusoffset=FP_OFF(&soundcardstatus); /* Fetch status word offset address */

  filehandle=open(cs->SndDrvr,O_RDONLY|O_BINARY); // Open the voice driver file
  if (filehandle==-1) return(FALSE);     // On driver open error exit

  driversize=filelength(filehandle);    /* Get the size of the file in bytes */
  pointertodrivermem=(char far *) farmalloc(driversize+16l);
  if (pointertodrivermem==0)
    {
    close(filehandle);                  /* Close the driver file */
    return(FALSE);                      /* Can't allocate driver memory, exit */
    }
  driversegment=FP_SEG(pointertodrivermem); /* Get driver memory block segment*/
  driveroffset=FP_OFF(pointertodrivermem); /* Get driver memory block offset */
  if (driveroffset>16)                  /* Is the memory offset > 1 segment */
    {
    driversegment+=(driveroffset/16);   /* Yes, calculate new segment size */
    }
  driversegment++;                      /* Round up, account for remainder */
  pointertodriver=(char far *) MK_FP(driversegment,0);

  if (read(filehandle, pointertodriver, (unsigned int) driversize)==-1)
    {
    farfree(pointertodrivermem);        /* Free memory used by voice driver */
    pointertodriver=0;                  /* Clear the pointer to voice driver */
    pointertodrivermem=0;               /* Clear the pointer to voice memory */
    close(filehandle);                  /* Close the driver file */
    return(FALSE);                      /* Can't read driver file, exit */
    }

  close(filehandle);                    /* Close the driver file */

  if ((pointertodriver[3]!='C')||(pointertodriver[4]!='T'))
    {
    farfree(pointertodrivermem);        /* Free memory used by voice driver */
    pointertodriver   =0;               /* Clear the pointer to voice driver */
    pointertodrivermem=0;               /* Clear the pointer to voice memory */
    return(FALSE);                      /* Not a SB voice driver file, exit */
    }

  soundcardstatus=SetupAndTestCard(cs->SndPort,cs->SndInt);

  if (soundcardstatus!=0)               /* SB Board in system and working? */
    {
    farfree(pointertodrivermem);        /* Free memory used by voice driver */
    pointertodriver   =0;               /* Clear the pointer to voice driver */
    pointertodrivermem=0;               /* Clear the pointer to voice memory */
    return(-1);                         /* No SB board found for this machine*/
    }

  asm {
        mov         bx,5                /* Set load status word address func */
        mov         es,statussegment
        mov         di,statusoffset
        call        pointertodriver     /* Call driver set status word address*/
      }

  driverloaded=TRUE;                    /* Set SB driver is loaded flag */
  return(TRUE);                         /* Return with driver loaded flag */
  }

static char SetupAndTestCard(unsigned int portaddress, unsigned int intaddress)
  {
  int temp;
  asm {
        mov         bx,1                /* Set SB load port address function */
        mov         ax,portaddress      /* Set the SB port address in AX */
        call        pointertodriver     /* Call driver to set SB port address */
        mov         bx,2                /* Set SB load int address function */
        mov         ax,intaddress       /* Set the SB int address in AX */
        call        pointertodriver     /* Call driver to set SB int address */
        mov         bx,3                /* Set the SB initialize function */
        call        pointertodriver     /* Call driver to initialize itself */
        mov         temp,ax  /* Get initialize driver return code */
      }
  return(temp);
  }



char PlaySbVocFile(char far * filename)
  {
  int   filehandle         = 0;       /* Declare a file handle             */
  uint  vocfileheadersize  = 0;       /* Declare variable for header size  */
  uint  vocfilesegment     = 0;       /* Declare variable for voice segment*/
  uint  vocfileoffset      = 0;       /* Declare variable for voice offset */
  char  fullfilename[MAXFILENAMELEN]; /* Declare sound path/driver path    */

  if (driverloaded==FALSE) return(FALSE); /* Have we already loaded driver */
                                          /* If No, exit with error        */

  if (soundcardstatus!=0) StopSound();    /* Do a new sound. */

  MakeFileName(fullfilename,WorkDir,filename,".voc");

  filehandle=open(fullfilename,O_RDONLY|O_BINARY); // Open the voice file.
  if (filehandle==-1) return(FALSE);   // On voice file open error then abort.

/*
**
**      If memory has already been allocated for a previous voice
**      file and is large enough for the new voice file then use the
**      existing buffer pointed to by the variable "pointertovoice".
**
**      If the existing voice buffer is not large enough then de-allocate
**      the existing buffer and allocate a larger one if possible.
**
*/

  // Get the required memory.

  if (pointertovoice!=NULL)           // memory already used by voice file?
    {
    if (filelength(filehandle) > vocfilesize)
      {
      delete pointertovoice;          // Free memory used by last voice file
      pointertovoice=NULL;            // Clear the pointer to the voice file
      }
    }
  if (pointertovoice==NULL)
    {
    vocfilesize    = filelength(filehandle); // Get size of the file in bytes
    pointertovoice = new char [vocfilesize];
    if (pointertovoice==NULL)
      {
      close(filehandle);              // Close the voice file
      return(FALSE);                  // Can't allocate voice memory, exit.
      }
    }

  // Read the .voc info.
  if (read(filehandle, pointertovoice, (uint) vocfilesize)==-1)
    {
    delete pointertovoice;              /* Free memory used by voice file */
    pointertovoice=NULL;                /* Clear the pointer to voice file */
    close(filehandle);                  /* Close the voice file */
    return(FALSE);                      /* Can't read voice file, exit */
    }

  close(filehandle);                    /* Close the voice file */

  if ((pointertovoice[0]!='C')||(pointertovoice[1]!='r'))
    {
    delete pointertovoice;              /* Free memory used by voice file */
    pointertovoice=NULL;                /* Clear the pointer to voice file */
    return(FALSE);                      /* Not a SB voice file, exit */
    }

  uchar temp        = pointertovoice[20];
  vocfileheadersize = (uint) temp;
  vocfilesegment    = FP_SEG(pointertovoice);
  vocfileoffset     = (FP_OFF(pointertovoice) + vocfileheadersize);

  asm {
        mov         bx,6
        mov         es,vocfilesegment
        mov         di,vocfileoffset
        call        pointertodriver
      }     

  return(TRUE);                         // Return with driver loaded flag.
  }

void StopSound(void)
  {
  if (driverloaded==FALSE) return;
  asm {
    mov         bx,8
    call        pointertodriver
    }
  }

void FreeSoundSample(void)
  {
  if (driverloaded==FALSE) return;
  StopSound();
  if (pointertovoice!=0)
    {
    delete  pointertovoice;  // Free memory used by voice file
    pointertovoice = 0;      // Clear the pointer to voice file
    vocfilesize    = 0;      // No sound so size is 0
    }
  }


void ShutSbVocDriver(void)
  {
  if (driverloaded==FALSE) return;     // Have we already loaded the driver
                                       // Driver not loaded then exit
  WaitSbVocComplete();                 // Wait for last sound to complete

  if (pointertovoice!=0)
    {
    delete pointertovoice;   // Free memory used by voice file
    pointertovoice = 0;      // Clear the pointer to voice file
    vocfilesize    = 0;      // No sound so size is 0
    }

  asm {
    mov         bx,9
    call        pointertodriver
    }

  farfree(pointertodrivermem);          /* Free memory used by voice driver */
  pointertodriver    = 0;               /* Clear the pointer to driver file */
  pointertodrivermem = 0;               /* Clear the pointer to driver file */
  driverloaded=FALSE;                   /* Set SB driver is loaded flag */
  return;                               /* Return from subroutine */
  }

static char ParseBlasterEnv(unsigned int *portaddress, unsigned int *intaddress)
  {
  char * envpointer=0;                  /* Declare pointer for env string */
  char * intpointer=0;                  /* Declare pointer interrupt string */
  char * portpointer=0;                 /* Declare pointer for port string */

// Already init in InitConfigData
//  *portaddress=0x220;                   /* Clear address variable */
//  *intaddress=7;                        /* Clear interrupt variable */

  envpointer=getenv("BLASTER");         /* Fetch port and interrupt address */
  if (envpointer==0)                    /* Port and interrupt address found? */
    {
    return(FALSE);                      /* Must know about port and interrupt */
    }

  portpointer=strchr(envpointer, 'a');  /* Look for port address specifier */
  if (portpointer!=0)
    {
    portpointer++;
    *portaddress=strtoul(portpointer ,0, 16);
    }
  portpointer=strchr(envpointer, 'A');  /* Look for port address specifier */
  if (portpointer!=0)
    {
    portpointer++;
    *portaddress=strtoul(portpointer ,0, 16);
    }

  intpointer=strchr(envpointer, 'i');    /* Look for int address specifier */
  if (intpointer!=0)
    {
    intpointer++;
    *intaddress=strtoul(intpointer ,0, 16);
    }
  intpointer=strchr(envpointer, 'I');   /* Look for int address specifier */
  if (intpointer!=0)
    {
    intpointer++;
    *intaddress=strtoul(intpointer ,0, 16);
    }
  if ((*intaddress==0)||(*portaddress==0))
    {
    return(FALSE);                      /* Must know about port and interrupt */
    }
  return(TRUE);                         /* Return with good port/int address */
  }



static void WaitSbVocComplete(void)
  {

  if (driverloaded==FALSE)              /* Have we already loaded the driver */
    return;                             /* Driver not loaded then exit */

  while (soundcardstatus!=0)            /* When SB soundcard is busy */
    {
    delay(50);                          /* Wait for 50 milliseconds */
    }

  return;                               /* Return from subroutine */
  }

int LoadSnd(char *prompt,char *ext,char *path, unsigned char *addr,char *fbuffer, int remember)
  {
  FILE *fp;
  char fname[MAXFILENAMELEN];
  char result;
  char srchfor[6];
  register int loop;

  strcpy(fname,path);
  if (!getfname(5,7,prompt,ext,fname)) return(FALSE);
  if ( (fp=fopen(fname,"rb")) ==NULL)  // must exist because getfname searches for it
    {
    errorbox("Can't Find file!","(C)ontinue");
    return(FALSE);
    }
  fread(addr,LASTSND,sizeof(sndstruct),fp);
  for (loop=0;loop<9*LASTSND;loop++) *(fbuffer+loop)=0;
  fread(fbuffer,9*LASTSND,1,fp);
  fclose(fp);
  if (remember) strcpy( (char *) curfile,(char *) fname);
  return(TRUE);
  }

int SaveSnd(char *prompt,char *ext,char *path, char *buffer,char *fbuffer)
  {
  FILE *fp=NULL;
#ifdef CRIPPLEWARE
  StdCrippleMsg();
#else
  if ( (fp=GetSaveFile(prompt,ext,path,curfile))==NULL) return(False);
  fwrite(buffer,LASTSND,sizeof(sndstruct),fp);
  fwrite(fbuffer,9*LASTSND,1,fp);
  fclose(fp);
#endif
  return(TRUE);
  }



