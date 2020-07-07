# COBOLworx GnuCOBOL Source Level Debugging Support

These are the source files for building the tools that make up the COBOLworx debugging extensions for GnuCOBOL.

You'll need to have GnuCOBOL Version 3.x-dev (or later) installed.

Building the source, and installing the binaries and scripts into /usr/local, operates as you'd generally expect:

	make 
	sudo make install

Using the debugging extensions is generally a case of using the cobcd script where you'd ordinarily use the cobc command to compile a COBOL program:

Where you would enter the command 

	cobc -x hello.cbl

you instead enter

	cobcd -x hello.cbl

After that, you can debug the resulting binary with

	gdb ./hello

The source code for the extension is in:

	cobcd-st/
	cobcd-sfix/
	python/
	include/

Utility scripts used during development and deployment are found in 

    bin/ 

optfde01 is a COBOL program that, while simple, is a little more interesting than "hello, world".  (It reads three records from one file and writes them to another.)  After you do a 'make install' you'll be able to enter this directory and run the Makefile.  optfde01 also serves as an example of how to use Visual Studio Code to debug COBOL.  The optfde01/.vscode/ subdirectory contains the launch.json and tasks.json files that you'll clone into your own projects.  The program is found in the directory

    optfde01/ 

It should be emphasized that there are now two branches that are required to be present:

origin/master is supposed to be stable and buildable at all times.

origin/relcan, for "release candidate", is used by the cbl-gdb-distribution repositories package-
building routines for creating packages.  My procedure lately has been to fuss and fiddle
with relcan until the completed final packages work properly, and then merge relcan into
master.  That merge gets tagged as a release version, and at that point ../include/version.h
is incremented.

Feel free to contact me with any questions or problems.

2020-05-07

Bob Dubner
COBOLworx
support@cobolworx.com
