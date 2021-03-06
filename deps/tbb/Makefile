# Copyright 2005-2010 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks.
#
# Threading Building Blocks is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# Threading Building Blocks is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Threading Building Blocks; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As a special exception, you may use this file as part of a free software
# library without restriction.  Specifically, if other files instantiate
# templates or use macros or inline functions from this file, or you compile
# this file and link it with other files to produce an executable, this
# file does not by itself cause the resulting executable to be covered by
# the GNU General Public License.  This exception does not however
# invalidate any other reasons why the executable file might be covered by
# the GNU General Public License.
LEVEL=../..
include $(LEVEL)/Makefile.common

tbb_root?=$(PROJ_SRC_DIR)
include $(tbb_root)/build/common.inc
.PHONY: default all tbb tbbmalloc test examples

#workaround for non-depend targets tbb and tbbmalloc which both depend on version_string.tmp
#According to documentation submakes should run in parallel
.NOTPARALLEL: tbb tbbmalloc

default: tbb tbbmalloc tbb_debug tbbmalloc_debug

#all: tbb tbbmalloc test examples

all:: default

tbb_debug: mkdir
	$(Echo) Compiling TBB Debug build
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbb cfg=debug tbb_root=$(tbb_root)
	$(Verb) $(MKDIR) $(LibDir)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_debug/libtbb_debug.so $(LibDir)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_debug/libtbb_debug.so.2 $(LibDir)

tbb: mkdir
	$(Echo) Compiling TBB Release build
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbb cfg=release tbb_root=$(tbb_root)
	$(Verb) $(MKDIR) $(LibDir)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_release/libtbb.so $(LibDir)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_release/libtbb.so.2 $(LibDir)

tbbmalloc_debug: mkdir
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc tbb_root=$(tbb_root)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_debug/libtbbmalloc_debug.so $(LibDir)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_debug/libtbbmalloc_debug.so.2 $(LibDir)

tbbmalloc: mkdir
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc tbb_root=$(tbb_root)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_release/libtbbmalloc.so $(LibDir)
	$(Verb) cp $(tbb_build_dir)/$(tbb_build_prefix)_release/libtbbmalloc.so.2 $(LibDir)

test: tbb tbbmalloc
	-$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=debug malloc_test tbb_root=$(tbb_root)
	-$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.test cfg=debug tbb_root=$(tbb_root)
	-$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.tbbmalloc cfg=release malloc_test tbb_root=$(tbb_root)
	-$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.test cfg=release tbb_root=$(tbb_root)

rml: mkdir
	$(MAKE) -C "$(work_dir)_debug"  -r -f $(tbb_root)/build/Makefile.rml cfg=debug tbb_root=$(tbb_root)
	$(MAKE) -C "$(work_dir)_release"  -r -f $(tbb_root)/build/Makefile.rml cfg=release tbb_root=$(tbb_root)


examples: tbb tbbmalloc
	$(MAKE) -C examples -r -f Makefile tbb_root=.. release test

.PHONY: clean clean_examples mkdir info

clean:: clean_examples
#	echo $(PROJ_SRC_DIR)
#	echo $(LibDir)
	$(shell $(RM) $(work_dir)_release$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_release >$(NUL) 2>$(NUL))
	$(shell $(RM) $(work_dir)_debug$(SLASH)*.* >$(NUL) 2>$(NUL))
	$(shell $(RD) $(work_dir)_debug >$(NUL) 2>$(NUL))
	$(Verb) rm -f $(tbb_build_dir)/release
	$(Verb) rm -f $(tbb_build_dir)/debug
#	@echo clean done

clean_examples:
	$(shell $(MAKE) -s -i -r -C examples -f Makefile tbb_root=.. clean >$(NUL) 2>$(NUL))

mkdir:
	$(shell $(MD) "$(work_dir)_release" >$(NUL) 2>$(NUL))
	$(shell $(MD) "$(work_dir)_debug" >$(NUL) 2>$(NUL))
#$(Echo) Created $(work_dir)_release and ..._debug directories

info:
	@echo OS: $(tbb_os)
	@echo arch=$(arch)
	@echo compiler=$(compiler)
	@echo runtime=$(runtime)
	@echo tbb_build_prefix=$(tbb_build_prefix)

