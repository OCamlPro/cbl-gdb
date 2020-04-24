@ECHO OFF
SETLOCAL

set XorM=-m

IF "%1"=="-x" (
    SET XorM=-x
    SHIFT
    )
IF "%1"=="-m" (
    SET XorM=-m
    SHIFT
    )
  
set SOURCEFILE=%1
set BASENAME=%~n1

if "%BASENAME%"=="" (
    echo No input files
    goto :eof
    )

:: Do a -S compile to create the .s file
set COB_CFLAGS=-fPIC -Wa,-L,-anchls=%BASENAME%.lst -I/mingw32/include -Wno-unused -fsigned-char -Wno-pointer-sign -D__USE_MINGW_ANSI_STDIO=1 -I/mingw32/include -march=i686 -mtune=generic -pipe -pipe
cobc -S -d -g %XorM% --fgen-c-line-directives --fgen-c-labels -T %BASENAME%.cbl.lst --tlines=0 --tsymbols  %SOURCEFILE%

:: Do a -c compile to create the .h, .i, and .lst file that COBST needs
cobc -c -d -g %XorM% --fgen-c-line-directives --fgen-c-labels -T %BASENAME%.cbl.lst --tlines=0 --tsymbols  %SOURCEFILE%

:: Get rid of the .o file we won't be using, just to avoid possible confusion
erase %BASENAME%.o

:: Run COBST to create basename.sym.c
cobst -q %BASENAME%

:: Compile basename.sym.c to create basename.sym.o
set COB_CFLAGS=
cobc -c -o %BASENAME%.sym.o %BASENAME%.sym.c

:: Use SFIX to modify the basename.s file
sfix -q %BASENAME%.s %BASENAME%-2.s %BASENAME%.c %SOURCEFILE%

:: Assemble the modified .s file
cobc -c -o %BASENAME%.o %BASENAME%-2.s

:: Link the two .o modules together
IF "%XorM%"=="-x" (
    cobc %XorM% %BASENAME%.o %BASENAME%.sym.o 
    ) ELSE (
    gcc -shared -o %BASENAME% %BASENAME%.o %BASENAME%.sym.o -lcob 
    )

SET CPRINTPY=
for %%I in (cprint.py) do SET CPRINTPY=%%~$PATH:I


if "%CPRINTPY%"=="" (
    echo Couldn't find a copy of cprint.py on the PATH
    ) ELSE (
    copy %CPRINTPY% %BASENAME%-gdb.py >NUL
    )

if "%COBCDNOCLEAN%"=="" (
    erase /Q %BASENAME%.c %BASENAME%.s %BASENAME%*.h %BASENAME%.i %BASENAME%.lst ^
    %BASENAME%.cbl.lst %BASENAME%.full.tab %BASENAME%.tab %BASENAME%.o ^
    %BASENAME%.cbl.o %BASENAME%-2.lst %BASENAME%.sym.c %BASENAME%.sym.o ^
    %BASENAME%-2.s %BASENAME%.o
    )

