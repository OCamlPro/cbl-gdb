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

# Find the location of gcc's cc1 compiler, and assembler, and loader:
CC1=$(shell gcc --print-prog-name cc1)
ASSEMBLER=$(shell gcc --print-prog-name as)
LOADER=$(shell gcc --print-prog-name ld)

COBC = cobc
# These paths are established for local debugging.	Most likely the cobst and 
# obmod executables will find their way into /usr/local/bin/ for production
# purposes.	 You Have Been Warned.
COBST = ../../cobst/cobst
OBMOD = ../../obmod/obmod
SFIX = ../../sfix/sfix

ifndef STEPS

all: $(project)

$(project) : $(project).cbl
# This rule creates a cbl-gdb modified executable.	The debugging information
# in the modified executable will point to the project.cbl source code.
#
#	COBCDEVEL=1 COBCDNOCLEAN=2 ../../cobcd $< 
	COBCDEVEL=1 COBCDFLAGS=$(COBCDFLAGS) ../../cobcd $< 

# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c .s *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o .x -2.lst \
			.sym.c .sym.o -2.s -2.o
clean:
	@rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN)

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
endif  ## ifndef $(STEPS)

ifdef STEPS

all : $(project)

$(project).c $(project).lst : $(project).cbl
	@echo " "
	@echo "*****************************"
	@echo "Compile from COBOL -> C"
	@echo "*****************************"
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" $(COBC) -c -x -d -g $(free) \
		--fgen-c-line-directives --fgen-c-labels \
		-T$(project).cbl.lst --tlines=0 --tsymbols $(project).cbl

$(project).sym.o : $(project).lst
	@echo " "
	@echo "*****************************"
	@echo "Generate symbols cross-reference"
	@echo "*****************************"
	$(COBST) $(project).cbl
	gcc -c -o $(project).sym.o $(project).sym.c 

$(project).s : $(project).c
	@echo " "
	@echo "*****************************"
	@echo "Compile from C -> S"
	@echo "*****************************"

	$(CC1) -quiet -imultiarch x86_64-linux-gnu \
	-D PIC $(project).c -quiet -dumpbase $(project).c -mtune=generic \
	-march=x86-64 -auxbase-strip $(project).o -g -fPIC \
	-fstack-protector-strong -Wformat -Wformat-security -o $(project).s
	$(SFIX) $(project).s $(project).s $(project).c $(project).cbl


$(project).o : $(project).s
	@echo " "
	@echo "*****************************"
	@echo "Assemble from S -> O"
	@echo "*****************************"

	$(ASSEMBLER) --64 -L -anchls=$(project)-2.lst -o $(project).o $(project).s

#$(project).cbl.o : $(project).tab $(project).o
#	@echo " "
#	@echo "*****************************"
#	@echo "Do Dubner's modifications to O"
#	@echo "*****************************"
#	$(OBMOD) -x -i $(project).o -t $(project).tab -o $(project).cbl.o

$(project) : $(project).o $(project).sym.o
	@echo " "
	@echo "*****************************"
	@echo "Load from O into executable"
	@echo "*****************************"
	$(COBC) -x -o $(project) $(project).o $(project).sym.o



endif # ifdef $(STEPS)

.PHONY : steps
steps :
	@echo " "
	@echo "*****************************"
	@echo "Compile from COBOL -> C"
	@echo "*****************************"
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" $(COBC) -c -x -d -g $(free) \
		-T$(project).cbl.lst --tlines=0 --tsymbols $(project).cbl

	@echo " "
	@echo "*****************************"
	@echo "Generate symbols cross-reference"
	@echo "*****************************"
	$(COBST) $(project).cbl

	@echo " "
	@echo "*****************************"
	@echo "Compile from C -> S"
	@echo "*****************************"

	$(CC1) -quiet -imultiarch x86_64-linux-gnu \
	-D PIC $(project).c -quiet -dumpbase $(project).c -mtune=generic \
	-march=x86-64 -auxbase-strip $(project).o -g -fPIC \
	-fstack-protector-strong -Wformat -Wformat-security -o $(project).s
	$(SFIX) $(project).s $(project).s $(project).c $(project).cbl

	@echo " "
	@echo "*****************************"
	@echo "Assemble from S -> O"
	@echo "*****************************"

	$(ASSEMBLER) --64 -L -anchls=$(project).lst -o $(project).o $(project).s

	@echo " "
	@echo "*****************************"
	@echo "Do Dubner's modifications to O"
	@echo "*****************************"

	$(OBMOD) -i $(project).o -t $(project).tab -o $(project).cbl.o

	@echo " "
	@echo "*****************************"
	@echo "Load from O into executable"
	@echo "*****************************"

	$(COBC) -x -o $(project) $(project).cbl.o


