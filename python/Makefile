# Code to see if ~/.gdbinit already exists

AUTO_LOAD_FOUND := $(shell grep -s -e "^[ \t]*set auto-load safe-path /" ~/.gdbinit | wc -l)
AUTO_SCRIPTS_FOUND := $(shell grep -s -e "^[ \t]*set auto-load python-scripts on" ~/.gdbinit | wc -l)

.PHONY: all
all :
## Make sure the VERSION text in cprint.py is up-to-date
	./version

.PHONY: pregit
pregit:
	dos2unix cprint.py
	dos2unix Makefile
	dos2unix .gitignore

#
# As of this writing, cprint.py is being embedded into the COBOL executables.
# By default, gdb doesn't put the current working directory onto the list 
# of safe auto-load directories.  There is a default safe directory, but that
# directory isn't on the search path.  So, one way or the other, the user's 
# .gdbinit file has to be modified.	 I have chosen to defeat the default 
# security setting to allow auto-load from anywhere.
#
.PHONY : install
install :
	mkdir -p /usr/local/bin/
	cp cprint.py /usr/local/bin/
ifeq ($(AUTO_LOAD_FOUND), 0)
	echo "set auto-load safe-path /" >>~/.gdbinit
endif
ifeq ($(AUTO_SCRIPTS_FOUND), 0)
	echo "set auto-load python-scripts on" >>~/.gdbinit
endif

