CC      = x86_64-linux-gnu-gcc
LD      = x86_64-linux-gnu-ld
CFLAGS  = -m64 -ffreestanding -fno-stack-protector \
          -mno-red-zone -nostdlib -Ikernel -Ilibraries \
          -mgeneral-regs-only -O2 -mcmodel=large -fno-pic -fno-pie
LDFLAGS = -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000

SRC_DIRS := kernel libraries userspace
SRCS := $(shell find $(SRC_DIRS) -type f -name '*.c')
OBJS := $(SRCS:.c=.o)

.PHONY: all clean run

all: OS.iso

kernel/kernel.elf: $(OBJS) kernel/linker.ld
	$(LD) $(LDFLAGS) -T kernel/linker.ld $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

OS.iso: kernel/kernel.elf
	rm -rf iso_root
	mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	cp kernel/kernel.elf          iso_root/boot/kernel.elf
	cp limine.conf                iso_root/boot/limine/limine.conf
	cp limine.conf                iso_root/EFI/BOOT/limine.conf
	cp limine/limine-bios.sys     iso_root/boot/limine/
	cp limine/limine-bios-cd.bin  iso_root/boot/limine/
	cp limine/limine-uefi-cd.bin  iso_root/boot/limine/
	cp limine/BOOTX64.EFI         iso_root/EFI/BOOT/ 2>/dev/null || true
	cp limine/BOOTIA32.EFI        iso_root/EFI/BOOT/ 2>/dev/null || true
	xorriso -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		-o OS.iso iso_root
	./limine/limine bios-install OS.iso

run: OS.iso
	# no gpu
	# qemu-system-x86_64 -cdrom OS.iso -boot d -m 256M -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0
	# gpu
	qemu-system-x86_64 -cdrom OS.iso -boot d -m 256M   -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0   -vga virtio -global virtio-gpu-pci.vgamem_mb=256
clean:
	rm -rf iso_root $(OBJS) kernel/kernel.elf OS.iso