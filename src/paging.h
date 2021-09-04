#pragma once

#include "types.h"

// this entry structure is for 4KiB pages only! 4MiB page entries have different
// structures and so if you want those, you'll have to adjust this
struct page_directory_entry {
	u8 present		: 1;
	u8 rw			: 1;
	u8 user			: 1;
	u8 write_through	: 1;
	u8 cache_disable	: 1;
	u8 accessed		: 1;
	u8 reserved0		: 1;
	// must always be 0 for this structure
	u8 page_size		: 1;
	u8 ignored		: 4;
	u32 address		: 20;
};

struct page_table_entry {
	u8 present		: 1;
	u8 rw			: 1;
	u8 user			: 1;
	u8 write_through	: 1;
	u8 cache_disable	: 1;
	u8 accessed		: 1;
	u8 dirty		: 1;
	u8 reserved0		: 1;
	u8 global		: 1;
	u8 ignored		: 3;
	u32 frame		: 20;
};


