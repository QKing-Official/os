#ifndef USERSPACE_H
#define USERSPACE_H

typedef void (*userspace_entry)(void);
typedef int  (*userspace_test)(void);

struct userspace_program {
    const char *name;
    userspace_entry main;
    userspace_test test;
} __attribute__((aligned(8)));

#endif