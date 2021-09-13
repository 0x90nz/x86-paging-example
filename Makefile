COMMON_FLAGS=-Wall -m32 -march=i386 -nostdlib -nostdinc -ffreestanding -fno-stack-protector
CFLAGS=$(COMMON_FLAGS)
ASFLAGS=$(COMMON_FLAGS)
OBJDIR=build
SRCDIR=src
.PHONY: all build_dir clean run

all: build_dir build/kernel.elf

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.S
	$(CC) $(ASFLAGS) -c -o $@ $<

build_dir:
	@mkdir -p build

build/kernel.elf: build/multiboot.o build/boot.o build/kernel.o build/vga.o build/hexdump.o
	$(CC) $(CFLAGS) $^ -o $@ -T link.ld

run: build_dir build/kernel.elf
	qemu-system-x86_64 -kernel build/kernel.elf

clean:
	rm -rf build
