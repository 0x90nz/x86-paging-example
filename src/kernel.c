#include "types.h"
#include "paging.h"

#define VGA_WIDTH	80
#define VGA_HEIGHT	25
#define VGA_BUFFER	((u16*)0xb8000)
#define ROUND_PAGE(x)	(((x - 1) | 4095) + 1)

extern int _ebss;

// the page directory
struct page_directory_entry* pgdir;

// the end of our kernel image, defined in link.ld
static void* next_free_page;

// allocate a /physical/ page. we start directly after our kernel has ended
// (though rounded by one page size), and employ an ostrich-based policy with
// regards the existence of the memory we're allocating
void* allocate_page()
{
	void* addr = next_free_page;
	next_free_page += 4096;
	return addr;
}

void memset(void* dst, u8 value, u32 len)
{
	asm volatile(
		"rep stosb"
		:
		: "a" (value), "D" (dst), "c" (len)
		: "memory"
	);
}

void memcpy(void* dst, const void* src, u32 len)
{
	asm volatile(
		"rep movsl\n\t"         // move as much as we can long-sized
		"movl   %3, %%ecx\n\t"  // get the rest of the length
		"andl   $3, %%ecx\n\t"
		"jz     1f\n\t"         // perfectly long aligned? done if so
		"rep movsb\n\t"
		"1:"
		:
		: "S" (src), "D" (dst), "c" (len / 4), "r" (len)
		: "memory"
	);
}

void vwrite_xy(int x, int y, const char* str)
{
	u16* vbuf = VGA_BUFFER + (y * VGA_WIDTH) + x;
	while (*str) {
		*vbuf++ = *str++ | 0x0200;
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

void setup_paging()
{
	next_free_page = (void*)ROUND_PAGE((u32)&_ebss);

	pgdir = allocate_page();
	memset(pgdir, 0, sizeof(pgdir));

	struct page_table_entry* low_mem_pt = allocate_page();
	memset(low_mem_pt, 0, sizeof(low_mem_pt));

	pgdir[0].present = 1;
	pgdir[0].address = (u32)low_mem_pt >> 12;

	// identity-map the first 4MiB of memory
	for (int i = 0; i < 0x400; i++) {
		low_mem_pt[i].present = 1;
		low_mem_pt[i].frame = i;
	}

	// set the page directory
	asm volatile("mov %%eax, %%cr3\n\t" :: "a" (pgdir));

	// enable paging by setting the PE bit in cr0
	asm volatile(
		"mov %cr0, %eax\n\t"
		"orl $(1 << 31), %eax\n\t"
		"mov %eax, %cr0\n\t"
	);
}

void map_page(void* phys_addr, void* virtual_addr)
{
	u32 pgdir_index = (u32)virtual_addr >> 22;
	u32 pgtbl_index = ((u32)virtual_addr >> 12) & 0x3ff;

	struct page_table_entry* pte;
	if (!pgdir[pgdir_index].present) {
		pte = allocate_page();
		pgdir[pgdir_index].present = 1;
		pgdir[pgdir_index].address = (u32)pte >> 12;
	} else {
		pte = (void*)(pgdir[pgdir_index].address << 12);
	}

	pte[pgtbl_index].present = 1;
	pte[pgtbl_index].frame = (u32)phys_addr >> 12;

	asm volatile("invlpg (%0)" :: "b"(virtual_addr) : "memory");
}

void kernel_entry()
{
	vclear();
	vwrite_xy(0, 0, "Good boot.");
	setup_paging();
	vwrite_xy(0, 1, "Setup paging!");

	hang();
}

