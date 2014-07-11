/*  sound effect header file                  */
/*  created by Oliver Stone     7-19-91       */
/*  last edited   7-19-91                     */

#ifndef SOUND
#define SOUND

#include "jstick.h"

#define NUMFREQ  36
#define FREQL    6              //  Low frequency is 2 to the 6th, 64
#define FREQH    13             //  2 to the 13th, or 8192
#define LASTSND  30
#define DELAY    7

#define HELPSOUND  0
#define KILLSOUND 1
#define ADDMONEYSOUND 2
#define SUBMONEYSOUND 3
#define ADDLIFESOUND  4
#define ADDSCORESOUND 5
#define SUBSCORESOUND 6
#define ADDOTHERSOUND 7
#define SUBOTHERSOUND 8

typedef struct 
  {
  unsigned int freq[NUMFREQ];
  } sndstruct;

typedef struct 
  {
  char fname[LASTSND][13];
  char far * pointertodriver[LASTSND];
  unsigned int memoryblocks[LASTSND];
  unsigned int accesscount[LASTSND];
  } cachestruct;
 
// Sound Blaster CMF Information Follows 

#define TIMER0 0xFFFF  // This is the default clock speed
#define TIMER1 0x1FFF  // This is eight times above for use w/o FastTimer()
#define HDRCLK 146     // This should be the value in the cmf file's header
                       // 146 is approx. (18.2 * 8) or 1193180/0x1FFF
#define MAXINST     16
#define CARDEXIST    1
#define SONGLOADED   2
#define MUSICOFF     4

// Generic Routines.
int SaveSnd(char *prompt,char *ext,char *path, char *buffer,char *fbuffer);
int LoadSnd(char *prompt,char *ext,char *path, unsigned char *addr,char *fbuf,int remember);


// Song Interface Routines.
extern int  DetectCard(SoundCards *s,unsigned int *Port, unsigned int *Interrupt,char *drvrname);
extern int  InitMusic(int *Instru,char *cmffile);
extern int  SoundCard(ConfigStruct *cs);
extern void PlayIt(void);
extern void sbfreemem(void);
extern void ResetFM(void);
extern void StopIt(void);

// Voc routines.
char InitSbVocDriver (ConfigStruct *cs);
char PlaySbVocFile   (char far * filename);
void ShutSbVocDriver (void);
void StopSound       (void);
void FreeSoundSample (void);
#endif
