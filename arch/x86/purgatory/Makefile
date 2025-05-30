# SPDX-License-Identifier: GPL-2.0
OBJECT_FILES_NON_STANDARD := y

purgatory-y := purgatory.o stack.o setup-x86_$(BITS).o sha256.o entry64.o string.o

targets += $(purgatory-y)
PURGATORY_OBJS = $(addprefix $(obj)/,$(purgatory-y))

LDFLAGS_purgatory.ro := -e purgatory_start -r --no-undefined -nostdlib -z nodefaultlib
targets += purgatory.ro

# Sanitizer, etc. runtimes are unavailable and cannot be linked here.
PGO_PROFILE	:= n
KASAN_SANITIZE	:= n
KCOV_INSTRUMENT := n

# Default KBUILD_CFLAGS can have -pg option set when FTRACE is enabled. That
# in turn leaves some undefined symbols like __fentry__ in purgatory and not
# sure how to relocate those. Like kexec-tools, use custom flags.

KBUILD_CFLAGS := -fno-strict-aliasing -Wall -Wstrict-prototypes -fno-zero-initialized-in-bss -fno-builtin -ffreestanding -c -MD -Os -mcmodel=large
KBUILD_CFLAGS += -m$(BITS)
KBUILD_CFLAGS += $(call cc-option,-fno-PIE)

$(obj)/purgatory.ro: $(PURGATORY_OBJS) FORCE
		$(call if_changed,ld)

targets += kexec-purgatory.c

CMD_BIN2C = $(objtree)/scripts/basic/bin2c
quiet_cmd_bin2c = BIN2C   $@
      cmd_bin2c = $(CMD_BIN2C) kexec_purgatory < $< > $@

$(obj)/kexec-purgatory.c: $(obj)/purgatory.ro FORCE
	$(call if_changed,bin2c)

obj-$(CONFIG_KEXEC_FILE)	+= kexec-purgatory.o
