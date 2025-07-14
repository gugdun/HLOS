#include <xencore/arch/x86_64/pit.h>
#include <xencore/arch/x86_64/ports.h>

#include <xencore/io/tty.h>

#define PIT_FREQUENCY 1193182
#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40

void setup_pit(uint32_t hz) {
    if (hz == 0 || hz > PIT_FREQUENCY) return;

    uint16_t divisor = (uint16_t)(PIT_FREQUENCY / hz);

    outb(PIT_COMMAND_PORT, 0x36); // Channel 0, LSB/MSB, mode 3 (square wave)

    outb(PIT_CHANNEL0_PORT, divisor & 0xFF);        // LSB
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xFF); // MSB

    outb(0x21, inb(0x21) & ~1); // Unmask IRQ0 (timer)

    tty_printf("[PIT] Set frequency to %u Hz\n", hz);
}
