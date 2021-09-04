	.section .text
	.global _start
_start:
	# we setup a stack, as multiboot might not provide this for us. we just
	# place it at the end of conventional memory, as we're very unlikely to
	# need more than ~400KiB of stack space :)
	mov	$0x0007ffff, %esp

	jmp	kernel_entry

	# fallthrough protection for kernel entry point. it /should/ never
	# return, but just in case it does, we hang.
	cli
1:	hlt
	jmp	1b

