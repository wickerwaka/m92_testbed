#if !defined( TIMING_TESTS_H )
#define TIMING_TESTS_H 1

#include <stdint.h>

typedef enum
{
#define TEST(desc, name, page, ticks) TIMING_TEST_ ##name,
#include "timing_tests_desc.h"
#undef TEST
    NUM_TIMING_TESTS
} TimingTest;

typedef struct
{
    const char *name;
    uint8_t (*func)();
    uint8_t page;
    uint8_t hw_ticks;
} TestDesc;

extern TestDesc timing_tests[NUM_TIMING_TESTS];

uint16_t run_timing_test(TimingTest test);

#endif // TIMING_TESTS_H
