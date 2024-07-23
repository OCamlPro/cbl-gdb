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
    echo cprint/x #1\n
    cprint/x #1
    echo cprint/m *\n
    cprint/m *
    echo cprint/m ?\n
    cprint/m ?
	echo cprint/x #1\n
	cprint/x #1
	echo cprint/b #1\n
	cprint/b #1
    echo KNOWN GOOD FINISH\n
end


file test
cprint/v3
cprint/r 6

break 129
break 139
break 149
break 159
break 170
break 177
break 187
break 197
break 206
break 216
break 226
break 230

run
printloop

delete
break 230
run

echo KNOWN GOOD START\n
echo cprint com\n
cprint com
echo cprint com!\n
cprint com!
echo cprint *com\n
cprint *com
echo cprint com*\n
cprint com*
echo cprint *com*\n
cprint *com*
echo cprint def-b*\n
cprint def-b*
echo cprint *x\n
cprint *x

cprint/v0
echo cprint com\n
cprint com
echo cprint com!\n
cprint com!
echo cprint *com\n
cprint *com
echo cprint com*\n
cprint com*
echo cprint *com*\n
cprint *com*
echo cprint def-b*\n
cprint def-b*
echo cprint *x\n
cprint *x

echo TESTING ASSIGNMENT:\n

cprint header                     = "10.31e1"
cprint withpic                    = "10.31e1"
cprint def-BINARY-CHAR            = "10.31e1"
cprint def-BINARY-CHAR            =  10.31e1 
cprint def-BINARY-CHAR-SIGNED     = "10.31e1"
cprint def-BINARY-CHAR-UNSIGNED   = "10.31e1"
cprint def-BINARY-C-LONG          = "10.31e1"
cprint def-BINARY-C-LONG-SIGNED   = "10.31e1"
cprint def-BINARY-C-LONG-UNSIGNED = "10.31e1"
cprint def-BINARY-DOUBLE          = "10.31e1"
cprint def-BINARY-DOUBLE-SIGNED   = "10.31e1"
cprint def-BINARY-DOUBLE-UNSIGNED = "10.31e1"
cprint def-BINARY-INT             = "10.31e1"
cprint def-BINARY-LONG            = "10.31e1"
cprint def-BINARY-LONG-SIGNED     = "10.31e1"
cprint def-BINARY-LONG-UNSIGNED   = "10.31e1"
cprint def-BINARY-LONG-LONG       = "10.31e1"
cprint def-BINARY-SHORT           = "10.31e1"
cprint def-BINARY-SHORT-SIGNED    = "10.31e1"
cprint def-BINARY-SHORT-UNSIGNED  = "10.31e1"
cprint def-COMPUTATIONAL          = "10.31e1"
cprint def-COMP-1                 = "10.31e1"
cprint def-COMP-2                 = "10.31e1"
cprint def-COMPUTATIONAL-1        = "10.31e1"
cprint def-COMPUTATIONAL-2        = "10.31e1"
cprint def-FLOAT-DECIMAL-16       = "10.31e1"
cprint def-FLOAT-DECIMAL-34       = "10.31e1"
cprint def-FLOAT-LONG             = "10.31e1"
cprint def-FLOAT-SHORT            = "10.31e1"
cprint def-INDEX                  = "10.31e1"
cprint def-SIGNED-INT             = "10.31e1"
cprint def-SIGNED-LONG            = "10.31e1"
cprint def-SIGNED-SHORT           = "10.31e1"
cprint def-UNSIGNED-INT           = "10.31e1"
cprint def-UNSIGNED-LONG          = "10.31e1"
cprint def-UNSIGNED-SHORT         = "10.31e1"
cprint def-COMP-3                 = "10.31e1"
cprint def-COMPUTATIONAL-3        = "10.31e1"
cprint def-PACKED-DECIMAL         = "10.31e1"
cprint def-PACKED-DECIMAL-S       = "10.31e1"
cprint def-BINARY                 = "10.31e1"
cprint def-BINARY-S               = "10.31e1"
cprint def-COMP                   = "10.31e1"
cprint def-COMPUTATIONAL          = "10.31e1"
cprint def-COMP-4                 = "10.31e1"
cprint def-COMPUTATIONAL-4        = "10.31e1"
cprint def-COMP-5                 = "10.31e1"
cprint def-COMPUTATIONAL-5        = "10.31e1"
cprint def-COMP-6                 = "10.31e1"
cprint def-COMP-X                 = "10.31e1"
cprint def-COMPUTATIONAL-X        = "10.31e1"
cprint def-COMP-XX                = "10.31e1"
cprint def-DISPLAY                = "10.31e1"

cp *

echo KNOWN GOOD FINISH\n

echo Checking the debugger CPRINT LENGTH OF feature\n
delete
break 134
break 135
break 136
break 137
break 138
break 139
break 140
break 141
break 142
break 143
break 144
break 145
break 146
break 147
break 148
break 149
break 150
break 151
break 152
break 153
break 154
break 155
break 156
break 157
break 158
break 159
break 160
break 161
break 162
break 163
break 164
break 165
break 166
break 167
break 168
break 170

echo KNOWN GOOD START\n
run
cprint LENGTH OF def-BINARY-CHAR
continue
cprint LENGTH OF def-BINARY-CHAR-SIGNED
continue
cprint LENGTH OF def-BINARY-CHAR-UNSIGNED
continue
cprint LENGTH OF def-BINARY-C-LONG
continue
cprint LENGTH OF def-BINARY-C-LONG-SIGNED
continue
cprint LENGTH OF def-BINARY-C-LONG-UNSIGNED
continue
cprint LENGTH OF def-BINARY-DOUBLE
continue
cprint LENGTH OF def-BINARY-DOUBLE-SIGNED
continue
cprint LENGTH OF def-BINARY-DOUBLE-UNSIGNED
continue
cprint LENGTH OF def-BINARY-INT
continue
cprint LENGTH OF def-BINARY-LONG
continue
cprint LENGTH OF def-BINARY-LONG-SIGNED
continue
cprint LENGTH OF def-BINARY-LONG-UNSIGNED
continue
cprint LENGTH OF def-BINARY-LONG-LONG
continue
cprint LENGTH OF def-BINARY-SHORT
continue
cprint LENGTH OF def-BINARY-SHORT-SIGNED
continue
cprint LENGTH OF def-BINARY-SHORT-UNSIGNED
continue
cprint LENGTH OF def-COMPUTATIONAL
continue
cprint LENGTH OF def-COMP-1
continue
cprint LENGTH OF def-COMP-2
continue
cprint LENGTH OF def-COMPUTATIONAL-1
continue
cprint LENGTH OF def-COMPUTATIONAL-2
continue
cprint LENGTH OF def-FLOAT-DECIMAL-16
continue
cprint LENGTH OF def-FLOAT-DECIMAL-34
continue
cprint LENGTH OF def-FLOAT-LONG
continue
cprint LENGTH OF def-FLOAT-SHORT
continue
cprint LENGTH OF def-INDEX
continue
cprint LENGTH OF def-POINTER
continue
cprint LENGTH OF def-PROCEDURE-POINTER
continue
cprint LENGTH OF def-PROGRAM-POINTER
continue
cprint LENGTH OF def-SIGNED-INT
continue
cprint LENGTH OF def-SIGNED-LONG
continue
cprint LENGTH OF def-SIGNED-SHORT
continue
cprint LENGTH OF def-UNSIGNED-INT
continue
cprint LENGTH OF def-UNSIGNED-LONG
continue
cprint LENGTH OF def-UNSIGNED-SHORT
echo KNOWN GOOD FINISH\n

continue

quit
