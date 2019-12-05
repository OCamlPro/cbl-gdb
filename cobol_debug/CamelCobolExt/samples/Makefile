sample_dirs = $(sort $(dir $(wildcard */)))
sample_dirs_clean = $(addsuffix .clean, $(sample_dirs))
sample_dirs_ref = $(addsuffix .ref,$(sample_dirs))
sample_dirs_check = $(addsuffix .check,$(sample_dirs))

.PHONY: samples $(sample_dirs)
samples: $(sample_dirs)

$(sample_dirs):
	$(MAKE) -C $@
	
.PHONY: clean 
clean: $(sample_dirs_clean)
$(sample_dirs_clean):
	$(MAKE) -C $(subst .clean,,$@) clean
	
.PHONY: ref 
ref: $(sample_dirs_ref)
$(sample_dirs_ref):
	$(MAKE) -C $(subst .ref,,$@) ref
	
.PHONY: check
check: $(sample_dirs_check)
$(sample_dirs_check):
	$(MAKE) -C $(subst .check,,$@) check
	
	

