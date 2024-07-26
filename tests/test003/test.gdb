# testing ...

source ../tests_init.gdb

define printem
    echo KNOWN GOOD START\n
    echo cprint/v3 *\n
    cprint/v3 *
    echo cprint/v3 ?\n
    cprint/v3 ?
    echo cprint/v0 *\n
    cprint/v0 *
    echo cprint/v0 ?\n
    cprint/v0 ?
    echo cprint/x #1\n
    cprint/x #1
    echo cprint/m *\n
    cprint/m *
    echo cprint/m ?\n
    cprint/m ?
    echo KNOWN GOOD FINISH\nend
end

file test
cprint/v3
cprint/r 6

### Just run the whole program clean, to establish a baseline
### (We don't want to think we have a debugging problem when
###  in fact it was the test program that changed.)
echo KNOWN GOOD START\n
run
echo KNOWN GOOD FINISH\n

## We want to check the 'frame N' handling

echo KNOWN GOOD START\n
echo \nSet breakpoint at INDENTER.cpy\n
echo Run from beginning\n
echo KNOWN GOOD FINISH\n
break INDENTER.cpy:2
run

echo KNOWN GOOD START\n
echo \nThese should be the C.cbl variables\n
echo Run from beginning\n
echo KNOWN GOOD FINISH\n
printem

cup
echo KNOWN GOOD START\n
echo \nThese should be the B.cbl variables after CUP\n
echo Run from beginning\n
echo KNOWN GOOD FINISH\n
printem

cup
echo KNOWN GOOD START\n
echo \nThese should be the A.cbl variables after CUP\n
echo Run from beginning\n
echo KNOWN GOOD FINISH\n
printem

cdown
echo KNOWN GOOD START\n
echo \nThese should be the B.cbl variables after CDOWN\n
echo Run from beginning\n
echo KNOWN GOOD FINISH\n
printem

cdown
echo KNOWN GOOD START\n
echo \nThese should be the C.cbl variables after CDOWN\n
echo Run from beginning\n
echo KNOWN GOOD FINISH\n
printem

continue

### Let's check the reloading of PICTURE_STRING when switching from module
### to module

echo Testing module switching...\n
delete
break MAIN.cbl:10
break MAIN.cbl:11
break MAIN.cbl:12
break MAIN.cbl:13
break A.cbl:18
break A.cbl:19
break A.cbl:20
break B.cbl:18
break B.cbl:19
break C.cbl:16
echo KNOWN GOOD START\n

run
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *
continue
cprint *

echo KNOWN GOOD FINISH\n

quit
