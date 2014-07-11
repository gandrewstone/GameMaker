rem
rem ObjectWindows - (C) Copyright 1992 by Borland International
rem
rem Build OWL
rem
cd source
rem
rem build static link libraries
rem
make -DMODEL=s
make -DMODEL=m
make -DMODEL=l
rem
rem build DLL
rem
make -DDLL
cd ..
