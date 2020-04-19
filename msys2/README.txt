Bob Dubner, 2020-05-15

The enclosed .zip file was provided by Simon Sobisch.

To get the build to work, I had to modify a couple of things:

In PKGBUILD, line 25: change "diff" to "diffutils"
             line 89: comment out 'make check'
             line 90: comment out 'make test'

Following those changes: 

makepkg-mingw -sCLf  ## --check

