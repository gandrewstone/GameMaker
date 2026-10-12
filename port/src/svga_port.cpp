/* Replacement for SVGAC.C (Super-VGA card detection and bank switching).
 *
 * The original probes BIOS ROM at C000:0000 and programs vendor registers for ATI, Paradise, Tseng,
 * Trident, ... cards. In a window there is exactly one "card": a plain 320x200x256 VGA whose scrolling
 * the engine simulates in software (VideoCards::SoftwareSim), so detection reduces to picking that.
 */
#include "svga.h"
#include "gmgen.h"

uchar curpage = 0;
uchar zeropage = 0;

VideoCards Vcard = SoftwareSim;     // was defined in svgaa.asm
unsigned int ATIExtReg = 0;         // ditto; only used by the ATI-specific paths

int Force_VGA(VideoCards v)
  {
  Vcard = SoftwareSim;              // whatever was configured, there is only the simulated card
  return(Vcard);
  }

int Identify_VGA(void)
  {
  Vcard = SoftwareSim;
  return(TRUE);
  }

int MoveWindow(unsigned char offset64k)
  {
  return(1);                        // no banked video memory to move
  }
