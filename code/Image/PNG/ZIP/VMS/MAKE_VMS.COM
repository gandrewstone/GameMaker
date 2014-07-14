$ !
$ !     "Makefile" for VMS versions of Zip, ZipCloak, ZipNote,
$ !     and ZipSplit (stolen from Unzip)
$ !
$ !      IMPORTANT NOTE: do not forget to set the symbols as said below
$ !                      in the Symbols section.
$ !
$ axp = f$getsyi("HW_MODEL").ge.1024
$ if axp then cc = "cc/standard=vaxc"
$ link = "link/notrace"
$ if f$search("VAXCRTL.OPT").eqs.""
$ then	create vaxcrtl.opt
$	open/append tmp vaxcrtl.opt
$	if axp      then write tmp "!Dummy .OPT file for AXP"
$	if .not.axp then write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
$	close tmp
$ endif
$ set verify    ! like "echo on", eh?
$ !
$ !------------------------------- Zip section --------------------------------
$ !
$ cc/def=CRYPT zip, zipfile, zipup, crypt, fileio, util, deflate, globals,-
        trees, bits, vms, VMSmunch
$ link/notrace zip,zipfile,zipup,crypt,fileio,util,deflate,globals,-
        trees,bits,vms,VMSmunch,vaxcrtl.opt/options
$ !
$ !-------------------------- Zip utilities section ---------------------------
$ !
$ cc /def=(UTIL) /obj=zipfile_.obj zipfile.c
$ cc /def=(UTIL) /obj=zipup_.obj zipup.c
$ cc /def=(UTIL) /obj=fileio_.obj fileio.c
$ cc /def=(UTIL) /obj=util_.obj util.c
$ cc /def=(UTIL) /obj=crypt_.obj crypt.c
$ cc zipcloak
$ cc zipnote, zipsplit
$ link zipcloak, zipfile_, zipup_, fileio_, util_, crypt_, globals, -
        vms, VMSmunch, vaxcrtl.opt/options
$ link zipnote, zipfile_, zipup_, fileio_, util_, globals, vms, VMSmunch,-
        vaxcrtl.opt/options
$ link zipsplit, zipfile_, zipup_, fileio_, util_, globals, vms, VMSmunch,-
        vaxcrtl.opt/options
$ !
$ !----------------------------- Symbols section ------------------------------
$ !
$ ! Set up symbols for the various executables.  Edit the example below,
$ ! changing "disk:[directory]" as appropriate.
$ !
$ zip           == "$disk:[directory]zip.exe"
$ zipcloak      == "$disk:[directory]zipcloak.exe"
$ zipnote       == "$disk:[directory]zipnote.exe"
$ zipsplit      == "$disk:[directory]zipsplit.exe"
$ !
$ set noverify
