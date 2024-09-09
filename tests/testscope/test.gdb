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

echo KNOWN GOOD START\n
cprint *
echo KNOWN GOOD FINISH\n

break 27
break 30
break 34
break 68
break 72
break 75
break 79
break 85
break 91
break 95
break 119
break 130
break 133
break 135
break 141
break 146
break 152
break 155

run
printloop

delete

break 30
run
cp ws-firstname of ws-bob = "bobby"
cp WS-FIRSTNAME in WS-JIM=WS-LASTNAME of WS-JIM
printem
continue

delete
break 121
run

echo KNOWN GOOD START\n
cp LS-LASTNAME/LS-JORB
cp LS-LASTNAME/LS-JORB(3:2) = "xy"
cp LS-LASTNAME/LS-JORB
echo KNOWN GOOD FINISH\n

quit
