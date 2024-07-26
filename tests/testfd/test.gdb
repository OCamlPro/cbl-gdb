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
    echo cprint/v6 *\n
    cprint/v6 *
    echo cprint/v6 ?\n
    cprint/v6 ?
    echo cprint/m *\n
    cprint/m *
    echo cprint/m ?\n
    cprint/m ?
    echo KNOWN GOOD FINISH\n
end

file test
cprint/v3
cprint/r 6

break 19

run
printem

quit
