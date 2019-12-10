This is the root folder of the development area for GnuCOBOL + CamelianMods
+ gdb debugging.

As of this writing (2019-12-03) the only thing that exists is in the 
OriginalCamelianCobolExtension folder.  Dating back to August 2019, it was 
quickly put together to enable line breakpoints in obmod-modified executables.

It knew nothing about COBOL variables, which could only be inspected by 
command line entries in the debug console window.

That was pretty cool, but the idea is present the Visual Studio Code user with
a more unified presentation: not just breakpoints, but COBOL variables.

That effort starts here.

