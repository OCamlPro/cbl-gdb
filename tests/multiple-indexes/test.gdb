# testing ...

source ../tests_init.gdb

file ./test
cprint/v3

break 34
run

echo KNOWN GOOD START\n

echo TESTING cprint/v0 *\n
cprint/v0 *

echo TESTING cprint/p *\n
cprint/p *

echo KNOWN GOOD FINISH\n

continue
quit
