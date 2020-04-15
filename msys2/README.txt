Bob Dubner, 2020-05-15

This is the tale of a wasted week, trying to get the COBOL debugging toolchain
working in a native Windows way using the MSYS2 environment.

To get back to where I was when I shut down, you would need to install 64-bit
MSYS2 from www.msys2.org on a 64-bit Windows Professional platform.

Configure and install it thusly:
    
running as msys 64-bit: (C:\msys64\msys2_shell.cmd -mingw64)

pacman -S base-devel
pacman -S subversion
pacman -S rsync
pacman -S mingw-w64-x86_64-toolchain

There are a couple of files in the mingw-w64-gnucobol-svn.zip package that
Simon Sobisch sent me on 2020-04-14 that need to be renamed:

cobenv.ctxt becomes cobenv.cmd
cobenv.stxt becomes cobenv.sh

To get the build to work, I had to modify a couple of things:

In PKGBUILD, line 25: change "diff" to "diffutils"
             line 89: comment out 'make check'
             line 90: comment out 'make test'

# Simon said to run this:
makepkg-mingw -sCLf --check

makepkg-mingw -sCLf

Ultimate that worked, creating an install package.  Not knowing
what to do with it, I went to ./src/build-x86_64 and

make install

That installed cobc in /mingw64/bin

Magic environment variables, which I put into .profile, allowed the
cobc compiler to work.

export COB_CONFIG_DIR=c:/msys64/mingw64/share/gnucobol/config
export COB_LIBRARY_PATH=C:/msys64/mingw64/lib
export COB_COPY_DIR=C:/msys64/mingw64/share/gnucobol/copy

I came up with a compilation script, here named "cobcd_for_msys2", which
creates a debuggable executable.

The bad news:  When gdb 9.1 loads that executable, you have to execute
"l rtest.cbl:1" before it understands you are working with an executable
pointing back to the source code module rtest.cbl.  With a .c module, gdb seems
to default to the proper .c code; with .cbl it seems confused until you "prime
the pump" with that explicit listing command.

You need to have a "rtest-gdb.py" copy of the cprint.py script where gdb can find
it.  (The method used in Linux, which is to embed cprint.py into the executable,
doesn't work in Windows.  Unlike a ELF file, the COFF/PE+ file format doesn't have
provision for a .debug_gdb_scripts section.)

The additional bad news: The Python script doesn't quite work.  It tries to
run, but the "p *" command doesn't find all of the variables that it ought to.
Plus, some of the variables that it does find don't seem to display properly.

It needs debugging.

And that is where I said, "Hold, enough!"








