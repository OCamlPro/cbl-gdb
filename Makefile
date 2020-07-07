CXXFLAGS:=-std=c++11 -Wall
export CXXFLAGS

DESTDIR ?=/usr/local

# Don't try to build the optfde01 example until the cobcd script
# can be found in /usr/local/bin/
COBCDEXISTS:=$(wildcard /usr/local/bin/cobcd)
COBCD:=/usr/local/bin/cobcd
CWD:=$(shell basename `pwd`)

.PHONY : all
all: cobcd cobcd.bat
## Make sure the VERSION text in cobcd.py is up-to-date
	$(MAKE) -C python
## Then you can make cobst
	$(MAKE) -C cobcd-st
	$(MAKE) -C cobcd-sfix

.PHONY : test
test:
	$(MAKE) -C cobcd-st
	$(MAKE) -C cobcd-sfix
ifeq ($(COBCDEXISTS), $(COBCD))
	$(MAKE) -C optfde01
endif

.PHONY : clean
clean:
	$(MAKE) -C cobcd-st clean
	$(MAKE) -C cobcd-sfix clean
	$(MAKE) -C optfde01 clean

cobcd : include/version.h
	bin/version-replace.sh $@ $^

cobcd.bat : include/version.h
	bin/version-replace.sh $@ $^

.PHONY : install
install:
	mkdir -p $(DESTDIR)/bin/
	install cobcd $(DESTDIR)/bin/
	install cobcd.bat $(DESTDIR)/bin/
	$(MAKE) -C cobcd-st install
	$(MAKE) -C cobcd-sfix install
	$(MAKE) -C python install

.PHONY : uninstall
uninstall:
	rm -f $(DESTDIR)/bin/cobcd
	rm -f $(DESTDIR)/bin/cobcd.bat
	$(MAKE) -C cobcd-st uninstall
	$(MAKE) -C cobcd-sfix uninstall
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
	$(MAKE) -C python pregit
