This folder is intended to provide a standalone GnuCOBOL compiler with
Symas/Camelian debugging capability that runs natively on Windows.

It was built on Windows 10 Profession with the MSYS2/MINGW32 toolchain, 
so the cobc.exe program and the Camelian programs -- cobst.exe and 
sfix.exe -- are all i686 32-bit executables.

The included gcc compiler is version 9.3, and the gdb debugger is 9.1; they
are the latest versions, as of this writing, provided by the MINGW32 toolchain.

You probably received this package as a 7-zip self-extracting executable, 
and you probably let it install itself as C:\cblgdb\

The way you use it is to execute

    C:\cblgdb\bin\cobenv.cmd

That sets up the execution path and the environment variables for
GnuCOBOL.  You can then compile a COBOL program like this

    cobcd -x your_program.cbl

You can then debug it with

    gdb your_program

When you do that, you'll be confronted with a (gdb) prompt.  Use the "list"
command to look at the code.  Use the "break" command to set a breakpoint, and
use the "run" command to start the program.

The Camelian debugging package intercepts the "print" command in order to
allow the display of COBOL variables.

"print *" shows you all the variables in the current execution context.

"print 4" shows you the fourth variable in the "print *" list.

"print num" shows you all variables with "num" in their name.

There is a /d switch that shows debugging information for a variable:  Use
the name (or the number) like this:

    print/d specific_name
    print/d 4
    
There is also a "watch" capability, where the code traps when a variable
changes.  The command for that is "cwatch":

    cwatch my-counter

Please contact me at rdubner@symas.com with questions, comments, complaints,
bugs, or (with any luck!) accolades.

Special thanks go to Arnold Trembley and Simon Sobisch.  Putting this package
together involved a number of procedures and concepts that were new to me, and
the effort would have been much harder without Simon's help and Arnold's 
documenation.



Bob Dubner
April 26, 2020






