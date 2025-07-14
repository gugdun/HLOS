#include <xencore/arch/x86_64/pic.h>
#include <xencore/arch/x86_64/ports.h>

#include <xencore/io/tty.h>

void remap_pic() {
    uint8_t a1 = inb(0x21); // Save masks
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11); // Start PIC init
    outb(0xA0, 0x11);

    outb(0x21, 0x20); // Master PIC vector offset = 0x20
    outb(0xA1, 0x28); // Slave PIC vector offset = 0x28

    outb(0x21, 0x04); // Tell Master PIC that Slave is at IRQ2 (0000 0100)
    outb(0xA1, 0x02); // Tell Slave PIC its cascade identity

    outb(0x21, 0x01); // 8086 mode
    outb(0xA1, 0x01);

    outb(0x21, a1); // Restore saved masks
    outb(0xA1, a2);

    tty_printf("[PIC] Remapped IRQs\n");
}
