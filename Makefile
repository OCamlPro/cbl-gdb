CXXFLAGS = -std=c++11
export CXXFLAGS

all:
	$(MAKE) -C cobst
	$(MAKE) -C obmod
	$(MAKE) -C sfix
	$(MAKE) -C samples
	$(MAKE) -C picgen
	$(MAKE) -C selfcheck
	$(MAKE) -C optfde01
	$(MAKE) -C vsextensions

.PHONY : clean
clean:
	$(MAKE) -C cobst clean
	$(MAKE) -C obmod clean
	$(MAKE) -C sfix clean
	$(MAKE) -C samples clean
	$(MAKE) -C picgen
	$(MAKE) -C selfcheck clean
	$(MAKE) -C optfde01 clean
	$(MAKE) -C vsextensions clean

.PHONY : install
install:
	$(MAKE) -C cobst install
	$(MAKE) -C obmod install
	$(MAKE) -C sfix install
	$(MAKE) -C python install
	cp cobcd /usr/local/bin/

# Dubner likes to work in Windows, using Visual Studio.  This can result in
# Windows-style CRLF line endings.  He also likes to work with C++ code 
# formatted with Whitesmith style brace formatting.  This rule fixes the 
# potential problems that might arise by running dos2unix on the various files, 
# followed by `astyle --style=kr` for Kernighan & Ritchie brace formatting.
.PHONY : pregit
pregit:
	$(MAKE) -C cobst pregit
	$(MAKE) -C obmod pregit
	$(MAKE) -C sfix pregit
	$(MAKE) -C python pregit
	$(MAKE) -C picgen
	$(MAKE) -C selfcheck pregit
