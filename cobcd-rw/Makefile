project=cobcd-rw

prefix ?=/usr/local

src := $(wildcard *.cpp)
obj := $(src:.cpp=.o)
dep := $(obj:.o=.d)  # one dependency file for each source

$(project) : $(obj)
	$(CPP) -o $@ $^
	
-include $(dep)   # include all dep files in the makefile

# generate a dep files using the C preprocessor
%.d: %.cpp
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	@rm -f $(obj) $(project) $(dep)

.PHONY: pregit
pregit:
	astyle -n --style=kr --indent=spaces=4 --break-return-type *.h
	astyle -n --style=kr --indent=spaces=4 --break-return-type *.cpp

.PHONY : install
install :
	mkdir -p $(DESTDIR)$(prefix)/bin/
	install $(project) $(DESTDIR)$(prefix)/bin/

.PHONY : uninstall
uninstall :
	rm -f $(DESTDIR)$(prefix)/bin/$(project)
