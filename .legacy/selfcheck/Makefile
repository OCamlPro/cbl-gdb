project=selfcheck
CPP = g++

src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)
dep = $(obj:.o=.d)	# one dependency file for each source

$(project) : $(obj)
	$(CPP) -o $@ $^

-include $(dep)	  # include all dep files in the makefile

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.cpp
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(project) $(dep) program_under_test program_under_test.cbl \
		program_under_test.tab results.txt results.tab gdb_script.txt

.PHONY: pregit
pregit:
	dos2unix *.h
	dos2unix *.cpp
	dos2unix Makefile
	dos2unix .gitignore
	astyle -n --style=kr --indent=spaces=4 --break-return-type *.h
	astyle -n --style=kr --indent=spaces=4 --break-return-type *.cpp

#.PHONY : install
#install :
#	install $(project) /usr/local/bin/
	