#include <xencore/fs/test_sample.h>
#include <xencore/io/tty.h>

static uint8_t *sample_base = NULL;
static size_t sample_size = 0;

void analyse_test_sample(struct TestSampleParams *params)
{
    sample_base = (uint8_t *)params->addr;
    sample_size = (size_t)params->size;
    tty_printf(
        "[Test Sample] %u bytes @ 0x%x\n",
        sample_size, (uint64_t)sample_base
    );
    tty_printf(
        "         | 0x00 0x01 0x02 0x03\n  -------+--------------------\n  0x0000 | 0x%x 0x%x 0x%x 0x%x\n",
        sample_base[0], sample_base[1], sample_base[2], sample_base[3]
    );
}
