@echo off
echo.
echo Initialising for DJGPP build...

rem make a copy of Makefile.in with an extra line at the beginning
rem enabling DJGPP build

echo DJGPPDOS=1 > Makefile
type Makefile.in >> Makefile

rem copy config.dj to config.h

copy config.dj config.h | echo.

echo done.
echo.
echo Make sure that you have DJGPP and Allegro installed properly.
echo Also, if you want a build with TCP/IP support, make sure that
echo you have installed libsocket (http://www.libsocket.tsx.org/)
echo If you want a version without TCP/IP, you need to edit the
echo Makefile and make the following changes:
echo.
echo     CFLAGS_NEWFEATURES=-DDOGS -DTCPIP
echo ->  CFLAGS_NEWFEATURES=-DDOGS
echo     LIBS=-lalleg -lm -lemu -lsocket
echo ->  LIBS=-lalleg -lm -lemu
echo.

