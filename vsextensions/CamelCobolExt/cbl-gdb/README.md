# Camelian Systems COBOL debugger

This extension provides source-level debugging of COBOL programs compiled with the GnuCOBOL COBOL compiler.  

## Basic methodology

COBOL source code is compiled with the GnuCOBOL compiler.  Debugging information and a gdb extension are added to the executable by additional processing.  The executable is then debugged with this extension, which provides a Debug Adapter that is aware of the extensions to gdb that makes source-level debugging possible.

## Installation

You probably found this README along with a file named cbl-gdb-x.y.z.vsix, which is a Visual Studio Code extension package.  You have a couple of options for loading the extension into VS code:

- From inside Visual Studio Code, use the Command Palette (Ctrl+Shift+P) to find "Extensions: Load extension from VSIX..."  Select that, and point it at the .VSIX file

- From the command line, execute `code --install-extension cbl-gdb-x.y.z.vsix1`

## Additional extensions

There are a number of COBOL formatting extensions available at Microsoft's Extension Marketplace.  We've found that "Enterprise COBOL for z/OS" seems to coexist with our Debugging Adapter/

## Next steps

You likely downloaded this extension in a package that includes a folder named `starter-cbl-gdb`.  Take a look in there for information about setting up a Visual Studio Code workspace for compiling, running, and debugging COBOL source code.

## Attaching to existing processes

If you are in the business of attaching to existing processes, you'll need an appropriate configuration in your launch.json file.  You'll need an `attach` configuration and an `inputs` section:

```
    "configurations": [
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
```
The above methodology is demonstrated in CamelCobolExt/samples/launchso

When you execute `Debug/Start Debugging (F5)` and choose `Attach to COBOL debugger`,you will be asked `Enter the PID to attach to`.  You'll need to know the PID (perhaps the application will have been designed to tell you what its PID is, or you'll have found the PID through ps(1) or pgrep(1)

Take special note of the `"solibs"` variable.  As described here it gets set from a global environment variable.  It doesn't have to be set that way, but if the application involves dynamically-loaded shared libraries, the `"solibs"' variable somehow has to be set to a colon-separated list of folders that include the location of your libraries.  If it isn't set, then the instance of gdb launched by Visual Studio Code won't be able to find the symbols for those libraries.