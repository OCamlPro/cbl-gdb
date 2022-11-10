# COBOLworx GnuCOBOL Source Level Debugging Support

These are the source files for building the tools that make up the COBOLworx debugging extensions for GnuCOBOL.

## Requirements

You'll need to have GnuCOBOL Version 3.1 (or later) installed.  3.1 has features needed by these extensions.

These tools are known to compile with GCC-4.8.5 and later.  They have been demonstrated to work on Centos7/RHEL7, Centos8/RHEL8, and Ubuntu18.4.
During building the binaries `xxd` and possibly `sed` are necessary, too.

Actual debugging requires that your version of GDB was compiled with Python2 or Python3 support.  The default installations of GDB in the above-mentioned operating systems are known to have Python support.

## Building and Installation

Building the source, and installing the binaries and scripts operates as you'd generally expect:

	make 
	sudo make install

By default, the executables are installed in /usr/local/bin.  If you choose to install them elsewhere, you can do it like this:

	sudo make PREFIX=<prefix> install

will cause the executables to be installed in <prefix>/bin

The installation can be reversed with one or the other of

	sudo make uninstall
	sudo make PREFIX=<prefix> uninstall

## How to use the debugging extensions

Using the debugging extensions is generally a case of using the cobcd script where you'd ordinarily use the cobc command to compile a COBOL program:

Where you would enter the command 

	cobc -x prog.cbl

you instead enter

	cobcd -x prog.cbl

After that, you can debug the resulting binary with

	gdb ./prog

## Source code for the extensions

The source code for the extension is in:

	cobcd-st/
	cobcd-sfix/
	python/
	include/

Utility scripts used during development and deployment are found in 

    bin/ 

## Sample code for debugging

The directory `optfde01/` contains a COBOL program that, while simple, is a little more interesting than "hello, world".  (It reads three records from one file and writes them to another.)  After you do a 'make install' you'll be able to enter this directory and run the Makefile.  optfde01 also serves as an example of how to use Visual Studio Code to debug COBOL.  The optfde01/.vscode/ subdirectory contains the launch.json and tasks.json files that you'll clone into your own projects.  The program is found in the directory

## GIT repository structure

There are two branches that are required to be present:

origin/master is supposed to be stable and buildable at all times.

origin/relcan, for "release candidate", is used by the cbl-gdb-distribution repositories package-building routines for creating packages.  My procedure lately has been to fuss and fiddle with relcan until the completed final packages work properly, and then merge relcan into master.  That merge gets tagged as a release version, and at that point ../include/version.h is incremented.  The build process has scripts that propogate version.h through the various executables.

*Note: you may notice a branch named `bobdev`.  It is what it sounds like: a place where I am doing development. It probably should come with a sign over the door: **Here there be dragons**.*

Feel free to contact me with any questions or problems.

2020-07-07

Bob Dubner
COBOLworx
support@cobolworx.com
