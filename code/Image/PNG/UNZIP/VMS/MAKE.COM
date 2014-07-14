$ !
$ !	"Makefile" for VMS versions of UnZip/ZipInfo and UnZipSFX
$ !
$ !	To define additional options, define the global symbol
$ !	LOCAL_UNZIP prior to executing MAKE.COM:
$ !
$ !		$ LOCAL_UNZIP == "VMSCLI,RETURN_SEVERITY,"
$ !		$ @MAKE
$ !
$ !	The trailing "," may be omitted.  Valid VMS-specific options
$ !	include VMSCLI, VMSWILD, RETURN_SEVERITY and RETURN_CODES; see
$ !	the INSTALL file for other options (e.g., CHECK_EOF).
$ !
$ !
$ ! Find out current disk, directory, compiler and options
$ !
$ my_name = f$env("procedure")
$ here = f$parse(my_name,,,"device") + f$parse(my_name,,,"directory")
$ if f$type(LOCAL_UNZIP).eqs.""
$ then
$	local_unzip = ""
$ else	! Trim blanks and append comma if missing
$	local_unzip = f$edit(local_unzip, "TRIM")
$	if f$extract(f$length(local_unzip)-1, 1, local_unzip).nes."," then -
		local_unzip = local_unzip + ","
$ endif
$ axp = f$getsyi("HW_MODEL").ge.1024
$ if axp
$ then
$	cc = "cc/standard=vaxc/ansi/nowarnings/include=[]"
$	defs = "''local_unzip'MODERN"
$	opts = ""
$ else
$	defs = "''local_unzip'VMS"
$	opts = ",[.VMS]VMSSHARE.OPT/OPTIONS"
$	if (f$search("SYS$SYSTEM:VAXC.EXE").eqs."" .and. -
		f$trnlnm("GNU_CC").nes."") .or. (p1.eqs."GCC")
$	then
$		cc = "gcc"
$		opts = "''opts',GNU_CC:[000000]GCCLIB.OLB/LIB"
$	else
$		cc = "cc"
$	endif
$ endif
$ def = "/define=(''defs')"
$ old_ver = f$ver(1)	! Turn echo on to see what's happening
$ on error then goto error
$ on control_y then goto error
$ !
$ x = ""
$ if f$search("SYS$LIBRARY:SYS$LIB_C.TLB").nes."" then -
	x = "+SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY"
$ 'CC'/NOLIST'DEF' /OBJ=UNZIP.OBJ UNZIP.C
$ 'CC'/NOLIST'DEF' /OBJ=CRYPT.OBJ CRYPT.C
$ 'CC'/NOLIST'DEF' /OBJ=ENVARGS.OBJ ENVARGS.C
$ 'CC'/NOLIST'DEF' /OBJ=EXPLODE.OBJ EXPLODE.C
$ 'CC'/NOLIST'DEF' /OBJ=EXTRACT.OBJ EXTRACT.C
$ 'CC'/NOLIST'DEF' /OBJ=FILE_IO.OBJ FILE_IO.C
$ 'CC'/NOLIST'DEF' /OBJ=INFLATE.OBJ INFLATE.C
$ 'CC'/NOLIST'DEF' /OBJ=MATCH.OBJ MATCH.C
$ 'CC'/NOLIST'DEF' /OBJ=UNREDUCE.OBJ UNREDUCE.C
$ 'CC'/NOLIST'DEF' /OBJ=UNSHRINK.OBJ UNSHRINK.C
$ 'CC'/NOLIST'DEF' /OBJ=ZIPINFO.OBJ ZIPINFO.C
$ 'CC'/INCLUDE=SYS$DISK:[]'DEF' /OBJ=[.VMS]VMS.OBJ; [.VMS]VMS.C'x'
$ !
$ local_unzip = f$edit(local_unzip,"UPCASE,TRIM")
$ if f$locate("VMSCLI",local_unzip).ne.f$length(local_unzip)
$ then
$	'CC'/INCLUDE=SYS$DISK:[]'DEF' /OBJ=[.VMS]CMDLINE.OBJ; [.VMS]CMDLINE.C'x'
$	'CC'/INCLUDE=SYS$DISK:[]/DEF=('DEFS',SFX) /OBJ=[.VMS]CMDLINE_.OBJ; -
		[.VMS]CMDLINE.C'x'
$	set command/obj=[.vms]unz_cld.obj [.vms]unz_cld.cld
$	cliobjs = ",[.vms]cmdline.obj, [.vms]unz_cld.obj"
$	cliobjx = ",[.vms]cmdline_.obj, [.vms]unz_cld.obj"
$	set default [.vms]
$ 	edit/tpu/nosection/nodisplay/command=cvthelp.tpu unzip_cli.help
$	set default [-]
$	runoff/out=unzip.hlp [.vms]unzip_cli.rnh
$ else
$	cliobjs = ""
$	cliobjx = ""
$	runoff/out=unzip.hlp [.vms]unzip_def.rnh
$ endif
$ !
$ LINK /NOTRACE/EXE=UNZIP.EXE unzip.obj;, crypt.obj;, envargs.obj;, -
	explode.obj;, extract.obj;, file_io.obj;, inflate.obj;, match.obj;, -
	unreduce.obj;, unshrink.obj;, zipinfo.obj;, [.VMS]vms.obj; -
	'cliobjs' 'opts', [.VMS]unzip.opt/opt
$ !
$ 'CC'/DEF=('DEFS',SFX)/NOLIST /OBJ=UNZIPSFX.OBJ UNZIP.C
$ 'CC'/DEF=('DEFS',SFX)/NOLIST /OBJ=EXTRACT_.OBJ EXTRACT.C
$ 'CC'/DEF=('DEFS',SFX)/INCLUDE=SYS$DISK:[] /OBJ=[.VMS]VMS_.OBJ; [.VMS]VMS.C'x'
$ LINK /NOTRACE/EXE=UNZIPSFX.EXE unzipsfx.obj;, crypt.obj;, extract_.obj;, -
	file_io.obj;, inflate.obj;, match.obj;, [.VMS]vms_.obj; -
	'cliobjx' 'opts', [.VMS]unzipsfx.opt/opt
$ !
$ ! Next line:  put similar lines (full pathname for unzip.exe) in
$ ! login.com.  Remember to include the leading "$" before disk name.
$ !
$! unzip == "$''here'unzip.exe"		! set up symbol to use unzip
$! zipinfo == "$''here'unzip.exe ""-Z"""	! set up symbol to use zipinfo
$ !
$error:
$ tmp = f$ver(old_ver)
$ exit
