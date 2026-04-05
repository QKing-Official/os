#include "../../libraries/libc/include/libc.h"
#include "../userspace.h"
#include "../../libraries/timer.h"

// A test program to test my own libc

static int tests_passed = 0;
static int tests_failed = 0;

static void expect(int cond, const char *name)
{
    if (cond) {
        printf("[ OK ] %s\n", name);
        tests_passed++;
    } else {
        printf("[FAIL] %s\n", name);
        tests_failed++;
    }
}

static void test_memory(void)
{
    char a[16];
    char b[16];

    memset(a, 'A', 5);
    a[5] = 0;

    expect(strlen(a) == 5, "memset + strlen");

    memcpy(b, a, 6);
    expect(strcmp(a, b) == 0, "memcpy");

    b[0] = 'B';
    expect(memcmp(a, b, 1) != 0, "memcmp");
}

static void test_string(void)
{
    char buf[32];

    strcpy(buf, "hello");
    expect(strlen(buf) == 5, "strcpy");

    expect(strcmp("abc", "abc") == 0, "strcmp equal");
    expect(strcmp("abc", "abd") < 0,  "strcmp less");
}

static void test_stdlib(void)
{
    expect(atoi("1234") == 1234, "atoi positive");
    expect(atoi("-42") == -42,   "atoi negative");
}

static void test_ctype(void)
{
    expect(isdigit('5'), "isdigit");
    expect(!isdigit('A'), "isdigit false");

    expect(isalpha('Z'), "isalpha upper");
    expect(isalpha('a'), "isalpha lower");
}

static void test_printf(void)
{
    printf("printf test: %s %d %c\n", "ok", 123, '!');
    expect(1, "printf executed");
}

void test_main(void)
{
    printf("\n=== LIBC TEST SUITE ===\n\n");

    test_memory();
    test_string();
    test_stdlib();
    test_ctype();
    test_printf();

    printf("\n-----------------------\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed == 0)
        printf("RESULT: ALL TESTS PASSED\n");
    else
        printf("RESULT: FAILURES DETECTED\n");

    printf("-----------------------\n");
    timer_delay_s(50);
}

int test_test(void)
{
    /* quick loader check */
    return 1;
}

__attribute__((used, section(".userspace_programs"), aligned(1)))
struct userspace_program test_prog = {
    .name = "test",
    .main = test_main,
    .test = test_test
};