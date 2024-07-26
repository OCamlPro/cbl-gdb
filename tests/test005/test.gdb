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
    echo cprint/m *\n
    cprint/m *
    echo cprint/m ?\n
    cprint/m ?
    echo KNOWN GOOD FINISH\n
end

file test
cprint/v3

break 564
run
printem

break 569
continue
printem

break 588
continue
printem


quit
