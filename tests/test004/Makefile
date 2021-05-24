COBCD?=../../cobcd
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

# $(info known-good files: $(knowngood))

.PHONY : known-good test all testprod

all:
	@echo "Your choices are 'make known-good' and 'make test' and 'make testprod'"

known-good:
	COBCDEVEL=1 $(COBCD) -x test.cbl
	CPRINT_RAISE=0 $(GDB) -x test.scr > known-good.txt
	../strip-diff known-good.txt >$(knowngood).stripped

test:
	COBCDEVEL=1 $(COBCD) -x test.cbl
	CPRINT_RAISE=0 $(GDB) -x test.scr > under-test.txt
	../strip-diff under-test.txt >under-test.stripped
	diff -uw under-test.stripped $(knowngood).stripped

testprod:
	cobcd -x test.cbl
	CPRINT_RAISE=0 $(GDB) -x test.scr > under-test.txt
	../strip-diff under-test.txt >under-test.stripped
	diff -uw under-test.stripped $(knowngood).stripped
