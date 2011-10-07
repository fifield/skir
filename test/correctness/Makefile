
##
##
##
LEVEL = ../..
DIRS = c streamit

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common

check-local::
	$(Verb) for dir in $(DIRS); do \
		($(MAKE) -C $$dir $@ ) || exit 1; \
	done
