Notes 2019-12-10, Bob Dubner

This folder is for developing the cbl-dbg extension.  

Making sure you have the correct software installed so that you can install
additional correct software is the usual infinite regress bookended in insanity.  
I no longer recall how I got bootstrapped in; I think I installed `snap` (or
maybe it was already present on Ubuntu) and then used 

	sudo snap install --classic code 
	sudo snap install --classic code-insiders
	
for the regular and bleeding-edge versions of Visual Studio Code.  You probably
don't need the code-insiders version.

I don't at what point npm was installed.  I do know that getting `vsce` to work
involved running

	npm install -g vsce

When this folder is initially cloned from a git repository, it is necessary to 
run

	npm install
	
to appease the Microsoft software development demons.  (It appears to create
a necessary massive folder called "node_modules".  I don't know what else it
might be doing.)

Creating the VSIX extension installation file is accomplished by running
	
	vsce package
	
The resulting cbl-dbg-x.x.x.vsix package needs to be installed on a user's
system.  This can be done by launching Visual Studio Code and displaying the
Extensions pane (Ctrl+Shift+X), then clicking the three-dot "More actions" item,
and then selecting "Install from VSIX...".

You can also get there via the Command Palette (Ctrl+Shift+P) and finding 
"Extensions: Install from VSIX...", which you can find by typing "VSIX".

It's also possible to install the VSIX package from the command line:

	code --install-extension cbl-dbg-x.x.x.vsix

With the cbl-dbg extension installed and enabled, you need to look elsewhere
for the cbl-dgb-template folder for the starting point for compiling and
debugging actual COBOL code.


