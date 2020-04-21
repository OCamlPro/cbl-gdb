# disable built-in rules:
.SUFFIXES:

#
# This is a sample of a development environment.  The make won't work until
# a cbl-gdb/ "make install" is performed, because prior to that the cobcd
# script won't be available in /usr/local/bin/

COBCD=../../cobcd

.PHONY : all
all:
#	mkdir -p library
	$(COBCD) -x -g -d MAIN.cbl
	$(COBCD) -m -g -d A.cbl
	$(COBCD) -m -g -d B.cbl
	$(COBCD) -m -g -d C.cbl
#	mv *.so library
	COB_LIBRARY_PATH=./library ./MAIN

.PHONY : clean
clean:
	rm -fr library *.so *.dll MAIN MAIN.exe *-gdb.py
