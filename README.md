# Camelian.Systems GnuCOBOL Source Level Debugging Support

These are the source files for building the tools that make up the Symas/Camelian debugging extensions for GnuCOBOL.

You'll need to have Version 3.x-dev, or later, of GnuCOBOL installed to actually make things work.

Building the source, and installing the binaries and scripts into /usr/local, operates as you'd generally expect:

	make 
	sudo make install

Using the extensions is generally a case of using the cobcd script where you'd ordinarily use the cobc program:

Instead of 

	cobc -x hello.cbl

you instead enter

	cobcd -x hello.cbl

After that, you can debug the resulting binary with

	gdb ./hello
	
The extensions' functionality is provided by the information in these folders:

	cobcd-st/
	cobcd-sfix/
	python/
	include/

.legacy/ is code that I used along the way, but is currently not involved in production or development, and most likely is thoroughly broken.

bin/ contains utility scripts used during development and deployment

vsdistrib/ contains the .vsix file that provides a Visual Studio Code extension that knows how to use the Symas/Camelian to do source-level debugging of COBOL code in the VSC IDE.

optfde01/ contains a simple program that can be compiled and debugged after you have done the 'make install'  The optfde01/.vscode/ subdirectory contains the launch.json and tasks.json files that you'll clone into your own projects.

Feel free to contact me with any questions or problems.

2020-05-07

Bob Dubner
Symas/Camelian
rdubner@symas.com





