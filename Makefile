CPP = g++
export CPP
CXXFLAGS:=-std=c++11 -Wall -O3
export CXXFLAGS

DESTDIR_DEFAULT = /usr/local
DESTDIR ?= $(DESTDIR_DEFAULT)

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

.PHONY : test
test:
	$(MAKE) -C cobcd-st
	$(MAKE) -C cobcd-sfix
	$(MAKE) -C cobcd-rw
ifeq ($(COBCDEXISTS), $(COBCD))
	$(MAKE) -C optfde01
endif

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
	@echo "add-auto-load-safe-path $(DESTDIR)/bin" >> $@
	@echo "set directories $(DESTDIR)/bin"         >> $@
	@echo "set auto-load python-scripts on"        >> $@

.PHONY : install
install:
	mkdir -p $(DESTDIR)/bin/
	install cobcd $(DESTDIR)/bin/
	install cobcd.bat $(DESTDIR)/bin/
	$(MAKE) -C cobcd-st install
	$(MAKE) -C cobcd-sfix install
	$(MAKE) -C cobcd-rw install
	$(MAKE) -C python install
	@/bin/echo -e "\033[0;33m"
	@echo "In order for gdb to be able to load the Python script that displays COBOL variables,"
	@echo "you will have to create a ~/.gdbinit file that has the following lines in it:"
	@echo ""
	@echo "      add-auto-load-safe-path $(DESTDIR)/bin"
	@echo "      set directories $(DESTDIR)/bin"
	@echo "      set auto-load python-scripts on"
	@echo ""
	@echo "You may use"
	@if [ X"$(DESTDIR)" = X"$(DESTDIR_DEFAULT)" ]    ; then \
	 echo "      $(MAKE) .gdbinit"                   ; else \
	 echo "      $(MAKE) .gdbinit DESTDIR=$(DESTDIR)"; fi
	@echo "to create a sample file with this content."
	@echo ""
	@echo "See https://sourceware.org/gdb/onlinedocs/gdb/Auto_002dloading-safe-path.html for more information"
	@/bin/echo -e "\033[0m"


.PHONY : uninstall
uninstall:
	rm -f $(DESTDIR)/bin/cobcd
	rm -f $(DESTDIR)/bin/cobcd.bat
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
