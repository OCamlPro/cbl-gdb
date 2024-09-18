# testing ...

source ../tests_init.gdb

file test
cprint/v3

break 10
run

echo KNOWN GOOD START\n
cprint variable1
cprint prog13::variable1
cprint prog12::variable1
cprint prog11::variable1
cprint prog31::variable1
cprint prog32::variable1
cprint prog33::variable1
cprint prog21::variable1
cprint prog23::variable1
cprint prog22::variable1
cprint variable1
echo KNOWN GOOD FINISH\n
continue

quit
