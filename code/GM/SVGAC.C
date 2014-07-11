/*-------------------------------------------------------------------------*/
/* sVGArout.C    super VGA routines                                        */
/* By Andy Stone                     Created: Aug. 3, 1991                 */
/*-------------------------------------------------------------------------*/
#pragma inline

#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "svga.h"
#include "gmgen.h"

#define sign(x) ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))

extern uint zeroaddon;
uchar       curpage=0;
uchar       zeropage=0;

static void SetupCard(VideoCards Vcard);
static int  SetParadise(void);
static int  SetNotSupported(void);
static int  idParadise(void);
static int  idTrident(void);
static int  idTseng4000(void);
static int  TextID_VGA(void);
static void SetOldATI(void);

static int  WhichATI  (void);
static int  WhichTseng(void);

VideoCards extern far Vcard;  // Defined in svgaa.asm so it can be in the code segment
extern unsigned int far ATIExtReg;


int Force_VGA(VideoCards v)
  {
  char vgas[][41] = {"Standard","ATI","ChipTech","Genoa","Paradise","Trident8800","Trident8900","Tseng3000","Tseng4000","Video7","VESA","ATIWonder","be simulated in Software"};

  Vcard=v;

  if ((Vcard<0)||(Vcard>NoType)) Vcard=NoType;
  if (Vcard < NoType) printf("Forcing Super VGA to %s.\n",vgas[Vcard]);

  SetupCard(Vcard);
  return(Vcard);
  }

/*
int Force_VGA(char *vgaid)
  {
  char vgas[][21] = {"Standard","ATI","ChipTech","Genoa","Paradise","Trident8800","Trident8900","Tseng3000","Tseng4000","Video7","VESA","ATIWonder"};

  for (Vcard=Unknown; Vcard<=NoType; Vcard=(VideoCards)(Vcard+1))
    if (strcmpi(vgaid,vgas[Vcard])==0) break;

  if ((Vcard<0)||(Vcard>NoType)) Vcard=NoType;
  if (Vcard < NoType) printf("Forcing Super VGA to %s.\n",&vgas[Vcard][0]);

  SetupCard(Vcard);

  return(Vcard);
  }
*/

int Identify_VGA(void)
  {
  // No?  check for Paradise (Western Digital)
  if (idParadise())
    {
    SetupCard(Vcard);
    printf ("Paradise Super VGA detected!\n");
    return(TRUE);
    }

  // No?, check for Trident
  if (idTrident())
    {
    if (Vcard==Trident8800) printf ("Trident 8800 VGA chip detected!\n");
    if (Vcard==Trident8900) printf ("Trident 8900 VGA chip detected!\n");
    SetupCard(Vcard);
    return(TRUE);
    }
  // No? Check for known text.
  if (TextID_VGA()) return(TRUE);

  if (idTseng4000())
    {
    Vcard=Tseng4000;
    if (Vcard==Tseng4000) printf("Tseng ET4000 Super VGA chip detected!\n");
    SetupCard(Vcard);
    return(TRUE);
    }

  printf("No super Vga card detected! Assuming standard vga.\n");
  Vcard=Unknown;
  return (TRUE);
  }

static int TextID_VGA(void)
  {
  char far *BIOSptr = (char far *) 0xC0000000;  /* Pointer which may have the VGA name near */
  int memctr=0;
  int vga=0,temp=0;
  char vgas[][21] = {"761295520","ChipTech","Genoa","Paradise","Trident","Tseng","Video7","Digital","Pixel Engineering"};
  
  for (memctr=0; memctr<256;memctr++)
    {
    for (vga=0;vga<8; vga++)
      if (strncmpi( (BIOSptr+memctr),vgas[vga],strlen(vgas[vga]))==0) // Make sure far ver of strcmpi
        { temp=vga; memctr=256; break; }
    }

  Vcard=Unknown;
  switch(temp)
    {
    case 0:
      WhichATI();
      if (Vcard==ATI)       printf("ATI Super VGA chip Detected!\n");
      if (Vcard==ATIWonder) printf("ATI VGA Wonder Detected!\n");
      break;
    case 7:
    case 3:
      Vcard=Paradise;
      printf ("Paradise Super VGA detected!\n");
      break;  
    case 4:
      if (!idTrident()) 
        {
        Vcard=Trident8900;
        printf("Trident ID - Unknown Hardware chip version - assuming 8900!\n");
        }
      else if (Vcard==Trident8800) printf ("Trident 8800 Super VGA chip detected!\n");
      else if (Vcard==Trident8900) printf ("Trident 8900 Super VGA chip detected!\n");
      break;
    case 8:
    case 5:
      WhichTseng();                // Figure out which Tseng chip
      if (Vcard==Tseng4000) printf("Tseng ET4000 Super VGA chip detected!\n");
      if (Vcard==Tseng3000) printf("Tseng ET3000 Super VGA chip detected!\n");
      break;
    }
  SetupCard(Vcard);
  return( (Vcard>0));
  }

static void SetupCard(VideoCards V)
  {
  switch(V)
    {
    case Unknown:
      break;
    case ATI:
      SetOldATI();
      break;
    case ChipTech:
      SetNotSupported();
      break;
    case Genoa:
      SetNotSupported();
      break;
    case Paradise:
      SetParadise();
      break;
    case Trident8800:
      SetTrident();                // Set up the Trident VGA device
      break;
    case Trident8900:
      SetTrident();
      break;
    case Tseng3000:
      break;
    case Tseng4000:
      break;
    case Video7:
      SetNotSupported();
      break;
    case ATIWonder:
      break;
    case NoType:
      Vcard=Unknown;
      break;
    }
  }

static int idTseng4000(void)
  {
  unsigned char New=0,Old=0;

  Screen(0);
  outportb(0x3D4,0x33);
  Old=inportb(0x3D5);
  outportb(0x3D5,Old+1);
  New=inportb(0x3D5);
  outportb(0x3D5,Old);
  Screen(TRUE);
  if (New==Old+1) return(TRUE);
  return(FALSE);
  }

static int WhichTseng(void)
  {
  unsigned char curval,newval;

  /* Check to see whether it is a Et3000, or ET4000 chip */

  curval=inportb(0x3CD); // ET3000- Bit #6 (undoced enable page change?) 
   // is always set in text mode - Et4000 would mean a different read page 
  
  if ((curval>>6)==0) Vcard=Tseng4000;
  else Vcard=Tseng3000;
  return(Vcard);
  }

static int idParadise(void)
  {
  char far *signature= (char far *) 0xC000007D;
  int loop;
  char sign[5]="VGA=";
  int quit=FALSE;

  for (loop=0;loop<4;loop++) 
    if (sign[loop]!=signature[loop]) quit=TRUE;
  if (quit) return(0);   

  Vcard=Paradise;
  return(1);
  }


static int idTrident(void)       /* Identify a Trident chip set function */
  {                              /* Start of function */
  unsigned char reg;             /* Define storage for device peak result */

  outportb(0x03C4,0x0B);         /* Set up to ask for version information */
  reg=(0x0F&(inportb(0x03C5)));  /* Fetch identification/version info */
  if ((reg==1)||(reg==2))        /* Was this a Trident chip set VGA */
    {                            /* Start of "if" logic */
    Vcard=Trident8800;           // Set the Global variable to correct chip
    return(TRUE);                /* Return truth in life */
    }                            /* End of "if" logic */
  if (reg==3)                    /* Was this a Trident chip set VGA */
    {                            /* Start of "if" logic */
    Vcard=Trident8900;           // Set global variable to correct chip
    return(TRUE);                /* Return truth in life */
    }                            /* End of "if" logic */
  return(FALSE);                 /* Signal this system is not a Trident VGA */
  }                              /* End of Trident chip set function */

static int SetParadise(void)
  {
  unsigned char reg;
  Vcard=Paradise; 
 
  /* Make Paradise act exactly as a normal VGA */
  outportb(0x03CE,0x0F);   /* access unlock registers register */
  outportb(0x03CF,0x05);   /* Unlock Enhanced Feature registers */
  outportb(0x03CE,0x0B);   /* access Memory size register */
  reg=inportb(0x03CF);     /* change Memory size register */
  reg&=0x0F;              /*   to standard VGA memory and mapping */
  outportb(0x03CF,reg);
  outportb(0x03CE,0x0F);   /* access unlock registers register */
  outportb(0x03CF,0x00);   /* Lock Enhanced Feature registers */
  return(1);
  }

static void SetOldATI(void)
  {
  int  far *Reg_ptr   = (int far *) 0xC0000010;  /* Pointer which contains ATI extended register address*/

  ATIExtReg= *Reg_ptr;   /* Get register address into ExtReg */
  Vcard=ATI;
  /* Setup display for single bank mode */
  asm mov       al,0BEh
  asm mov       ah,00000000h
  asm mov       dx,[cs:ATIExtReg]
  asm out       dx,ax
  } 

static int WhichATI(void)
  {
  char far *BIOSptr = (char far *) 0xC0000000;  /* Pointer which may have the VGA name near */
  int memctr=0;
  int dctr=0,temp=0;
  char data[][21] = {"ATI ULTRA","ATI VGAWONDER"};
  
  switch(*((char far *)0x0C0000043))
    {
    case '1':
    case '2':
    case '3':
    case 'a':
      Vcard=ATI;
      break;
    case '5':
    case '6':
      Vcard=ATIWonder;
      break;
    default:
      for (memctr=0; memctr<256;memctr++)
        {
        for (dctr=0;dctr<2; dctr++)
          if (strncmpi( (BIOSptr+memctr),data[dctr],strlen(data[dctr]))==0) // Make sure far ver of strcmpi
            {
            if (dctr==0) { Vcard=ATI;       return(1); }
            if (dctr==1) { Vcard=ATIWonder; return(1); }
            }
        }
      break;
    }
  return(0);
  } 

static int SetNotSupported(void)
  {
  Vcard=Unknown;
  return(1);
  }


int MoveWindow(unsigned char offset64k)
  {
  unsigned char reg;

  if ((Vcard==Paradise)|(Vcard==Unknown)) return(1);
  offset64k&=0xF;
  if (curpage!=offset64k)
    {
    curpage=offset64k;
    SetPage(curpage);
    }
  return(1);
  }


//static unsigned long int cwind;

/*
int ViewWindow(unsigned long int window)
  {
  int r=0x03D4,r1=0x03D5;
  
 union 
    {
    unsigned char c[4];
    unsigned long int li;
    unsigned int i[2];
    } wb;  

 //  cwind=window;
  wb.li=window;

  outportb(r,0x11);           // unlock registers 0-7
  outportb(r1,(inportb(r1)&127));

//      Set VGA Display Start address
  outportb(r,0x0C);
  outportb(r1,wb.c[1]);
  outportb(r,0x0D);
  outportb(r1,wb.c[0]);

  outportb(r,0x11);           // lock registers 0-7
  outportb(r1,(inportb(r1)|128));

  wb.li<<=2;
  zeroaddon=wb.i[0];
  zeropage= (wb.c[2]&0x0F);
  wb.li>>=2;

  // Above is standard VGA changing of Start Address high and low
  // Below is the > 256k Super VGA specific modifications

  if (Vcard==Tseng4000)
    {
    outportb(0x3D4,0x33);
    outportb(0x3D5,wb.c[2]&3);
    return(1);
    }
  if (Vcard==ATI) return(1);    
  if (Vcard==Paradise) return(1);
  if ((Vcard==Trident8800)||(Vcard==Trident8900)) SetTridentStart(wb.c[2]&1);
  if (Vcard==Unknown) return(1);
  return(1);
  }
*/

/*
int WaitVertRetrace(void)           // Wait for vertical retrace function
  {                                 // Start of function
  unsigned char reg=0;              // Define storage for device peak result

  while (reg==0)                    // Wait for vertical retrace to start
    {
    reg=inportb(0x03DA);            // Fetch status of vertical retrace pulse
    reg&=0x08;                      // Isolate the vertical retrace synch bit
    }
  return(TRUE);                     // Return truth in life
  }                                 // End of wait verticle retrace function
*/

