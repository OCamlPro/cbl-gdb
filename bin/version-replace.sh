#!/bin/bash

## This script is used to modify python/cobcd.py, cobcd and cobcd.bat
##
## It looks for [Version.*] and replaces it with [Version x.y], having
## gotten x.y from ../include/version.h
##
## Usage is version-replace in-file version.h

SED=sed
if echo $(uname -o) | grep -qi solaris
  then
  SED=gsed
  fi

if ! test -f $1 ; then
  echo "Couldn't find the target file $1"
  exit 1
  fi

VERSION_FILE=$2

if ! test -f $VERSION_FILE ; then
  echo "Couldn't find the version.h file $VERSION_FILE"
  exit 1
  fi

## Extract '2.4' from '#define VERSION "2.4"'
input="$VERSION_FILE"
while read -r line
  do
    IFS=' '; arrIN=($line); unset IFS;
    ## Find the line in question:
    if test X"${arrIN[0]}" == X"#define" && test X"${arrIN[1]}" == X"VERSION" ; then
      # Remove the leading and trailing double-quotes:
      VERSION=$($SED -e 's/^"//' -e 's/"$//' <<<"${arrIN[2]}")
      fi
  done < "$input"

VERSION_STRING='[Version '$VERSION']'

## We need to replace the existing out-of-date version string
## with our new one
echo Updating $1 to $VERSION_STRING
$SED -i -r "s/[[]Version.*[]]/$VERSION_STRING/g" $1

