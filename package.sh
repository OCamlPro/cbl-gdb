#!/bin/bash

## Creates a source-code package tarball.
## 

## Extract '2.4' from '#define VERSION "2.4"'
input="version.h"
while read -r line
  do
    IFS=' '; arrIN=($line); unset IFS;
    ## Find the line in question:
    if test X"${arrIN[0]}" == X"#define" && test X"${arrIN[1]}" == X"VERSION" ; then
      # Remove the leading and trailing double-quotes:
      VERSION=$(sed -e 's/^"//' -e 's/"$//' <<<"${arrIN[2]}")
      fi
  done < "$input"
  
## Make a copy of this working folder, but with the VERSION in its name
CWD=$(pwd)
cd ..
cp -r $CWD/ cblgdb-$VERSION/

## Make a tarball out of it
tar --exclude=.git --exclude=.legacy -czf cblgdb-$VERSION.tar.gz cblgdb-$VERSION
## Make a copy of the tarball, with "latest" in place of the version
cp cblgdb-$VERSION.tar.gz cblgdb-latest.tar.gz
## Make an "alpha" copy, which is used for internal builds that might
## not yet be releasable.  This is used by the centos, debian, and arm
## packaging routines.
cp cblgdb-$VERSION.tar.gz cblgdb-latest-alpha.tar.gz

## Get rid of the extra directory
rm -fr cblgdb-$VERSION/

