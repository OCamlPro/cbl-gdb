# This is common Makefile code for all the samples/ builds.
#
# Note that the environment variable 'free' needs to be set to either ""
# or "-free" depending on the input format of the .cbl source code 

# disable built-in rules:
.SUFFIXES:

# Find the location of gcc's cc1 compiler, and assembler, and loader:
CC1=$(shell gcc --print-prog-name cc1)
ASSEMBLER=$(shell gcc --print-prog-name as)
LOADER=$(shell gcc --print-prog-name ld)

COBC = cobc
COBST = ../../cobst/cobst
SFIX = ../../sfix/sfix

ifndef STEPS

all: $(project)

$(project) : $(project).cbl
# This rule creates a cbl-gdb modified executable.	The debugging information
# in the modified executable will point to the project.cbl source code.
#
	COBCDEVEL=1 COBCDNOCLEAN=2 COBCDFLAGS=$(COBCDFLAGS) COBCDXM=-x ../../cobcd $< 

# This cleans up the various project files, along with any additional
# $(CLEAN) files specified by the parent Makefile It also clobbers any
# .txt files that might be lying around; I often create .txt files
# from dwarfdump and hexdump.
.PHONY : clean
suffixes = .c .s *.h .i .lst .cbl.lst .full.tab .tab .o .cbl.o .x -2.lst \
			.sym.c .sym.o -2.s -2.o
clean:
	@rm -f $(addprefix $(project),$(suffixes)) $(project) *.txt $(CLEAN) $(project)-gdb.py
    
endif

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

$(project) : $(project).o $(project).sym.o
	@echo " "
	@echo "*****************************"
	@echo "Load from O into executable"
	@echo "*****************************"
	$(COBC) -x -o $(project) $(project).o $(project).sym.o

endif # ifdef $(STEPS)



