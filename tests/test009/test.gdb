# testing ...

source ../tests_init.gdb

define printem
    echo KNOWN GOOD START\n
    echo cprint/v0 *\n
    cprint/v0 *
    echo cprint/v0 ?\n
    cprint/v0 ?
    echo cprint/v3 *\n
    cprint/v3 *
    echo cprint/v3 ?\n
    cprint/v3 ?
    echo cprint/x 1\n
    cprint/x 1
    echo cprint/m *\n
    cprint/m *
    echo cprint/m ?\n
    cprint/m ?
	echo cprint/x 1\n
	cprint/x 1
	echo cprint/b 1\n
	cprint/b 1
    echo KNOWN GOOD FINISH\n
end


file test
cprint/v1
cprint/r 6

break 22
echo KNOWN GOOD START\n
run
cp *
cp/x *
cp/p *
echo KNOWN GOOD FINISH\n

quit
