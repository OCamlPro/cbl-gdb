
.PHONY : all package clean

#
# The package is dependent on a zillion files, so a true dependency tree is
# not really feasible.  Meanwhile, running vsce package causes the package
# contents to change, because they put a timestamp in there.  So, to avoid
# having git go spastic thinking things have changed when they actually haven't,
# this makefile leans in the direction of not doing things until a knowlegeable
# developer tells it to do things.
#
# So, the basic process requires operator intervention.  To update the 
# distribution package, you need to
#
# make clean && make && make install
#
# Otherwise, make all won't do anything if it sees an existing package.
# and make install won't do anything until it sees a package here that is newer
# than the one in vsdistrib.

node_modules=$(wildcard node_modules)
package=$(wildcard *.vsix)
nopackage=

all:
ifeq ($(node_modules),$(nopackage))
	npm install
endif

ifeq ($(package),$(nopackage))
	vsce package
endif


clean:
	rm -f *.vsix

install: ../../../vsdistrib/$(package)

../../../vsdistrib/$(package) : $(package)
	cp $< $@


