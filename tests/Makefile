DIRECTORIES = $(sort $(wildcard */))

.PHONY : known-good test $(DIRECTORIES) all

all :
	@echo "Your choices are 'make known-good' and 'make test'"

known-good : $(DIRECTORIES)
	@for dir in $(DIRECTORIES) ; \
	do \
	$(MAKE) -C $$dir known-good ; \
	done

test : $(DIRECTORIES)
	@for dir in $(DIRECTORIES) ; \
	do \
	$(MAKE) -C $$dir test ; \
	done

testprod : $(DIRECTORIES)
	@for dir in $(DIRECTORIES) ; \
	do \
	$(MAKE) -C $$dir testprod ; \
	done
