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

    /* Userspace programs - symbols defined INSIDE so they are exact, no padding tricks */
    .userspace_programs : ALIGN(8) {
        __start_userspace_programs = .;
        KEEP(*(.userspace_programs))
        __stop_userspace_programs = .;
    } :rodata

    /* Data section */
    .data : ALIGN(0x1000) {
        *(.data .data.*)
    } :data

    /* BSS + kernel stack */
    .bss : ALIGN(0x1000) {
        *(.bss .bss.*)
        *(COMMON)

        /* 64KB kernel stack - enough for deep shell/program call chains */
        . = ALIGN(16);
        stack_bottom = .;
        . += 0x10000;
        stack_top = .;
    } :data
}