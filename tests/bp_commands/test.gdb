# testing breakpoints-save and breakpoints-load
# commands to ease the setup within gdb via wrapper commands

# to not output any address we use the address conversion from strip-diff
# activating with SET ADDRESS

# basenames in breakpoint numbers
set filename-display basename

file test

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

echo KNOWN GOOD START\n
echo SET ADDRESS 0x1234\n
info breakpoints
breakpoints-save

delete breakpoints
info breakpoints

breakpoints-load
echo SET ADDRESS 0x1234\n
info breakpoints

# these output an address
break 152
break 155

breakpoints-load
echo SET ADDRESS 0x1234\n
info breakpoints

# just testing - that is normally set outside of GDB...
python
import os
os.environ['project'] = 'TESTME'
end
delete breakpoints

# testing error handling here
breakpoints-load

echo SET ADDRESS 0x1235\n
break 27
breakpoints-save
break 30
breakpoints-save subset
delete breakpoints

breakpoints-load subset
echo SET ADDRESS 0x1235\n
info breakpoints

breakpoints-load
echo SET ADDRESS 0x1235\n
info breakpoints



echo KNOWN GOOD FINISH\n

quit
