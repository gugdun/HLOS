#include <xencore/timer/sleep.h>
#include <xencore/common.h>

extern uint64_t sleep_countdown;

void ksleep(uint64_t ticks)
{
    sleep_countdown = ticks;
    while (sleep_countdown > 0) halt();
}
