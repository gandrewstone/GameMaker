/*---------------------------------------------------------------------------*/
/* Joy Stick Routine header file  ( for jstickc.c )                          */

#ifndef JOYSTICK
#define JOYSTICK
#ifndef GM_ENUM16
#define GM_ENUM16          // Borland enums are 16 bits; the port shim defines this as ": short"
#endif

#define JOYCLKINT 10   /* Clock timer ticks required prior to joy stick read */

#define CONFIGFILE "config.dat"

typedef enum GM_ENUM16
  { None, SndBlaster, End } SoundCards;

#ifndef SVGA
typedef enum GM_ENUM16
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