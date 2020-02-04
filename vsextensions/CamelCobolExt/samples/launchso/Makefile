project = elbarco

# disable built-in rules:
.SUFFIXES:

COBC = cobc
COBST = ../../../../cobst/cobst

.PHONY : all clean pregit stubby

all : launchso libboat.so $(project).so

launchso : launchso.o 
	gcc -export-dynamic launchso.o -o launchso -Wl,-rpath,. -ldl `cob-config --libs`

launchso.o : launchso.c
	gcc -c launchso.c -o launchso.o

libboat.so : boat.o
	gcc -shared boat.o -o libboat.so

boat.o:
	gcc -c -fPIC boat.c -o boat.o
	
clean:
	rm -f boat.o libboat.so launchso.o launchso elbarco.so elbarco.tab
	rm -f $(project).c $(project).cbl.lst $(project).cbl.o $(project).c*.h \
	$(project).full.tab $(project).i $(project).lst $(project).o \
	$(project).sym.c $(project).sym.o $(project)-2.lst $(project).-2.o \
	$(project)-2.s $(project)-2.so $(project)-2.s $(project)-2.o \
	$(project).s $(project).sym.so
	
pregit:
#	astyle -n --style=kr *.h
	astyle -n --style=kr *.c

$(project).so : $(project).cbl
	COBCDEVEL=1 COBCDNOCLEAN=2 COBCDFLAGS=$(COBCDFLAGS) COBCDXM=-m ../../../../cobcd $< 

	
