# Borland C++ - (C) Copyright 1991 by Borland International
# Makefile for WHELLO program

whello.exe: whello.obj whello.def whello.res
    tlink /Tw /v /n /c C:\BORLANDC\LIB\c0ws whello,\
          whello,\
          ,\
          C:\BORLANDC\LIB\cws C:\BORLANDC\LIB\cs C:\BORLANDC\LIB\import,\
          whello 
    rc whello.res

.cpp.obj :
    BCC -c -ms -v -W $<

.rc.res :
    rc -r -iC:\BORLANDC\INCLUDE $<

