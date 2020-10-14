COBCD?=../../cobcd
GDB?=gdb

.PHONY : all buildkg

all :
	echo "Hold your horses"
	exit 1

known-good:
	COBCDEVEL=1 $(COBCD) -x test.cbl
	$(GDB) -x test.scr ## > test.known-good



