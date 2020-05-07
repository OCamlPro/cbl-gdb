# Camelian.Systems GnuCOBOL Source Level Debugging Support

These are the source files for building the tools that make up the Symas/Camelian debugging extensions for GnuCOBOL.

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
    
There is a .vsix file containing the Visual Studio Code extension that knows how to use the Symas/Camelian debugging extensions to do source-level debugging of COBOL code in the VSC IDE.  (VSC integration is still under development.) It is found in 

    vsdistrib/

optfde01 is a COBOL program that, while simple, is a little more interesting than "hello, world".  (It reads three records from one file and writes them to another.)  After you do a 'make install' you'll be able to enter this directory and run the Makefile.  optfde01 also serves as an example of how to use Visual Studio Code to debug COBOL.  The optfde01/.vscode/ subdirectory contains the launch.json and tasks.json files that you'll clone into your own projects.  The program is found in the directory

    optfde01/ 

Feel free to contact me with any questions or problems.

2020-05-07

Bob Dubner
Symas/Camelian
rdubner@symas.com
