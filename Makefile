CXXFLAGS = -std=c++11
export CXXFLAGS

# Don't try to build the optfde01 example until the cobcd script
# can be found in /usr/local/bin/
COBCDEXISTS=$(wildcard /usr/local/bin/cobcd)
COBCD=/usr/local/bin/cobcd
CWD=$(shell basename `pwd`)

# Yes, this next thing needs to be done in a better way
VERSION=1.1

.PHONY : all
all:
	$(MAKE) -C cobst
	$(MAKE) -C sfix

.PHONY : test
test:
	$(MAKE) -C cobst
	$(MAKE) -C sfix
	$(MAKE) -C samples
ifeq ($(COBCDEXISTS), $(COBCD))
	$(MAKE) -C optfde01
endif

.PHONY : clean
clean:
	$(MAKE) -C cobst clean
	$(MAKE) -C sfix clean
	$(MAKE) -C samples clean
	$(MAKE) -C optfde01 clean

.PHONY : install
install:
ifdef DESTDIR
	mkdir -p $(DESTDIR)/bin/
	install cobcd $(DESTDIR)/bin/
else
	install cobcd /usr/local/bin/
endif
	$(MAKE) -C cobst install
	$(MAKE) -C sfix install
	$(MAKE) -C python install
	$(MAKE) -C vsdistrib install
	
#	copy cbl-gdb to cblgdb-x.y, tarball the copy, and delete the copy
.PHONY : package
package : clean
	cd .. && \
	cp -r $(CWD)/ cblgdb-$(VERSION)/ && \
	tar --exclude=.git --exclude=.legacy -czf cblgdb-$(VERSION).tar.gz cblgdb-$(VERSION) && \
	rm -fr cblgdb-$(VERSION)/

# Dubner likes to work in Windows, using Visual Studio.  This can result in
# Windows-style CRLF line endings.  He also likes to work with C++ code 
# formatted with Whitesmith style brace formatting.  This rule fixes the 
# potential problems that might arise by running dos2unix on the various files, 
# followed by `astyle --style=kr` for Kernighan & Ritchie brace formatting.
.PHONY : pregit
pregit:
	$(MAKE) -C cobst pregit
	$(MAKE) -C sfix pregit
	$(MAKE) -C python pregit
	