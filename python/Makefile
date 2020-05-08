# Code to see if ~/.gdbinit already exists

project=cobcd.py

AUTO_LOAD_FOUND := $(shell grep -s -e "^[ \t]*set auto-load safe-path /" ~/.gdbinit | wc -l)
AUTO_SCRIPTS_FOUND := $(shell grep -s -e "^[ \t]*set auto-load python-scripts on" ~/.gdbinit | wc -l)

.PHONY: all
all : cobcd.py

cobcd.py : ../include/version.h
## Make sure the VERSION text in cobcd.py is up-to-date
	../bin/version-replace.sh $@ $^

.PHONY: pregit
pregit:
	dos2unix cobcd.py
	dos2unix Makefile
	dos2unix .gitignore

#
# As of this writing, cobcd.py is being embedded into the COBOL executables.
# By default, gdb doesn't put the current working directory onto the list 
# of safe auto-load directories.  There is a default safe directory, but that
# directory isn't on the search path.  So, one way or the other, the user's 
# .gdbinit file has to be modified.	 I have chosen to defeat the default 
# security setting to allow auto-load from anywhere.
#
.PHONY : install
install :
ifdef DESTDIR
	mkdir -p $(DESTDIR)/bin/
	install $(project) $(DESTDIR)/bin/
else
	install $(project) /usr/local/bin/

