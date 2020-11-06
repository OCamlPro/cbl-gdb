COBCD?=../../cobcd
GDB?=gdb

.PHONY : known-good test all testprod

all:
	@echo "Your choices are 'make known-good' and 'make test' and 'make testprod'"

known-good:
	COBCDEVEL=1 $(COBCD) -o test -x MAIN.cbl
	COBCDEVEL=1 $(COBCD) -m A.cbl
	COBCDEVEL=1 $(COBCD) -m B.cbl
	COBCDEVEL=1 $(COBCD) -m C.cbl
	$(GDB) -x test.scr > known-good.txt
	../strip-diff known-good.txt >known-good.stripped

test:
	COBCDEVEL=1 $(COBCD) -o test -x MAIN.cbl
	COBCDEVEL=1 $(COBCD) -m A.cbl
	COBCDEVEL=1 $(COBCD) -m B.cbl
	COBCDEVEL=1 $(COBCD) -m C.cbl
	$(GDB) -x test.scr > under-test.txt
	../strip-diff under-test.txt >under-test.stripped
	diff -u under-test.stripped known-good.stripped

testprod:
	cobcd -o test -x MAIN.cbl
	cobcd -m A.cbl
	cobcd -m B.cbl
	cobcd -m C.cbl
	$(GDB) -x test.scr > under-test.txt
	../strip-diff under-test.txt >under-test.stripped
	diff -u under-test.stripped known-good.stripped
