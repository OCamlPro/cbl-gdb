# testing ...

source ../tests_init.gdb


file ./test
cprint/v0

break 14
break 19
break 24
break 30
break 35
break 40

cstart
echo KNOWN GOOD START\n
cprint aflag=false
cprint aflag="x"
cprint aflag=true
cprint aflag=false
continue
cprint/v0 AFLAG
continue
cprint/v1 AFLAG
continue
cprint/v2 AFLAG
continue
cprint/v3 BFLAG
continue
cprint/v6 BFLAG
continue
cprint/v8 BFLAG
cprint AFLAG=BFLAG
echo KNOWN GOOD FINISH\n
continue

quit
