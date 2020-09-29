project=cobcd-st

PREFIX ?=/usr/local

src := $(wildcard *.cpp)
obj := $(src:.cpp=.o)
dep := $(obj:.o=.d)	# one dependency file for each source

$(project) : $(obj)
	$(CPP) -o $@ $^

-include $(dep)	  # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.cpp
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@
	
cobst.cpp : cobcd_py.h
	
cobcd_py.h : ../python/cobcd.py
	xxd -i ../python/cobcd.py >cobcd_py.h

.PHONY: clean
clean:
	@rm -f $(obj) $(project) $(dep) cobcd_py.h

.PHONY: pregit
pregit:
	dos2unix *.h
	dos2unix *.cpp
	dos2unix Makefile
	dos2unix .gitignore
	astyle -n --style=kr --indent=spaces=4 --break-return-type *.h
	astyle -n --style=kr --indent=spaces=4 --break-return-type *.cpp

.PHONY : install
install :
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	install $(project) $(DESTDIR)$(PREFIX)/bin/

.PHONY : uninstall
uninstall :
	rm -f $(DESTDIR)$(PREFIX)/bin/$(project)
