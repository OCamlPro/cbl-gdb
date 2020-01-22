project = elbarco

# disable built-in rules:
.SUFFIXES:

COBC = cobc
COBST = ../../../../cobst/cobst
OBMOD = ../../../../obmod/obmod

.PHONY : all clean pregit stubby

all : launchso libboat.so $(project).so

launchso : launchso.o 
	gcc -export-dynamic launchso.o -o launchso -ldl `cob-config --libs`

launchso.o : launchso.c
	gcc -g -c launchso.c -o launchso.o

libboat.so : boat.o
	gcc -shared boat.o -o libboat.so

boat.o:
	gcc -g -c -fPIC boat.c -o boat.o
	
clean:
	rm -f boat.o libboat.so launchso.o launchso elbarco.so elbarco.tab
	rm -f $(project).c $(project).cbl.lst $(project).cbl.o $(project).c*.h \
	$(project).full.tab $(project).i $(project).lst $(project).o \
	$(project).sym.c $(project).sym.o 
	
pregit:
#	astyle -n --style=kr *.h
	astyle -n --style=kr *.c

$(project).so : $(project).cbl
	@# cobc creates the initial relocatable .o ELF file
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" cobc -c -d -g $(free) \
		-T$(project).cbl.lst --tlines=0 --tsymbols $(project).cbl

	@# cobst creates the project.tab file from the cobc output files that are
	@# used by obmod and 
	$(COBST) $(project).cbl
	
	@# obmod creates a modified relocatable .o ELF file from project.tab and
	@# the .o file created by cobc
	$(OBMOD) -i $(project).o -t $(project).tab -o $(project).cbl.o
	
	@# This second invocation of cobc creates the modified ELF library from 
	@# the modified .o ELF relocatable
	$(COBC) -o $(project) $(project).cbl.o
	
	@# Get rid of the intermediate files:
	rm -f $(project).c $(project).cbl.lst $(project).cbl.o $(project).c*.h \
	$(project).full.tab $(project).i $(project).lst $(project).o \
	$(project).sym.c $(project).sym.o 

stubby: 
	COB_CFLAGS="-Wa,-L,-anchls=$(project).lst" cobc -g $(free) \
		-T$(project).cbl.lst --tlines=0 --tsymbols $(project).cbl
	
