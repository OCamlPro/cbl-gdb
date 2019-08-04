GDB_DATA_DIRECTORY := $(shell echo 'show data-directory'|gdb -q |tr -cd [:print:]|sed -e 's/.*\"\(.*\)\".*/\1/')

.PHONY: all
all :
	@echo There is actually nothing to do for Python code.

.PHONY: pregit
pregit:
	dos2unix cprint.py
	dos2unix Makefile
	dos2unix .gitignore

# cprint.py needs to go into the gdb's data-directory, which can be
# seen with (gdb) show data-directory
.PHONY : install
install :
	echo Install not implemented
#	@mkdir -p $(GDB_DATA_DIRECTORY)/auto-load/
#	install cprint.py $(GDB_DATA_DIRECTORY)/auto-load/
