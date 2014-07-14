bcc -ml -O2 image.cpp d:\drv\fileclss.cpp d:\drv\viddrv.lib > bugs
REM tlink -v @image.lnk  >> bugs
type bugs | more
