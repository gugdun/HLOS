#ifndef _TEST_SAMPLE_H
#define _TEST_SAMPLE_H

#include <stdint.h>

#define TAR_BLOCK_SIZE       512
#define TAR_REGULAR_FILE     '0'
#define TAR_HARD_LINK        '1'
#define TAR_SYMBOLIC_LINK    '2'
#define TAR_CHARACTER_DEVICE '3'
#define TAR_BLOCK_DEVICE     '4'
#define TAR_DIRECTORY        '5'
#define TAR_FIFO             '6'
#define TAR_CONTIGUOUS_FILE  '7'

struct TarHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

struct TestSampleParams {
    uint64_t addr;
    uint64_t size;
};

void analyse_test_sample(struct TestSampleParams *params);

#endif
