# SPDX-License-Identifier: GPL-2.0

# GCOV must not instrument itself in value profile, or kernel cannot be booted.
ifeq ($(CONFIG_PGO_GEN),y)
GCOV_PROFILE := n
endif

ccflags-y := -DSRCTREE='"$(srctree)"' -DOBJTREE='"$(objtree)"'

obj-y := base.o fs.o
obj-$(CONFIG_CC_IS_GCC) += gcc_base.o gcc_4_7.o
CFLAGS_gcc_base.o += -Wno-missing-prototypes -Wno-missing-declarations
obj-$(CONFIG_GCOV_FORMAT_CLANG) += clang.o
CFLAGS_clang.o += -Wno-missing-prototypes -Wno-missing-declarations
