readme.txt - for pnglib 0.6

This is a draft version of pnglib, not the final version.  It may not
work.  Furthermore, some of the library is not finished, mostly stuff
that is not part of the "core" library.  See pngtodo.txt for more
details.

Having said that, I should mention that it will read and write
PNG files.  The "core" of the library seems solid enough to allow
those who are interested to experiment with it.  I expect to finish
a Beta version in a week or two, along with a testing program to
allow easy testing on various platforms.  If that goes well, a
full version can be available as early as June.  Please be patient,
as this is an all volunteer effort.

For a detailed description on using pnglib, read pnglib.txt.  For
usage information and restrictions (what little they are) on pnglib,
see png.h.  For a description on using zlib (the compression library
used by pnglib) and zlib's restrictions, see zlib.h

I have included a make file, but you will probably have to modify it
for your own needs.  I'm using Borland C++, running large memory
model on Windows 3.11, but it should work on almost anything.  Support
for medium memory model is planned, but is not in this version.
You will get some "parameter not used" errors in pngrtran.c.  These
functions are not finished, so the code is ifdef'ed out.  Just
ignore these warnings.

You will need zlib 0.9 to run this.  That is the current version,
although later versions should work also.  zlib is a compression
library that is useful for more things then just png files.  If
you need a compression library, check out zlib.h

zlib should be available at the same place that pnglib is.
If not, it should be at ftp.uu.net in /graphics/png
Eventually, it will be at ftp.uu.net in /pub/archiving/zip/zlib

You will also want a copy of the PNG specification.  It should
be available at the same place you picked up pnglib.  If it is
not there, try ftp.uu.net in the /graphics/png directory.

This version is dated May 1, 1995.  If it is significantly after that,
look for a new version.  I'm not sure where the final code will go,
but you may try ftp.uu.net in the /graphics/png directory, or
ftp.group42.com, or, on CompuServe, Lib 20 at GO GRAPHSUPPORT
If you can't find it in any of those places, e-mail me, and I'll
tell you where it is.

If you have any code changes, requests, problems, etc., please e-mail
them to me.  Also, I'd appreciate any make files or project files.
Please mention "pnglib" somewhere in the subject line.  Thanks.

We are about to change our internet address, but for now, you can
reach me at:

internet: group42@interserv.com
CompuServe: 75501,1625

Please do not send me general questions about PNG.  Send them to
the address in the specification.  At the same time, please do
not send pnglib questions to that address, send them to me.  I'll
get them in the end anyway.  If you have a question about something
in the PNG specification that is related to using pnglib, send it
to me.  Send me any questions that start with "I was using pnglib,
and ...".  If in doubt, send questions to me.  I'll bounce them
to others, if necessary.

Please do not send suggestions on how to change PNG.  We have
been discussing PNG for 4 months now, and it is official and
finished.  If you have suggestions for pnglib, however, I'll
gladly listen.  Even if your suggestion is not used for version
1.0, it may be used later.

Good luck, and happy coding.

-Guy Eric Schalnat
 Group 42, Inc.
 internet: group42@interserv.com
 CompuServe: 75501,1625
 Coming soon: www.group42.com, ftp.group42.com

