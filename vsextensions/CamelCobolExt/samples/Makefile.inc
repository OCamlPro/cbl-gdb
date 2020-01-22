# This is common Makefile code for all the samples/ builds.
#
# 1) It uses GnuCOBOL cobc to generate debug-enabled .o, .lst,
# .cbl.lst, .c, and .i files.
#
# 2) It then runs cobst, which reads some of those files, to generate
# the symbol table .tab file
#
# 3) It then runs obmod, which reads the .tab and the .o file, and
# generates .cbl.o.	 That file has the original back-to-the-c
# debugging information stripped out and replaced with the
# back-to-the-cbl debugging information
#
# 4) From .o and .cbl.o it creates the project and project.x
# executables, which may or may not be useful, depending on whether or
# not the original .cbl module was designed to create a standalone
# executable

# Note that the environment variable 'free' needs to be set to either ""
# or "-free" depending on the input format of the .cbl source code 

# disable built-in rules:
.SUFFIXES:

all: $(project)

$(project) : $(project).cbl 
	COBCDEVEL=1 COBCDFLAGS=$(COBCDFLAGS) ../../../../cobcd $< 
	
# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c .s *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o .x .sym.c .sym.o
clean:
	rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN)





