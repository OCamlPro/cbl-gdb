#!/bin/bash

## Modifies cprint.py, if necessary, with the VERSION text
## found in ../version.h

## Extract '2.4' from '#define VERSION "2.4"'
input="../version.h"
while read -r line
  do
    IFS=' '; arrIN=($line); unset IFS;
    ## Find the line in question:
    if test X"${arrIN[0]}" == X"#define" && test X"${arrIN[1]}" == X"VERSION" ; then
      # Remove the leading and trailing double-quotes:
      VERSION=$(sed -e 's/^"//' -e 's/"$//' <<<"${arrIN[2]}")
      fi
  done < "$input"

VERSION_STRING='[Version '$VERSION']'

#echo VERSION is $VERSION
#echo VERSION_STRING is $VERSION_STRING

if ! grep -q -F "$VERSION_STRING" cprint.py ; then
    ## We need to replace the existing out-of-date version string
    ## with our new one
    echo Updating cprint.py version to $VERSION_STRING
    sed -i -r "s/[[]Version [0-9]+[.][0-9]+[]]/$VERSION_STRING/g" cprint.py 
    fi
