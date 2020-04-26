@ECHO OFF
SETLOCAL
::
:: Make sure cobc can run, using the default environment if one has
:: not already been set up
::
if not defined COB_CONFIG_DIR (
    call cobenv --setenv >NUL
    )

::
:: Bust up the input parameters into one that come before the single
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
set COB_CFLAGS=-fPIC -Wa,-L,-anchls=%BASENAME%.lst -I/mingw32/include -Wno-unused -fsigned-char -Wno-pointer-sign -D__USE_MINGW_ANSI_STDIO=1 -I/mingw32/include -march=i686 -mtune=generic -pipe -pipe
cobc -S -d -g %XorM% --fgen-c-line-directives --fgen-c-labels -T %BASENAME%.cbl.lst --tlines=0 --tsymbols %BEFORESOURCE% %SOURCEFILE%
IF NOT ERRORLEVEL 0 GOTO bail

:: Do a -c compile to create the .h, .i, and .lst file that COBST needs
cobc -c -d -g %XorM% --fgen-c-line-directives --fgen-c-labels -T %BASENAME%.cbl.lst --tlines=0 --tsymbols %BEFORESOURCE% %SOURCEFILE%
IF NOT ERRORLEVEL 0 GOTO bail

:: Get rid of the .o file we won't be using, just to avoid possible confusion
erase %BASENAME%.o
IF NOT ERRORLEVEL 0 GOTO bail

:: Run COBST to create basename.sym.c
cobst -q %BASENAME%
IF NOT ERRORLEVEL 0 GOTO bail

:: Compile basename.sym.c to create basename.sym.o
set COB_CFLAGS=
cobc -c -o %BASENAME%.sym.o %BASENAME%.sym.c
IF NOT ERRORLEVEL 0 GOTO bail

:: Use SFIX to modify the basename.s file
sfix -q %BASENAME%.s %BASENAME%-2.s %BASENAME%.c %SOURCEFILE%
IF NOT ERRORLEVEL 0 GOTO bail

:: Assemble the modified .s file
cobc -c -o %BASENAME%.o %BASENAME%-2.s
IF NOT ERRORLEVEL 0 GOTO bail

:: Link the two .o modules together
IF "%XorM%"=="-x" (
    cobc -x %BASENAME%.o %BASENAME%.sym.o %AFTERSOURCE%
    IF NOT ERRORLEVEL 0 GOTO bail
    ) ELSE (
    gcc -shared -o %BASENAME%.dll %BASENAME%.o %BASENAME%.sym.o -lcob %AFTERSOURCE%
    IF NOT ERRORLEVEL 0 GOTO bail
    )

SET CPRINTPY=
for %%I in (cprint.py) do SET CPRINTPY=%%~$PATH:I

if "%CPRINTPY%"=="" (
    echo Couldn't find a copy of cprint.py on the PATH
    ) ELSE (
    copy %CPRINTPY% %BASENAME%-gdb.py >NUL
    )

:bail

if "%COBCDNOCLEAN%"=="" (
    erase /Q %BASENAME%.c %BASENAME%.s %BASENAME%*.h %BASENAME%.i %BASENAME%.lst ^
    %BASENAME%.cbl.lst %BASENAME%.full.tab %BASENAME%.tab %BASENAME%.o ^
    %BASENAME%.cbl.o %BASENAME%-2.lst %BASENAME%.sym.c %BASENAME%.sym.o ^
    %BASENAME%-2.s %BASENAME%.o  %BASENAME%-2.o
    )

ENDLOCAL