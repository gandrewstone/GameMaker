/*---------------------------------------------------------------------------*/
/* Joy Stick Routine header file  ( for jstickc.c )                          */

#ifndef JOYSTICK
#define JOYSTICK

#define JOYCLKINT 10   /* Clock timer ticks required prior to joy stick read */

#define CONFIGFILE "config.dat"

typedef enum
  { None, SndBlaster, End } SoundCards;

#ifndef SVGA
typedef enum 
   { Unknown, ATI, ChipTech, Genoa, Paradise, Trident8800,Trident8900, Tseng3000, Tseng4000, Video7, VESA, ATIWonder,SoftwareSim,NoType} VideoCards;
#endif

typedef struct
  {
  int        joyx[5];       // Saved joy stick X coordiantes
  int        joyy[5];       // Saved joy stick Y coordiantes
  VideoCards ForceVGA;    // NoType = detect, unknown = standard VGA
  SoundCards ForceSnd;
  unsigned int SndInt;
  unsigned int SndPort;
  char SndDrvr[41];
  char extra[431];          // Lots of extra space for more tailored options
  } ConfigStruct;

extern void GetJoyPos(char far *pos, int far *button, ConfigStruct far *js);
extern int  LoadConfigData(ConfigStruct far  *js);
extern int  SaveConfigData(ConfigStruct far *js);
extern char InitJStick(void);
extern void joyclearbuts(void);

extern "C" void ReadJoyStick(unsigned int far *x,unsigned int far *y,unsigned int far *butn);

#endif
/* ---------------------------------END--------------------------------------*/
