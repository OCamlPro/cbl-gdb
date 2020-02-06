CXXFLAGS = -std=c++11
export CXXFLAGS

# Don't try to build the optfde01 example until the cobcd script
# can be found in /usr/local/bin/
COBCDEXISTS=$(wildcard /usr/local/bin/cobcd)
COBCD=/usr/local/bin/cobcd

all:
	$(MAKE) -C cobst
	$(MAKE) -C sfix
	$(MAKE) -C samples
ifeq ($(COBCDEXISTS), $(COBCD))
	$(MAKE) -C optfde01
endif
	$(MAKE) -C vsextensions

.PHONY : clean
clean:
	$(MAKE) -C cobst clean
	$(MAKE) -C sfix clean
	$(MAKE) -C samples clean
	$(MAKE) -C optfde01 clean
	$(MAKE) -C vsextensions clean

.PHONY : install
install:
	$(MAKE) -C cobst install
	$(MAKE) -C sfix install
	$(MAKE) -C python install
	$(MAKE) -C vsextensions install
	cp cobcd /usr/local/bin/

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
	