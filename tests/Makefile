DIRECTORIES = $(sort $(wildcard */))

.PHONY : known-good test $(DIRECTORIES)

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
