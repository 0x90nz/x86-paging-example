#include "vga.h"
#include "types.h"

#define VGA_WIDTH	80
#define VGA_HEIGHT	25
#define VGA_BUFFER	((u16*)0xb8000)
#define VGA_COLOR	0x0a00

u8 csr_x, csr_y;

static inline void outb(u16 port, u8 data)
{
	asm volatile("outb %0, %1" :: "a"(data), "Nd"(port));
}

static inline u8 inb(u16 port)
{
	u8 result;
	asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
	return result;
}

void set_cursor(int x, int y)
{
    unsigned short pos = x + y * VGA_WIDTH;

    outb(0x3d4, 0x0f);
    outb(0x3d5, (u8)(pos & 0xff));
    outb(0x3d4, 0x0e);
    outb(0x3d5, (u8)((pos >> 8) & 0xff));
}

void vga_init()
{
	vga_clear();
	// max scanline = 15
	outb(0x3d4, 0x09);
	outb(0x3d5, 0x0f);

	// cursor end line = 15
	outb(0x3d4, 0x0b);
	outb(0x3d5, 0x0f);

	// cursor start line = 14
	outb(0x3d4, 0x0a);
	outb(0x3d5, 0x0e);
}

static inline void set_entry(int x, int y, u16 entry)
{
	VGA_BUFFER[x + (y * VGA_WIDTH)] = entry;
}

void vga_clear()
{
	for (int y = 0; y < VGA_HEIGHT; y++) {
		for (int x = 0; x < VGA_WIDTH; x++) {
			set_entry(x, y, VGA_COLOR);
		}
	}
	csr_x = 0;
	csr_y = 0;
	set_cursor(csr_x, csr_y);
}

static void scroll()
{
	for (int y = 0; y < VGA_HEIGHT - 1; y++) {
		for (int x = 0; x < VGA_WIDTH; x++) {
			VGA_BUFFER[x + (y * VGA_WIDTH)] = VGA_BUFFER[x + ((y + 1) * VGA_WIDTH)];
		}
	}

	for (int x = 0; x < VGA_WIDTH; x++) {
		VGA_BUFFER[x + ((VGA_HEIGHT - 1) * VGA_WIDTH)] = 0;
	}
}

void vga_putc(int c)
{
	if (c == '\n') {
		csr_x = 0;
		csr_y++;
		if (csr_y >= VGA_HEIGHT) {
			csr_y--;
			scroll();
		}
	} else if (c == '\b') {
		if (csr_x == 0) {
			if (csr_y > 0) {
				csr_y--;
				csr_x = 0;
			}
		} else if (csr_x > 0) {
			csr_x--;
		}
	} else if (c == '\r') {
		csr_x = 0;
	} else {
		set_entry(csr_x, csr_y, (c & 0xff) | 0x0200);
		csr_x++;

		if (csr_x >= VGA_WIDTH) {
			vga_putc('\n');
		}
	}

	set_cursor(csr_x, csr_y);
}

void vga_puts(const char* str)
{
	while (*str) {
		vga_putc(*str++);
	}
}


