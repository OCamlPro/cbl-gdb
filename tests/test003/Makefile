PYTHON?=/usr/bin/python3
COBCD?=$(PYTHON) ../../cobcd
GDB?=gdb

# For reasons that remain unknown -- probably because of the older
# version of GDB, but who knows? -- the response on RedHat7 isn't quite
# the same as for Ubuntu.

# So, we cook up, and read, a different set of known-good files for RH7

# start with "known-good"
knowngood:=known-good

# tack on "-rh7" on a RedHat7 system:
#knowngood := $(knowngood)$(shell uname -r |grep el7| sed 's/\(^.*\)el7\(.*\)/-rh7/g')

# We also need separate known-good files for MINGW32 and MINGW64
#knowngood := $(knowngood)$(shell uname -s | grep MINGW | sed 's/^\(MINGW..\).*/-\1/g')

.PHONY : known-good test all testprod

all:
	@echo "Your choices are 'make known-good' and 'make test' and 'make testprod'"

#
# When invoking GDB, initialize CPRINT_V=3.  By default, when cobcd.py is loaded, it initializes
# the v-mode to zero.  We don't want it that way, and when running in Windows there is no
# way to override it from inside the GDB test.scr script.

known-good:
	COBCDEVEL=1 $(COBCD) -o test -x MAIN.cbl
	COBCDEVEL=1 $(COBCD) -m A.cbl
	COBCDEVEL=1 $(COBCD) -m B.cbl
	COBCDEVEL=1 $(COBCD) -m C.cbl
	CPRINT_RAISE=0 CPRINT_V=3 COBCDRW=$(COBCDRW) $(GDB) --batch -x test.scr > known-good.txt
	$(PYTHON) ../strip-diff known-good.txt >$(knowngood).stripped

test:
	COBCDEVEL=1 $(COBCD) -o test -x MAIN.cbl
	COBCDEVEL=1 $(COBCD) -m A.cbl
	COBCDEVEL=1 $(COBCD) -m B.cbl
	COBCDEVEL=1 $(COBCD) -m C.cbl
	CPRINT_RAISE=0 CPRINT_V=3 COBCDRW=$(COBCDRW) $(GDB) --batch -x test.scr > under-test.txt
	$(PYTHON) ../strip-diff under-test.txt >under-test.stripped
	diff -uw $(knowngood).stripped under-test.stripped

testprod:
	cobcd -o test -x MAIN.cbl
	cobcd -m A.cbl
	cobcd -m B.cbl
	cobcd -m C.cbl
	CPRINT_RAISE=0 CPRINT_V=3 $(GDB) --batch -x test.scr > under-test.txt
	$(PYTHON) ../strip-diff under-test.txt >under-test.stripped
	diff -uw $(knowngood).stripped under-test.stripped
