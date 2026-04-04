OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kmain)

PHDRS {
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2));
    rodata  PT_LOAD FLAGS((1 << 2));
    data    PT_LOAD FLAGS((1 << 0) | (1 << 1) | (1 << 2));
}

SECTIONS {
    . = 0xffffffff80000000;

    /* Kernel code */
    .text : ALIGN(0x1000) {
        *(.text .text.*)
    } :text

    /* Read-only data */
    .rodata : ALIGN(0x1000) {
        *(.rodata .rodata.*)
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :rodata

    /* Userspace programs - symbols defined INSIDE */
    .userspace_programs : ALIGN(8) {
        __start_userspace_programs = .;
        KEEP(*(.userspace_programs))
        __stop_userspace_programs = .;
    } :rodata

    /* Userspace RAM section - writable, aligned to 4KB - FOR LATER */
    .userspace_ram : ALIGN(0x1000) {
        __start_userspace_ram = .;
        KEEP(*(.userspace_ram))
        __stop_userspace_ram  = .;
    } :data

    /* Data section */
    .data : ALIGN(0x1000) {
        *(.data .data.*)
    } :data

    /* BSS + kernel stack */
    .bss : ALIGN(0x1000) {
        *(.bss .bss.*)
        *(COMMON)

        /* 64KB kernel stack - enough for deep shell/program call chains since I'm addicted to big programs */
        . = ALIGN(16);
        stack_bottom = .;
        . += 0x10000;
        stack_top = .;
    } :data
}