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

    /* Userspace programs section (top-level, separate) */
    .userspace_programs : ALIGN(8) {
        KEEP(*(.userspace_programs*))
        /* fallback for empty section if no program exists */
        KEEP(*(.userspace_programs_empty))
    } :rodata

    /* Data section */
    .data : ALIGN(0x1000) {
        *(.data .data.*)
    } :data

    /* BSS section */
    .bss : ALIGN(0x1000) {
        *(.bss .bss.*)
        *(COMMON)
    } :data
}

/* Symbols for iterating userspace programs */
__start_userspace_programs = ADDR(.userspace_programs);
__stop_userspace_programs  = ADDR(.userspace_programs) + SIZEOF(.userspace_programs);