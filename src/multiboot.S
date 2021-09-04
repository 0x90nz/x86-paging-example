# This file contains the multiboot header which enables GRUB or QEMU to detect
# the resulting elf file from compilation as a multiboot executable, and load
# it.
# The header needs to be near the start of the file, so we define it's own
# section 'multiboot' which is the first section output (specified in link.ld)

.equ	MB_MAGIC, 0x1badb002
.equ	MB_FLAGS, 0
.equ	MB_CKSUM, -(MB_MAGIC + MB_FLAGS)

	.section multiboot
	.align 4
.long	MB_MAGIC
.long	MB_FLAGS
.long	MB_CKSUM

