#include "timing_tests.h"
#include "interrupts.h"

#define TEST(desc, name, page, ticks) extern uint16_t name();
#include "timing_tests_desc.h"
#undef TEST

TestDesc timing_tests[NUM_TIMING_TESTS] =
{
    #define TEST(desc, name, page, ticks) { desc, name, page, ticks },
    #include "timing_tests_desc.h"
    #undef TEST
};

uint16_t run_timing_test(TimingTest test)
{
    disable_interrupts();

    timing_tests[test].func();
    uint16_t ticks = timing_tests[test].func();

    enable_interrupts();

    return ticks;
}

