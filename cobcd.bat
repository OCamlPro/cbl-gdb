@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
::
::###########################################################################
::## The version number in the next line is updated by a Linux bash script. #
::## The text "Version x.x" has to appear in square brackets.               #
::## Don't mess with it!                                                    #
::###########################################################################
::
set VERSION=[Version 3.14]
::
::
set argC=0
for %%x in (%*) do Set /A argC+=1

IF "%argC%"=="0" (
    echo This is cobcd %VERSION%
    echo It is a wrapper for GnuCOBOL cobc
    echo
    echo Use environment variables COBCD_COBC or COBC to specify the cobc executable
    goto :eof
    )

IF NOT "%COBCD_COBC%"=="" (
    set COBC=%COBCD_COBC%
    ) ELSE (
    IF "%COBC%"=="" (
        set COBC=cobc
        )
    )

FOR /F "tokens=* USEBACKQ" %%F IN (`%COBC% --info`) DO (
    set COBC_INFO=!COBC_INFO! %%F
    )

set GCC=
set ORIGINAL_COB_CFLAGS=
set IN_GCC=0
set IN_OCC=0

set EXTRACTOR="%COBC_INFO%"
:do_it_again
FOR /F "tokens=1,*" %%F IN (%EXTRACTOR%) DO (
set EXTRACTOR="%%G"

IF "%%F"=="COB_CFLAGS" (
    SET IN_GCC=3
    SET IN_OCC=1
    goto do_it_again
    )
IF "%%F"=="COB_LDFLAGS" (
    SET IN_OCC=3
    goto do_it_again
    )
IF "%IN_GCC%"=="2" (
    SET GCC=!GCC! %%F
    )
IF "%IN_GCC%"=="1" (
    IF "%%F"==":" (
        set IN_GCC=2
        )
    )
IF "%IN_GCC%"=="0" (
    IF "%%F"=="COB_CC" (
        set IN_GCC=1
        )
    )

IF "%IN_OCC%"=="2" (
    set ORIGINAL_COB_CFLAGS=!ORIGINAL_COB_CFLAGS! %%F
    )
IF "%IN_OCC%"=="1" (
    IF "%%F"==":" (
        set IN_OCC=2
        )
    )
IF "%IN_OCC%"=="0" (
    IF "%%F"=="COB_CFLAGS" (
        set IN_OCC=1
        )
    )

goto do_it_again
)

::echo GCC is %GCC%
::echo ORIGINAL_COB_CFLAGS is %ORIGINAL_COB_CFLAGS%

::
::
:: Make sure cobc can run, using the default environment if one has
:: not already been set up
::
if not defined COB_CONFIG_DIR (
    call cobenv --setenv >NUL
    )

::
:: Bust up the input parameters into one that comes before the single
:: COBOL source module we know how to handle, the COBOL module itself,
:: and the ones after
::

set BEFORESOURCE=
set SOURCEFILE=
set BASENAME=
set AFTERSOURCE=
set XorM=-m

:parameter_loop

    IF "%1"=="-x" (
        SET XorM=-x
        )
    IF /i "%~x1"==".cbl" (
        set SOURCEFILE=%1
        set BASENAME=%~n1
        goto parameter_continue
        )
    IF /i "%~x1"==".cob" (
        set SOURCEFILE=%1
        set BASENAME=%~n1
        goto parameter_continue
        )
    IF "%SOURCEFILE%"=="" (
        set BEFORESOURCE=%BEFORESOURCE% %1
        ) ELSE (
        set AFTERSOURCE=%AFTERSOURCE% %1
        )
:parameter_continue
    shift
    if not "%~1"=="" goto parameter_loop

:: echo BEFORESOURCE %BEFORESOURCE%
:: echo SOURCEFILE %SOURCEFILE%
:: echo AFTERSOURCE %AFTERSOURCE%
:: echo BASENAME %BASENAME%
:: echo XorM %XorM%

if "%BASENAME%"=="" (
    echo No input files
    goto :eof
    )

:: Do a -S compile to create the .s file
set COB_CFLAGS=-fPIC -Wa,-L,-anchls=%BASENAME%.lst %ORIGINAL_COB_CFLAGS%
%COBC% -S -d -g %XorM% --fgen-c-line-directives --fgen-c-labels -T %BASENAME%.cbl.lst --tlines=0 --tsymbols %BEFORESOURCE% %SOURCEFILE%
IF NOT ERRORLEVEL 0 GOTO bail

:: Do a -c compile to create the .h, .i, and .lst file that COBST needs
%COBC% -c -d -g %XorM% --fgen-c-line-directives --fgen-c-labels -T %BASENAME%.cbl.lst --tlines=0 --tsymbols %BEFORESOURCE% %SOURCEFILE%
IF NOT ERRORLEVEL 0 GOTO bail

:: Get rid of the .o file we won't be using, just to avoid possible confusion
erase %BASENAME%.o
IF NOT ERRORLEVEL 0 GOTO bail

:: Run COBST to create basename.sym.c
cobcd-st -q %BASENAME%
IF NOT ERRORLEVEL 0 GOTO bail

:: Compile basename.sym.c to create basename.sym.o
set COB_CFLAGS=%ORIGINAL_COB_CFLAGS%
%COBC% -c -o %BASENAME%.sym.o %BASENAME%.sym.c
IF NOT ERRORLEVEL 0 GOTO bail

:: Use SFIX to modify the basename.s file
cobcd-sfix -q %BASENAME%.s %BASENAME%-2.s %BASENAME%.c %SOURCEFILE%
IF NOT ERRORLEVEL 0 GOTO bail

:: Assemble the modified .s file
%COBC% -c -o %BASENAME%.o %BASENAME%-2.s
IF NOT ERRORLEVEL 0 GOTO bail

:: Link the two .o modules together
IF "%XorM%"=="-x" (
    %COBC% -x %BASENAME%.o %BASENAME%.sym.o %AFTERSOURCE%
    IF NOT ERRORLEVEL 0 GOTO bail
    ) ELSE (
    gcc -shared -o %BASENAME%.dll %BASENAME%.o %BASENAME%.sym.o -lcob %AFTERSOURCE%
    IF NOT ERRORLEVEL 0 GOTO bail
    )

SET COBCDPY=
for %%I in (cobcd.py) do SET COBCDPY=%%~$PATH:I

if "%COBCDPY%"=="" (
    echo Couldn't find a copy of cobcd.py on the PATH
    ) ELSE (
    copy %COBCDPY% %BASENAME%-gdb.py >NUL
    )

:bail

if "%COBCDNOCLEAN%"=="" (
    erase /Q %BASENAME%.c %BASENAME%.s %BASENAME%*.h %BASENAME%.i %BASENAME%.lst ^
    %BASENAME%.cbl.lst %BASENAME%.full.tab %BASENAME%.tab %BASENAME%.o ^
    %BASENAME%.cbl.o %BASENAME%-2.lst %BASENAME%.sym.c %BASENAME%.sym.o ^
    %BASENAME%-2.s %BASENAME%.o  %BASENAME%-2.o
    )

ENDLOCAL