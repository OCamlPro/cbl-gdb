#!/bin/sh
# -MD, -MT and -MF must be passed through to cobc.
# $1: cobcd command
export LC_ALL=C
rm -f test test.d
out=$($1 -x -MD -MT test -MF test.d test.cbl 2>&1); rc=$?
[ $rc -eq 0 ] || { echo "FAIL: rc=$rc"; echo "$out"; exit 1; }
[ -f test.d ] || { echo "FAIL: no dependency file"; exit 1; }
echo "PASS"
