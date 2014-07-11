#include  <stdio.h>
#include  <bios.h>
#include  <alloc.h>

#include  "sbcmusic.h"
#include  "sbc.h"
#include  "windio.h"
#include  "gen.h"
#include  "sound.h"
#include  "jstick.h"

#define APOS (sbmusic+offset)

static void InitCard(void);                        //Initialize FM chip
static void SetInstTable(char far *inst);          //put defined insts. in chip
static int  load_file(int *Instru,char *filename); //read CMF file
static long int getpaws(void);                     //Interpret CMF lengths

int  InitMusic(int *Instru,char *cmffile);         //Load song to memory, etc.
int  SoundCard(void);                              //Find Sound Card present
void sbfreemem(void);                              //free song from memory
void StopIt(void);                                 //quit playing song
void PlayIt(void);                                 //play song
void ResetFM(void);                                //Reset the FM chip

static long int offset = 0;        // Variables for use in PlayIt()
static unsigned long int wait = 0;
static unsigned char far *sbmusic=NULL;
static char far *inst=NULL;

int SoundCard(ConfigStruct *cs)
  {
  unsigned int status=FALSE;                    //check to see is SoundBlaster
                                                //fm voice is working 

  ct_io_addx=cs->SndPort;
  if ( (sbc_check_card() & 2) )
    {
    status=CARDEXIST;
    InitCard();
    printf("Sound Blaster Found!\n");
    }
  return(status);
  }

int InitMusic(int *Instru,char *cmffile)
  {
  int error;
  
  if ((error = load_file(Instru,cmffile)) == 0)
    {
    SetInstTable(inst);
    return(0);
    }
  return(error);
  }

static int load_file(int *Instru,char *cmffile)
  {
  FILE *fp;
  CMFHDR hdr;
  char extradat[32];
  long MusicSize;
  register int j;
    
  if ((fp=fopen(cmffile,"rb")) == NULL)
    {
    //               errorbox("Could not open file","(D)on't Use Music");
    return(1);
    }
  fread((char *)&hdr,1,sizeof(CMFHDR),fp);
  if ( (hdr.id[0]!='C') && (hdr.id[1]!='T') &&
       (hdr.id[2]!='M') && (hdr.id[3]!='F') )
    {
    fclose(fp);
    //              errorbox("This is not a CMF file!","(D)on't Use Music");
    return(2);
    }
  if (hdr.clock_ticks != HDRCLK)
    {
    fclose(fp);
    //              errorbox("This file has not been converted for use",
    //                       "with Game-Maker!    (D)on't use music");
    return(3);
    }
  *Instru=hdr.inst_num;
  fseek(fp,0L,SEEK_END);
  MusicSize = ftell(fp)-(long)hdr.music_blk;
  if ( ((inst = (char *)farmalloc((long)(hdr.inst_num*16))) == NULL) ||
       ((sbmusic = (unsigned char *)farmalloc(MusicSize)) == NULL)  )
    {
    fclose(fp);
    sbfreemem();
    //             errorbox("Not enough memory!","(D)on't Use Music");
    return(4);
    }
  fseek(fp,(long)hdr.inst_blk,SEEK_SET);
  fread(inst,1,(hdr.inst_num*16),fp);
  fseek(fp,(long)hdr.music_blk,SEEK_SET);
  fread((unsigned char *)sbmusic,1,MusicSize,fp);
  fclose(fp);
  return(0);
  }
  
void sbfreemem(void)
  {
  farfree(inst);
  farfree(sbmusic);
  }

void StopIt(void)                  // This function should:
  {                                //  Stop all music.
  sbfd_music_off();
  offset=0;
  }

void InitCard(void)                // This function should:
  {                                //  Initialize the FM chip
  sbfd_init();                     //  Set instrument table to internal default
  }

void ResetFM(void)
  {
  sbfd_reset();
  }
  
void SetInstTable(char far *inst)  // This function should:
  {                                //  Put the defined instruments into memory
  sbfd_instrument(inst);           //  Set the first 16 (11) instruments to the
  }                                //   first 16 (11) channels in melody
                                   //   (rhythm) mode.
                                   // Inst should contain the instrument table
                                   //   in SBI format.  At most 128 instruments.

void PlayIt(void)                  // This function should:
  {                                //  Accept the data from a CMF file at the
  static char channel = 0;         //   location specified by *sbmusic and
  static char funct = 8;           //   all resulting commands.
                                   //  Be called on every clock tick!
  if (offset==0) {wait =1; getpaws();} // Skip beginning silence.
  wait--;
  while (wait==0)
    {
    switch ((*(APOS))&0xF0)
      {
      case 0x90:                          // Note on
        channel = (*(APOS)) & 0x0F;
        offset++;
        funct = 9;
        break;
      case 0x80:                         // Note off
        channel = (*(APOS)) & 0x0F;
        offset++;
        funct = 8;
        break;
      }
    if ((*(APOS)) < 0x080)
      {
      if (funct == 8)
        sbfd_note_off(channel,*(APOS),*(APOS+1));
      else
        {
        if (*(APOS+1) == 0)
          sbfd_note_off(channel,*(APOS),0);
        else
          sbfd_note_on(channel,*(APOS),*(APOS+1));
        }
      offset+=2;
      }
    else switch((*(APOS))&0x0F0)
      {
      case 0xA0:  offset+=3;  break;        //Polyphonic Aftertouch--Ignore
      case 0xB0:
        switch (*(APOS+1))
          {
          case 0x67:                        //Change melody/rhythm mode
            sbfd_setmode(*(APOS+2));
            offset+=3;
            break;
          case 0x68: offset+=3; break;     //Increase pitch by a little
          case 0x69: offset+=3; break;     //Decrease pitch a little
          default: offset+=3;              //Unrcognized
          }
        break;
      case 0xC0:                            //Change instrument of channel
        sbfd_program_change((*(APOS))&0x0F,*(APOS+1));
        offset+=2;
        break;
      case 0xD0: offset += 2; break;       //Aftertouch -- ignore
      case 0xE0: offset += 3; break;       //Pitchbend -- ignore
      case 0xF0:
        if ( ((*(APOS)) == 0x0F7) || ((*(APOS)) == 0x0F0) )
          {
          offset++;                         //System Exclusive message--ignore
          offset+=getpaws();
          offset++;
          }
        else {sbfd_music_off(); offset=0;} //if (FF 2F), end of song so repeat.
        break;                             //if not, I'm lazy so repeat anyway.
                                           // maybe I should reset and reload.
      default: sbfd_reset(); offset=0;     //This should never occur!
      }
    wait = getpaws();
    }
  }

static long int getpaws(void)
  {
  unsigned long int value=0;
  char c;
  
  if ((value = (long int) *(APOS)) & 0x80)
    {
    value &= 0x7F;
    do
      {
      offset++;
      value = (value << 7) | ((c=*(APOS)) & 0x07F);
      } while (c & 0x80);
    }
  offset++;
  return(value);
  }
