#include "hexdump.h"
#include "vga.h"

static inline int isprint(int c)
{
	return c >= 0x20 && c <= 0x7e;
}

static inline int hex_digit(int n)
{
	return "0123456789abcdef"[n & 0x0f];
}

void hexu32(u32 n)
{
	for (int i = 28; i >= 0; i -= 4) {
		vga_putc(hex_digit(n >> i));
	}
}

void hexdump(const void* memory, u32 size)
{
	const u8* buffer = memory;
	for (int i = 0; i < size; i += 16) {
		int rest = size - i > 16 ? 16 : size - i;

		for (int j = 0; j < rest; j++) {
			u8 c = buffer[i + j];
			vga_putc(hex_digit(c >> 4));
			vga_putc(hex_digit(c));
			vga_putc(' ');
			if ((i + j + 1) % 8 == 0)
				vga_putc(' ');
		}
		vga_putc('|');
		for (int j = 0; j < rest; j++) {
			u8 c = buffer[i + j];
			vga_putc(isprint(c) ? c : '.');
		}
		vga_puts("|\n");
	}
}

