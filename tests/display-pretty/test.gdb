# testing cprint's pretty display

source ../tests_init.gdb

file ./test

cprint/v3

cstart
echo KNOWN GOOD START\n
echo TESTING cprint/v0 *\n
cprint/v0 *
echo TESTING cprint/v1 *\n
cprint/v1 *
echo TESTING cprint/v2 *\n
cprint/v2 *
echo TESTING cprint/v3 *\n
cprint/v3 *
echo TESTING cprint/v4 *\n
cprint/v4 *
echo TESTING cprint/v5 *\n
cprint/v5 *
echo TESTING cprint/v6 *\n
cprint/v6 *
echo TESTING cprint/v7 *\n
cprint/v7 *
echo TESTING cprint/v8 *\n
cprint/v8 *

echo TESTING cprint/p *\n
cprint/p *

echo KNOWN GOOD FINISH\n
continue
quit
