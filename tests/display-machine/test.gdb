# testing basic cprint

source ../tests_init.gdb

file ./test

cstart
echo KNOWN GOOD START\n
echo TESTING cprint/m *\n
cprint/m *
echo TESTING cprint/M *\n
cprint/M *

echo \ncprint/m VAR-BINARY- (multiple)\n
cprint/m VAR-BINARY-!
echo \ncprint/M VAR-BINARY- (multiple)\n
cprint/M VAR-BINARY-!

echo \ncprint/m VAR-UNSIGNED-S[HORT]\n
cprint/m VAR-UNSIGNED-S
echo \ncprint/M VAR-UNSIGNED-S[HORT]\n
cprint/M VAR-UNSIGNED-S

echo KNOWN GOOD FINISH\n
continue
quit
