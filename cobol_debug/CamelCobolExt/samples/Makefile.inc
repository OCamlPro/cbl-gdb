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

COBC = cobc
# These paths are established for local debugging.  Most likely the cobst and 
# obmod executables will find their way into /usr/local/bin/ for production
# purposes.  You Have Been Warned.
COBST = /home/bob/projects/cobol/cbl-gdb/cobst/cobst
OBMOD = /home/bob/projects/cobol/cbl-gdb/obmod/obmod

all: $(project).x $(project) $(project).s

# This rule creates a cbl-gdb modified executable.	The debugging information
# in the modified executable will point to the project.cbl source code.
#
# Note that it is this srule/script that will have to be copied elsewhere to
# compile .cbl sources for gdb debugging
# (Note: COBST and OBMOD need to be here for development, not production)
$(project) : $(project).cbl $(OBMOD) $(COBST)
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


# This rule creates project.x, which is an unmodified ELF executable created
# with all the same switches as the modified one.  The debugging information
# will thus point to the intermediate .c code
# information pointing to the intermediate .c code
$(project).x : $(project).cbl
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" cobc -c -x -d -g $(free) \
		-T$(project).cbl.lst --tlines=0 --tsymbols $(project).cbl

	$(COBC) -x -o $(project).x $(project).o
	
# cobc -S causes the processing to stop after gcc creates the .s assembly
# language file. As of this writing, I have found no use for it.
$(project).s : $(project).cbl 
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" cobc -S -x -d -g $(free) \
		$(project).cbl

# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c .s *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o .x
clean:
	rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN)

# For regression testing of cobst, this rule establishes a known .ref
# version of .tab
.PHONY : ref
ref:
	cp -f $(project).tab $(project).ref

# As part of regression testing, this rule checks a
# previously.established .ref against the current .tab
.PHONY : check
check:
	@cmp -s $(project).tab $(project).ref; \
		RETVAL=$$?; \
		if [ $$RETVAL -eq 0 ]; then \
			echo "checking: $(project).tab $(project).ref are the same"; \
		else \
			echo "checking: $(project).tab $(project).ref are different"; \
			exit 64; \
		fi




