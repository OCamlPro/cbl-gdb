This folder is intended to provide a standalone GnuCOBOL compiler that runs
natively on Windows.

It was built on Windows 10 Profession with the MSYS2/MINGW32 toolchain, 
so the cobc.exe program and supporting libraries are all i686 32-bit executables.

The included gcc compiler is version 9.3.  It is the latest versions, as of 
this writing, provided by the MINGW32 toolchain.

You probably received this package as a 7-zip self-extracting executable, 
and you probably let it install itself as C:\gnucobol\

The way you use it is to execute

    C:\cblgdb\bin\cobenv.cmd

That sets up the execution path and the environment variables for
GnuCOBOL.  You can then compile a COBOL program like this

    cobc -x your_program.cbl

Please contact me at rdubner@symas.com with questions, comments, complaints,
bugs, or (with any luck!) accolades.

Special thanks go to Arnold Trembley and Simon Sobisch.  Putting this package
together involved a number of procedures and concepts that were new to me, and
the effort would have been much harder without Simon's help and Arnold's 
documenation.

Bob Dubner
May 5, 2020






