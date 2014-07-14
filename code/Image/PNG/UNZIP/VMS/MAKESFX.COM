$!
$!  MAKESFX.COM:  command-procedure to create self-extracting ZIP archives
$!                usage:  @MAKESFX foo    (foo.zip -> foo.exe)
$!
$!  Martin P.J. Zinser 940804
$!
$!
$!  For this to work a symbol unzipsfx has to be defined which contains the 
$!  location of the unzip stub (e.g., unzipsfx:== device:[dir]unzipsfx.exe)
$!
$!  The zipfile given in p1 will be concatenated with unzipsfx and given a
$!  filename extension of .exe.  The default file extension for p1 is .zip
$!
$!  Use at your own risk, there is no guarantee here.  If it doesn't work,
$!  blame me (m.zinser@gsi.de), not the people from Info-ZIP.
$!
$!
$ inf = "''p1'"
$ usfx = f$parse("''unzipsfx'") - ";"
$ file = f$parse("''inf'",,,"DEVICE") + f$parse("''inf'",,,"DIRECTORY") + -
  f$parse("''inf'",,,"NAME") 
$ finf = "''file'" +f$parse("''inf'",".ZIP",,"TYPE") + -
  f$parse("''inf'",,,"VERSION")
$!
$! [GRR 940810:  what is the point of 'name'?  example?  commented out...]
$! $ name = f$extract(12,2,f$time()) + f$extract(15,2,f$time()) + -
$!   f$extract(18,2,f$time()) + f$extract(21,1,f$time())
$!
$ copy 'usfx','finf' 'file'.exe
$ exit
