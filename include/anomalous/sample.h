#ifndef _AM_SAMPLE_H
#define _AM_SAMPLE_H

#include <efi.h>

#include <xencore/fs/test_sample.h>

EFI_STATUS load_sample(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, struct TestSampleParams *params);

#endif
