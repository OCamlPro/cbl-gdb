CXXFLAGS = -std=c++11
export CXXFLAGS

all:
	$(MAKE) -C cobst
	$(MAKE) -C obmod
	$(MAKE) -C samples

.PHONY : clean
clean:
	$(MAKE) -C cobst clean
	$(MAKE) -C obmod clean
	$(MAKE) -C samples clean

# Dubner likes to work in Windows, using Visual Studio.  This can result in
# Windows-style CRLF line endings.  He also likes to work with C++ code formatted
# with Whitesmith style brace formatting.  This rule fixes that by running dos2unix
# on the various files, followed by `astyle --style=kr` for Kernighan & Ritchie
# brace formatting.
.PHONY : pregit
pregit:
	$(MAKE) -C cobst pregit
	$(MAKE) -C obmod pregit
