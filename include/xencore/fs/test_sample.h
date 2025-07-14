#ifndef _TEST_SAMPLE_H
#define _TEST_SAMPLE_H

#include <stdint.h>

struct TestSampleParams {
    uint64_t addr;
    uint64_t size;
};

void analyse_test_sample(struct TestSampleParams *params);

#endif
