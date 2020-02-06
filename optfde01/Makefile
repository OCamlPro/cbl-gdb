project = optfde01
#free = --free
free = 

# disable built-in rules:
.SUFFIXES:

# In production, you will want to just find cobcd in /usr/local/bin/
# COBCD=cobcd

# This project is in the development environment
COBCD="../cobcd"

all: $(project) 
# This rule creates a cbl-gdb modified executable.	The debugging information
# in the modified executable will point to the project.cbl source code.
#
# Note that it is this srule/script that will have to be copied elsewhere to
# compile .cbl sources for gdb debugging
$(project) : $(project).cbl 
	COBCDXM=-x $(COBCD) $< 

# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o 
clean:
	@rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN)

