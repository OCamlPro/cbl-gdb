project = optfde01
#free = --free
free = 

# disable built-in rules:
.SUFFIXES:

COBC = /usr/local/bin/cobc
COBST = /usr/local/bin/cobst
OBMOD = /usr/local/bin/obmod

all: $(project) 

# This rule creates a cbl-gdb modified executable.	The debugging information
# in the modified executable will point to the project.cbl source code.
#
# Note that it is this srule/script that will have to be copied elsewhere to
# compile .cbl sources for gdb debugging
$(project) : $(project).cbl 
	@# cobc creates the initial relocatable .o ELF file
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" cobc -c -x -d -g $(free) \
		-T$(project).cbl.lst --tlines=0 --tsymbols $(project).cbl

	@# cobst creates the project.tab file from the cobc output files that are
	@# used by obmod and 
	$(COBST) $(project).cbl
	
	@# obmod creates a modified relocatable .o ELF file from project.tab and
	@# the .o file created by cobc
	$(OBMOD) -i $(project).o -t $(project).tab -o $(project).cbl.o
	
	@# This second invocation of cobc creates the modified ELF executable from 
	@# the modified .o ELF relocatable
	$(COBC) -x -o $(project) $(project).cbl.o
	
	@# Get rid of intermediate files, leaving behind the .tab file needed by gdb
	rm $(project).c $(project).o $(project).cbl.o $(project)*.h $(project).i \
		$(project).lst $(project).cbl.lst $(project).full.tab

# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o 
clean:
	rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN)

