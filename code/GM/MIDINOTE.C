#include <stdio.h>

void main(void)
  {
  char midi[88][7]={"A1","A#/Bb1","B1","C1","C#/Db1","D1","D#/Eb1","E1","F1","F#/Gb1","G1","G#/Ab1", 
                    "A2","A#/Bb2","B2","C2","C#/Db2","D2","D#/Eb2","E2","F2","F#/Gb2","G2","G#/Ab2", 
                    "A3","A#/Bb3","B3","C3","C#/Db3","D3","D#/Eb3","E3","F3","F#/Gb3","G3","G#/Ab3", 
                    "A4","A#/Bb4","B4","C4","C#/Db4","D4","D#/Eb4","E4","F4","F#/Gb4","G4","G#/Ab4", 
                    "A5","A#/Bb5","B5","C5","C#/Db5","D5","D#/Eb5","E5","F5","F#/Gb5","G5","G#/Ab5", 
                    "A6","A#/Bb6","B6","C6","C#/Db6","D6","D#/Eb6","E6","F6","F#/Gb6","G6","G#/Ab6", 
                    "A7","A#/Bb7","B7","C7","C#/Db7","D7","D#/Eb7","E7","F7","F#/Gb7","G7","G#/Ab7", 
                    "A8","A#/Bb8","B8","C8"};
  int  notenum;
  char hexid=0;
                 
  do
    {
    while (!hexid)
      {
      printf("\n\nEnter MIDI note number (zero to quit, one for hex): ");
      scanf("%d",&notenum);
      if (notenum==1) hexid=1;
      else if (notenum==0) break;
      else if ( (notenum<21) || (notenum>108) )
        printf("\n\n        INVALID MIDI CODE");
      else printf("\nHex: %02X  Note: %s",notenum,midi[notenum-21]);
      }
    while (hexid)
      {
      printf("\n\nEnter MIDI note number (zero to quit, one for decimal): ");
      scanf("%x",&notenum);
      if (notenum==1) hexid=0;
      else if (notenum==0) break;
      else if ( (notenum<21) || (notenum>108) )
        printf("\n\n        INVALID MIDI CODE");
      else printf("\nDecimal: %3d  Note: %s",notenum,midi[notenum-21]);
      }
    } while (notenum != 0);
  }