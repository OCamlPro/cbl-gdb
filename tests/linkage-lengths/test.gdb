# testing ...

source ../tests_init.gdb

file ./test

cprint/v3
cstart


echo KNOWN GOOD START\n

echo TESTING cprint/v3 *\n
cprint/v3 *

echo TESTING cprint/p *\n
cprint/p *

echo KNOWN GOOD FINISH\n

b 29
continue

echo KNOWN GOOD START\n

echo TESTING cprint/v3 *\n
cprint/v3 *

echo TESTING cprint/p *\n
cprint/p *

echo KNOWN GOOD FINISH\n

continue


quit
