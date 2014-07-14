$ !
$ !     "Makefile" for VMS versions of Zip, ZipCloak, ZipNote,
$ !      and ZipSplit (stolen from Unzip)
$ !
$ !      IMPORTANT NOTE: do not forget to set the symbols as said below
$ !                      in the Symbols section.
$ !
$ if f$getsyi("HW_MODEL").ge.1024
$ then	write sys$output "GNU C has not yet been ported to OpenVMS AXP."
$	write sys$output "You must use MAKE.COM and DEC C to build Zip."
$	exit
$ endif
$ set verify    ! like "echo on", eh?
$ !
$ !------------------------------- Zip section --------------------------------
$ !
$ gcc /def=CRYPT zip
$ gcc /def=CRYPT crypt
$ gcc /def=CRYPT zipfile
$ gcc /def=CRYPT zipup
$ gcc /def=CRYPT fileio
$ gcc /def=CRYPT util
$ gcc /def=CRYPT deflate
$ gcc /def=CRYPT globals
$ gcc /def=CRYPT trees
$ gcc /def=CRYPT bits
$ gcc /def=CRYPT vms
$ gcc /def=CRYPT VMSmunch
$ ! add /def=CRYPT and crypt for crypt version
$ link zip,crypt,zipfile,zipup,fileio,util,deflate,globals,trees,bits, -
   vms,VMSmunch, gnu_cc:[000000]gcclib.olb/lib, sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ !
$ !-------------------------- Zip utilities section ---------------------------
$ !
$ gcc /def=(CRYPT,UTIL) zipnote
$ gcc /def=(CRYPT,UTIL) zipsplit
$ gcc /def=(CRYPT,UTIL) /obj=zipfile_.obj zipfile
$ gcc /def=(CRYPT,UTIL) /obj=zipup_.obj zipup
$ gcc /def=(CRYPT,UTIL) /obj=fileio_.obj fileio
$ gcc /def=(CRYPT,UTIL) /obj=util_.obj util
$ ! uncomment crypt and zipcloak lines for crypt version
$ gcc /def=(CRYPT,UTIL) zipcloak
$ gcc /def=(CRYPT,UTIL) /obj=crypt_.obj crypt
$ link zipcloak, zipfile_, zipup_, fileio_, util, crypt_, globals, -
   VMSmunch, gnu_cc:[000000]gcclib.olb/lib, sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ link zipnote, zipfile_, zipup_, fileio_, util_, globals, VMSmunch, -
   sys$input:/opt
sys$share:vaxcrtl.exe/shareable
$ link zipsplit, zipfile_, zipup_, fileio_, util_, globals, VMSmunch, -
   gnu_cc:[000000]gcclib.olb/lib, sys$input:/opt
sys$share:vaxcrtl.exe/shareable
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
