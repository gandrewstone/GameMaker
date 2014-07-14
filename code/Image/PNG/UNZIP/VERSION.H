/*
   version.h (for UnZip) by Info-ZIP.

   This header file is not copyrighted and may be distributed without
   restriction.  (That's a little geek humor, heh heh.)
 */

#ifndef __version_h   /* don't include more than once */
#define __version_h

#ifdef BETA
#  define BETALEVEL   "e"
#  define UZ_VERSION  "5.12e BETA of 25 Aug 94"   /* internal beta level */
#  define ZI_VERSION  "2.02e BETA of 25 Aug 94"
#  define D2_VERSION  "0.0 BETA of xx Xxx 94"
#else
#  define BETALEVEL   ""
#  define UZ_VERSION  "5.12 of 28 August 1994"   /* official release version */
#  define ZI_VERSION  "2.02 of 28 August 1994"
#  define D2_VERSION  "0.0 of x Xxxxxx 1994"   /* (DLL for OS/2) */
#  define RELEASE
#endif

#define UZ_MAJORVER  5
#define UZ_MINORVER  1

#define ZI_MAJORVER  2
#define ZI_MINORVER  0

#define PATCHLEVEL   2

#endif /* !__version_h */
