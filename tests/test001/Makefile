COBCD?=../../cobcd
GDB?=gdb

.PHONY : known-good test

known-good:
	COBCDEVEL=1 $(COBCD) -x test.cbl
	$(GDB) -x test.scr > test.known-good

test:
	COBCDEVEL=1 $(COBCD) -x test.cbl
	$(GDB) -x test.scr > test.under-test
	../strip-diff test.known-good >known-good
	../strip-diff test.under-test >under-test
	diff -u under-test known-good
