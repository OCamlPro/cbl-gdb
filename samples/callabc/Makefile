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
	cobcd -x -g -d MAIN.cbl
	cobcd -m -g -d A.cbl
	cobcd -m -g -d B.cbl
	cobcd -m -g -d C.cbl
#	mv *.so library
	COB_LIBRARY_PATH=./library ./MAIN

.PHONY : clean
clean:
	rm -fr library *.so MAIN
