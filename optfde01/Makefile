project = optfde01

# disable built-in rules:
.SUFFIXES:

#
# This is a sample of a development environment.  The make won't work until
# a cbl-gdb/ "make install" is performed, because prior to that the cobcd
# script won't be available in /usr/local/bin/

COBCD=cobcd

all: $(project) 
# This rule creates a cbl-gdb modified executable.	The debugging information
# in the modified executable will point to the project.cbl source code.
#
# Note that it is this srule/script that will have to be copied elsewhere to
# compile .cbl sources for gdb debugging
$(project) : $(project).cbl 
	$(COBCD) -x $<

# This cleans up the various intermediate project files

.PHONY : clean
suffixes = .c *.h .i .o .so .s 
clean:
	@rm -f $(addprefix $(project),$(suffixes)) $(project) $(project)-gdb.py 

