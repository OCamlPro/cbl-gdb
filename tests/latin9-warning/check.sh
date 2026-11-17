#!/bin/sh
# A warning quoting a non-UTF-8 byte must not make cobcd fail.
# $1: cobcd command
export LC_ALL=C
rm -f test
out=$($1 -x -Wall test.cbl 2>&1); rc=$?
echo "$out" | grep -a "Exception only" && { echo "FAIL: spurious exception"; exit 1; }
[ $rc -eq 0 ] || { echo "FAIL: rc=$rc"; echo "$out"; exit 1; }
[ -x test ] || { echo "FAIL: no executable"; exit 1; }
echo "$out" | grep -aq "warning:" || { echo "FAIL: no warning seen"; exit 1; }
echo "PASS"
