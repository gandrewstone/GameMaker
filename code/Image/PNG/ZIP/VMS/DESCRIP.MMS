# VMS Makefile for Zip, ZipNote, ZipCloak and ZipSplit

#
#  Modified to support both AXP and VAX by Hunter Goatley, 10-SEP-1993 06:43
#
#  To build under OpenVMS AXP, you must define the macro __ALPHA__:
#
#	$ MMS/MACRO=(__ALPHA__=1)
#
.IFDEF EXE			!If EXE and OBJ aren't defined, define them
.ELSE
EXE = .EXE
OBJ = .OBJ
CFLAGS = /NOLIST/OBJECT=$(MMS$TARGET)
.ENDIF

.IFDEF __ALPHA__		!Under OpenVMS AXP, we must use /STAN=VAXC
CC = CC/STANDARD=VAXC
OPTFILE =
OPTIONS =
.ELSE
OPTFILE = ,VAXCRTL.OPT
OPTIONS = $(OPTFILE)/OPT
.ENDIF

LINK = $(LINK)/NOTRACE		!Link /NOTRACEBACK

OBJZ =	zip$(obj),zipfile$(obj),zipup$(obj),fileio$(obj),util$(obj),-
	globals$(obj),crypt$(obj),vms$(obj),VMSmunch$(obj)
OBJI =	deflate$(obj),trees$(obj),bits$(obj)
OBJU =	zipfile$(obj)_,zipup$(obj)_,fileio$(obj)_,globals$(obj),-
	util$(obj)_,vmsmunch$(obj),vms$(obj)
OBJN =	zipnote$(obj),$(OBJU)
OBJS =	zipsplit$(obj),$(OBJU)
OBJC =	zipcloak$(obj),$(OBJU),crypt$(obj)_

!
!  Define our new suffixes list
!
.SUFFIXES :
.SUFFIXES :	$(EXE) $(OBJ)_ $(OBJ) .C

.c$(obj) :
	$(CC)$(CFLAGS) $(MMS$SOURCE)

!
!  .OBJ_ files are used by the Zip utilities (ZipNote, ZipCloak, ZipSplit)
!
.c$(obj)_ :
	$(CC)$(CFLAGS)/DEFINE=("UTIL") $(MMS$SOURCE)

# rules for zip, zipnote, zipsplit, and zip.doc.

default :	zip$(exe),zipnote$(exe),zipsplit$(exe),zipcloak$(exe)
 @ !

zipfile$(obj)_ :	zipfile.c
zipup$(obj)_   :	zipup.c,revision.h
fileio$(obj)_  :	fileio.c
util$(obj)_    :	util.c
crypt$(obj)_   :	crypt.c

$(OBJZ) : zip.h,ziperr.h,tailor.h
$(OBJI) : zip.h,ziperr.h,tailor.h
$(OBJN) : zip.h,ziperr.h,tailor.h
$(OBJS) : zip.h,ziperr.h,tailor.h

zip$(obj),zipcloak$(obj),zipup$(obj),zipnote$(obj),zipsplit$(obj) : revision.h

zipfile$(obj), fileio$(obj), VMSmunch$(obj) : VMSmunch.h

zip$(exe) : $(OBJZ),$(OBJI)$(OPTFILE)
	$(LINK)/exe=zip$(exe) $(OBJZ),$(OBJI)$(OPTIONS)

zipnote$(exe) : $(OBJN)$(OPTFILE)
	$(LINK)/exe=zipnote$(exe) $(OBJN)$(OPTIONS)

zipsplit$(exe) : $(OBJS)$(OPTFILE)
	$(LINK)/exe=zipsplit$(exe) $(OBJS)$(OPTIONS)

zipcloak$(exe) : $(OBJC)$(OPTFILE)
	$(LINK)/exe=zipcloak$(exe) $(OBJC)$(OPTIONS)

VAXCRTL.OPT :
	@ open/write tmp vaxcrtl.opt
	@ write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
	@ close tmp
