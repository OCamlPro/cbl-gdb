# Development Notes 2019-12-10, Bob Dubner

This folder is for developing the cbl-dbg extension.  

## First, of course, you have to have Visual Studio Code installed

Making sure you have the correct software installed so that you can install
additional correct software is the usual infinite regress bookended in insanity.  
I no longer recall how I got bootstrapped in; I think I installed `snap` (or
maybe it was already present on Ubuntu) and then used 

```
sudo snap install --classic code 
sudo snap install --classic code-insiders
```
	
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

## Everything is installed.  Now what?

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

## Oh, you want to do development on the extension?

You have guts.  Either that, or you are me, and you are trying to remember what you did in the first place.

Here's where you are going to be going:  When you are actively debugging the debugger, there will be not one, not two, but _three_ Visual Studio Code instances running.

The `debugging adapter`, which is likely what you are most interested in changing, will be running from this folder as the `Server` configuration.

The `extension package`, which acts as a bridge between a user and the debugging adapter, will be running as a second instance in this folder as the `Extension` configuration.

The extension configuration launches a third instance of Visual Studio Code.  You'll use that one to open the folder containing the test program that will be debugged by the debugging adapter.  At this writing, this development-notes.md file is in a folder named `CamelCobolExt/cbl-gdb`.  Some suitable test programs can be found in `CamelCobolExt/samples`.

## The three launch configurations

Take a look at this project's `launch.json` file.

### The `"Extension"` configuration

The key thing about the Extension configuration is that it launches `execPath`, which means yet another copy of Visual Studio Code.

### The `"Server"` configuration

The Server configuration launches the debug adapter in Server mode, which is determined by the presences of the `"args": [ "--server=4711" ],` parameter.  In this mode, it listens for an attempt to connect on port 4711, rather than doing the usual extension thing.  (The "usual extension thing" is defined by the `"cbl-gdb"` configuration in the extensions package.json file, also found in this folder.)

### The `Client` configuration

The Client configuration controls what happens when the third instance of Visual Studio Code, the one launched by the Extensions configuration, is used to `F5/Start Debugging`.  Here is the launch.json file I've been using for targets used for debugging the debug adapter:

```
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Attach to debugServer",
            "type": "node",
            "request": "attach",
            "cwd": "${workspaceFolder}",
            "debugServer": 4711,
            "solibs":"${env:PRIM_LIBRARY_PATH}",
            "target": "${input:attachtopid}",
        },
        {
            "name": "Attach to COBOL debugger",
            "type": "cbl-gdb",
            "request": "attach",
            "cwd":"${workspaceFolder}",
            "solibs":"${env:PRIM_LIBRARY_PATH}",
            "target": "${input:attachtopid}",
        }
    ],
    "inputs": [
        {
            "id": "attachtopid",
            "type": "promptString",
            "description": "Enter the PID to attach to",
            "default": ""
        }
    ]
}
```
Note how the `Attach to debugServer` configuration expects to connect to a server instance through the matching port 4711.  

The `Attach to COBOL debugger` isn't used during sessions when one is debugging the debugger, but it is an example of how one uses the debug adapater extension once it is packaged up and installed.

It should be mentioned that the `"inputs"` section defines the `attachtopid` input, which causes Visual Studio Code to ask the user for the PID that is going to be debugged after gdb attaches to it.










