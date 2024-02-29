#include "timing_tests.h"
#include "interrupts.h"

#define TEST(desc, name, ticks) extern uint8_t name();
#include "timing_tests_desc.h"
#undef TEST

TestDesc timing_tests[NUM_TIMING_TESTS] =
{
    #define TEST(desc, name, ticks) { desc, name, ticks },
    #include "timing_tests_desc.h"
    #undef TEST
};

uint8_t run_timing_test(TimingTest test)
{
    disable_interrupts();

    uint8_t ticks = timing_tests[test].func();

    enable_interrupts();

    return ticks;
}

