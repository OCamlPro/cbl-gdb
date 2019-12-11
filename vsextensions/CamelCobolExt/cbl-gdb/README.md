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

