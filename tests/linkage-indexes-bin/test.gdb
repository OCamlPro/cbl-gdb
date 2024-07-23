# testing handling of binary indexes

source ../tests_init.gdb

file ./test

cprint/v3
cstart

break 34
break 63
continue

echo KNOWN GOOD START\n

echo TESTING cprint/v0 *\n
cprint/v0 *

echo TESTING cprint/p *\n
cprint/p *

cprint LS-SHM-AMNT=5

echo TESTING cprint/v0 *\n
cprint/v0 *

echo TESTING cprint/p *\n
cprint/p *

cprint LS-SHM-AMNT=10

cprint/v0 LS-SHM-VAL1(1)
cprint/v0 LS-SHM-VAL1(2)
cprint/v0 LS-SHM-VAL1(3)
cprint/v0 LS-SHM-VAL1(4)
cprint/v0 LS-SHM-VAL1(5)
cprint/v0 LS-SHM-VAL1(6)
cprint/v0 LS-SHM-VAL1(7)
cprint/v0 LS-SHM-VAL1(8)
cprint/v0 LS-SHM-VAL1(9)

echo KNOWN GOOD FINISH\n

continue

echo KNOWN GOOD START\n

echo TESTING cprint/v0 *\n
cprint/v0 *

echo TESTING cprint/p *\n
cprint/p *

cprint SHM-AMNT=5

echo TESTING cprint/v0 *\n
cprint/v0 *

echo TESTING cprint/p *\n
cprint/p *

cprint SHM-AMNT=10

echo KNOWN GOOD FINISH\n

quit
