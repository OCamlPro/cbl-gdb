# This is common Makefile code for all the samples/ builds.

# Note that the environment variable 'free' needs to be set to either ""
# or "-free" depending on the input format of the .cbl source code 

# disable built-in rules:
.SUFFIXES:

all: $(project)

$(project) : $(project).cbl 
	COBCDEVEL=1 COBCDFLAGS=$(COBCDFLAGS) COBCDXM=-x ../../../../cobcd $< 
	
# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c .s *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o .x .sym.c .sym.o
clean:
	rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN)





