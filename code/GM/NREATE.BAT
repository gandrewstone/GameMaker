@echo off
rem
rem **************************************************************************
rem *                                                                        *
rem *           Command Procedure to Build any EXE Image                     *
rem *                                                                        *
rem *           %2==DEBUG or debug Will Build a Debug Executable Image       *
rem *                                                                        *
rem *           %1  specifies file to compile                                * 
rem *                                                                        *
rem **************************************************************************
rem
if "%1" == "" goto SHOW_FORMAT
if not exist %1.mak goto BADFILE
if "%2" == "" goto BLNODEBUG
if %2 == debug goto BLDEBUG
if %2 == DEBUG goto BLDEBUG
rem
rem **************************************************************************
rem *           Invalid Build Option Entered                                 *
rem **************************************************************************
rem
echo Invalid Build Parameter Entered
goto EXIT
rem
rem **************************************************************************
rem *           No File specified!                                           *
rem **************************************************************************
rem
:SHOW_FORMAT
echo [44m
echo   ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿  
echo   ³                                         ³  
echo   ³    The format for this batch file is:   ³  
echo   ³       create {filename} [DEBUG]         ³  
echo   ³                                         ³  
echo   ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  
echo [40m
goto EXIT
rem
rem **************************************************************************
rem *           A non-existant make file was requested.                      *
rem **************************************************************************
rem
:BADFILE
echo [35;40m
echo   ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ  
echo.                                              
echo      The filename %1.mak does not exist     
echo                 as a make file.                
echo.                                              
echo   ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ  
echo [37m
goto EXIT
rem
rem **************************************************************************
rem *           Build With Debug Option Entered                              *
rem **************************************************************************
rem
:BLDEBUG
echo Build %1 With Debug Logic
make -f%1.mak -DOPTION=1 > %1.log
goto TYPELOG
rem
rem **************************************************************************
rem *           Build Without Debug Option Entered                           *
rem **************************************************************************
rem
:BLNODEBUG
echo Build %1 Without Debug Logic
make -f%1.mak -DOPTION=0 >%1.log
rem
rem **************************************************************************
rem *           Type the Log on the Console                                  *
rem **************************************************************************
rem
:TYPELOG
type %1.log
rem
rem **************************************************************************
rem *           Exit the Build Procedure                                     *
rem **************************************************************************
rem
:EXIT
echo on

