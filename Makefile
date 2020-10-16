CPP = g++
export CPP
CXXFLAGS:=-std=c++11 -Wall -O3
export CXXFLAGS

prefix_default = /usr/local
prefix ?= $(prefix_default)

# Don't try to build the optfde01 example until the cobcd script
# can be found in /usr/local/bin/
COBCDEXISTS:=$(wildcard /usr/local/bin/cobcd)
COBCD:=/usr/local/bin/cobcd
CWD:=$(shell basename `pwd`)

.PHONY : all
all: cobcd cobcd.bat
## Make sure the VERSION text in cobcd.py is up-to-date
	$(MAKE) -C python
## Then you can make cobst and friends
	$(MAKE) -C cobcd-st
	$(MAKE) -C cobcd-sfix
	$(MAKE) -C cobcd-rw

.PHONY : clean
clean:
	$(MAKE) -C cobcd-st clean
	$(MAKE) -C cobcd-sfix clean
	$(MAKE) -C cobcd-rw clean
	$(MAKE) -C optfde01 clean
	rm -rf .gdbinit

cobcd : include/version.h
	bin/version-replace.sh $@ $^

cobcd.bat : include/version.h
	bin/version-replace.sh $@ $^

.gdbinit:
	@echo "# enable use of COBOL cbl-dbg extension" > $@
	@echo "add-auto-load-safe-path $(prefix)/bin" >> $@
	@echo "set directories $(prefix)/bin"         >> $@
	@echo "set auto-load python-scripts on"        >> $@

.PHONY : install
install:
	mkdir -p $(DESTDIR)$(prefix)/bin/
	install cobcd $(DESTDIR)$(prefix)/bin/
	install cobcd.bat $(DESTDIR)$(prefix)/bin/
	$(MAKE) -C cobcd-st install
	$(MAKE) -C cobcd-sfix install
	$(MAKE) -C cobcd-rw install
	$(MAKE) -C python install
# Output .gdbinit instructions only if DESTDIR is empty.
# When non-empty, this is almost certainly part of a package building exercise,
# and the message is irrelevant and confusing.
	@if [ X"$(DESTDIR)" = X"" ] ; then /bin/echo -e "\033[0;33m" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "In order for gdb to be able to load the Python script that displays COBOL variables," ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "you will have to create a ~/.gdbinit file that has the following lines in it:" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "      add-auto-load-safe-path $(prefix)/bin" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "      set directories $(prefix)/bin" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "      set auto-load python-scripts on" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "You may use" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "" ; fi
	@if [ X"$(DESTDIR)" = X"" ] && [ X"$(prefix)"  = X"$(prefix_default)" ] ; then echo "      $(MAKE) .gdbinit" ; fi
	@if [ X"$(DESTDIR)" = X"" ] && [ X"$(prefix)" != X"$(prefix_default)" ] ; then echo "      $(MAKE) .gdbinit prefix=$(prefix)" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then echo "" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then 	echo "to create a sample file (in this folder) with that content." ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then 	echo "" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then 	echo "See https://sourceware.org/gdb/onlinedocs/gdb/Auto_002dloading-safe-path.html for more information" ; fi
	@if [ X"$(DESTDIR)" = X"" ] ; then 	/bin/echo -e "\033[0m" ; fi

.PHONY : uninstall
uninstall:
	rm -f $(DESTDIR)$(prefix)/bin/cobcd
	rm -f $(DESTDIR)$(prefix)/bin/cobcd.bat
	$(MAKE) -C cobcd-st uninstall
	$(MAKE) -C cobcd-sfix uninstall
	$(MAKE) -C cobcd-rw uninstall
	$(MAKE) -C python uninstall

#	copy cbl-gdb to cblgdb-x.y, tarball the copy, and delete the copy
.PHONY : package
package : clean
	./bin/package.sh

# Dubner likes to work in Windows, using Visual Studio.  This can result in
# Windows-style CRLF line endings.  He also likes to work with C++ code
# formatted with Whitesmith style brace formatting.  This rule fixes the
# potential problems that might arise by running dos2unix on the various files,
# followed by `astyle --style=kr` for Kernighan & Ritchie brace formatting.
.PHONY : pregit
pregit:
	$(MAKE) -C cobcd-st pregit
	$(MAKE) -C cobcd-sfix pregit
	$(MAKE) -C cobcd-rw pregit
	$(MAKE) -C python pregit

#	Provision for regression testing:
#	'make known-good' creates a series of known-good files, while
#	'make test' does a similar run, but compares the under_test to known_good files

.PHONY : known-good

known-good :
	$(MAKE) -C tests known-good 

.PHONY : test
test :
	$(MAKE) -C tests test

