#!/bin/sh
# Warnings must keep cobc's context lines on separate lines.
# $1: cobcd command
export LC_ALL=C
rm -f test
out=$($1 -x -Wall test.cbl 2>&1 >/dev/null); rc=$?
[ $rc -eq 0 ] || { echo "FAIL: rc=$rc"; echo "$out"; exit 1; }
echo "$out" | grep -aq "warning:" || { echo "FAIL: no warning seen"; exit 1; }
# cobc without context lines (before 3.2) passes trivially
echo "$out" | grep -a "warning:" | grep -aq " [0-9]* [|>] " && { echo "FAIL: context merged into warning"; echo "$out"; exit 1; }
echo "PASS"
