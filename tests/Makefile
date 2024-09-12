
all:
	@echo "Your choices are 'make known-good' and 'make test' and 'make testprod'"


# note: this is a direct copy from https://stackoverflow.com/a/33437808/5027456
#       by https://stackoverflow.com/users/4658481/klsdjfhsalkjfhl

UNAME_O := $(shell uname -o)

ifeq "$(UNAME_O)" "Solaris"
	FIND := gfind
else
	FIND := find
endif

DIRS=$(shell ${FIND} . -maxdepth 1 -mindepth 1 -type d -not -name ".*" -printf '%P\n')


# all direct directories of this dir. uses "-printf" to get rid of the "./"

$(DIRS):
	$(MAKE) -C $@
.PHONY: $(DIRS)

# if explicit targets where given: use them in the macro down below. each target will be delivered to each subdirectory contained in $(DIRS).
EXTRA_TARGETS=$(MAKECMDGOALS)

define RECURSIVE_MAKE_WITH_TARGET
# create new variable, with the name of the target as prefix. it holds all
# subdirectories with the target as suffix
$(1)_DIRS=$$(addprefix $(1)_,$$(DIRS))

# create new target with the variable holding all the subdirectories+suffix as
# prerequisite
$(1): $$($1_DIRS)

# use list to create target to fullfill prerequisite. the rule is to call
# recursive make into the subdir with the target
$$($(1)_DIRS):
	$$(MAKE) -C $$(patsubst $(1)_%,%,$$@) $(1)

# and make all targets .PHONY
.PHONY: $$($(1)_DIRS)
endef

# evaluate the macro for all given list of targets
$(foreach t,$(EXTRA_TARGETS),$(eval $(call RECURSIVE_MAKE_WITH_TARGET,$(t))))
