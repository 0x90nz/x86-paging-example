#include "types.h"

#define VGA_WIDTH	80
#define VGA_HEIGHT	25
#define VGA_BUFFER	((u16*)0xb8000)

void vwrite_xy(int x, int y, const char* str)
{
	u16* vbuf = VGA_BUFFER + (y * VGA_WIDTH) + x;
	while (*str) {
		*vbuf++ = *str++ | 0x0f00;
	}
}

void vclear()
{
	u16* vbuf = VGA_BUFFER;
	for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
		*vbuf++ = 0;
	}
}

__attribute__((noreturn)) static inline void hang()
{
	asm("1: cli; hlt; jmp 1b");
	__builtin_unreachable();
}

void kernel_entry()
{
	vclear();
	vwrite_xy(0, 0, "Hello World!");
	hang();
}

