#include "types.h"
#include "paging.h"
#include "vga.h"
#include "hexdump.h"
#include "va_arg.h"

#define ROUND_PAGE(x)	(((x - 1) | 4095) + 1)

extern int _ebss;

// the page directory
struct page_directory_entry* pgdir;

// the end of our kernel image, defined in link.ld
static void* next_free_page;

// allocate a /physical/ page. we start directly after our kernel has ended
// (though rounded by one page size), and employ an ostrich-based policy with
// regards the existence of the memory we're allocating
//
// page allocation is a one-way operation. there's no way to deallocate (free)
// a page once it's allocated.
void* allocate_page()
{
	void* addr = next_free_page;
	next_free_page += 4096;
	return addr;
}

// simple implementation of memset. uses x86 string instructions, but has the
// same operation as usual memset.
void memset(void* dst, u8 value, u32 len)
{
	asm volatile(
		"rep stosb"
		:
		: "a" (value), "D" (dst), "c" (len)
		: "memory"
	);
}

// simple implementation of memcpy. similar to `memset`, it uses x86 string
// instructions
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

// stop the processor
__attribute__((noreturn)) static inline void hang()
{
	// this inline asm first disable interrupts. it then halts the processor
	// in theory it should never get past the hlt, but in the case it does,
	// we jump straight back to the hlt.
	asm(
		"cli\n\t"
		"1: hlt\n\t"
		"jmp 1b\n\t"
	);
	// we can't possibly reach this point, so mark it as unreachable so that
	// the compiler doesn't complain
	__builtin_unreachable();
}

void setup_paging()
{
	// setup the first free page, on the first page boundary after the end
	// of the bss section
	next_free_page = (void*)ROUND_PAGE((u32)&_ebss);

	// allocate and zero initialise the page directory, the top level of the
	// paging structure
	pgdir = allocate_page();
	memset(pgdir, 0, sizeof(pgdir));

	// allocate and zero the page table, the second level of the paging
	// structure. this will be used to identity-map the low 4MiB of address
	// space (i.e. each physical address will map exactly to virtual
	// address)
	struct page_table_entry* low_mem_pt = allocate_page();
	memset(low_mem_pt, 0, sizeof(low_mem_pt));

	// set the first page table in the page directory to reference the low
	// memory page table
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
	// a linear address on x86 is composed of 3 parts, laid out like this:
	// | page directory (10bits) | page table (10bits) | offset (12bits)
	u32 pgdir_index = (u32)virtual_addr >> 22;
	u32 pgtbl_index = ((u32)virtual_addr >> 12) & 0x3ff;

	struct page_table_entry* pte;
	// if the page directory doesn't contain a mapping for the page table
	// entry we're concerned with, then we allocate it and set it up. if it
	// does, then we can extract the address of the entry and use that
	if (!pgdir[pgdir_index].present) {
		pte = allocate_page();
		pgdir[pgdir_index].present = 1;
		pgdir[pgdir_index].address = (u32)pte >> 12;
	} else {
		pte = (void*)(pgdir[pgdir_index].address << 12);
	}

	// setup the entry in the page table that we found/allocated
	pte[pgtbl_index].present = 1;
	pte[pgtbl_index].frame = (u32)phys_addr >> 12;

	// finally we need to invalidate the page in the TLB so that no cache
	// weirdness happens
	asm volatile("invlpg (%0)" :: "b"(virtual_addr) : "memory");
}

void print_fmt(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	while (*fmt) {
		char c = *fmt++;
		if (c == '$')
			hexu32(va_arg(args, u32));
		else
			vga_putc(c);
	}

	va_end(args);
}

void kernel_entry()
{
	vga_init();
	vga_puts("* started.\n");
	setup_paging();
	vga_puts("* paging setup\n");

	// get the /physical/ page we want
	void* pg = allocate_page();
	print_fmt("* allocated page at: $\n", pg);

	// create two mappings for that page, each at different virtual
	// addresses
	void* m1 = (void*)0xc0000000;
	void* m2 = (void*)0xc1000000;

	print_fmt("* mapping $ to $\n", pg, m1);
	map_page(pg, m1);
	print_fmt("* mapping $ to $\n", pg, m2);
	map_page(pg, m2);

	// set the first mapping to some arbitary sequence
	print_fmt("* filling $ with 0x41\n", m1);
	memset(m1, 0x41, 4096);

	// finally dump out the contents, so that we can see they are the same.
	print_fmt("\ndump of $:\n", m2);
	hexdump(m2, 128);

	hang();
}

