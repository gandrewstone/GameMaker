$!
$!	Clean.com --	procedure to delete files. It always returns success
$!			status despite any error or warnings. Also it extracts
$!			filename from MMS "module=file" format.
$!
$ on control_y then goto ctly
$ if p1.eqs."" then exit 1
$ i = -1
$scan_list:
$	i = i+1
$	item = f$elem(i,",",p1)
$	if item.eqs."" then goto scan_list
$	if item.eqs."," then goto done		! End of list
$	item = f$edit(item,"trim")		! Clean of blanks
$	wild = f$elem(1,"=",item)
$	show sym wild
$	if wild.eqs."=" then wild = f$elem(0,"=",item)
$	vers = f$parse(wild,,,"version","syntax_only")
$	if vers.eqs.";" then wild = wild - ";" + ";*"
$scan:
$		f = f$search(wild)
$		if f.eqs."" then goto scan_list
$		on error then goto err
$		on warning then goto warn
$		delete/log 'f'
$warn:
$err:
$		goto scan
$done:
$ctly:
$	exit 1
