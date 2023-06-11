// fatck.c : fat file system check tools source file
#include "fatck.h"

#ifndef FATDEV_DPT_ADDRESS
#define FATDEV_DPT_ADDRESS  (0x1BE)
#endif

#define GET_UINT32(x) (uint32_t)(*(x)) | ((uint32_t)*((x) + 1) << 8) | ((uint32_t)*((x) + 2) << 16) | ((uint32_t)*((x) + 3) << 24)

typedef struct fat_bpb 
{
    // FAT12/16/32 common field (offset from 0 to 35)
    uint8_t  BS_OEMName[9];
    uint16_t BPB_BytsPerSec;
    uint8_t  BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs_16;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t  BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;

    // Fields for FAT12/16 volumes (offset from 36)
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved;
    uint8_t  BS_BootSig;
    uint32_t BS_VolID;
    uint8_t  BS_VolLab[12];
    uint8_t  BS_FilSysType[9];
    uint8_t  BS_BootCode[449];
    uint16_t BS_BootSign;

    // Fields for FAT32 volumes (offset from 36)
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t  BPB_Reserved[12];
} fat_bpb_t;

static int fat_root_read(fatdev_t* device)
{
    int result = -1;
    uint8_t dpt_addr = 0;
    uint8_t* sec_bpb = NULL;
    // get fat device sector count.
    device->sector_count = device->file_size / device->sector_size;
    // get fat device block size.
    device->sector_buff = (uint8_t*)calloc(1, device->sector_size * sizeof(uint8_t));
    if (device->sector_buff == NULL)
    {
        printf("fat device sector buffer malloc failed.\r\n");
        return result;
    }
    result = fatdev_read(device, 0, device->sector_buff, device->sector_size);
    if (result != device->sector_size)
    {
        printf("fat root get dpt address failed.\r\n");
        return result;
    }
    // get fat device start parttion.
    dpt_addr = device->sector_buff[FATDEV_DPT_ADDRESS];
    if ((dpt_addr != 0x80) && (dpt_addr != 0x00))
    {
        device->part_start = 0;
    }
    else
    {
        device->part_start = GET_UINT32(&device->sector_buff[FATDEV_DPT_ADDRESS] + 8);
    }
    result = fatdev_read(device, (device->part_start * device->sector_size), device->sector_buff, device->sector_size);
    if (result != device->sector_size)
    {
        printf("fat root read start parttion failed.\r\n");
        return result;
    }
    // get fat device bpb sector.
    sec_bpb = device->sector_buff;

    return 0;
}

int fatck(const char* path, int sector_size)
{
    int result = -1;
    fatdev_t* device = NULL;
    device = fatdev_open(path, sector_size);
    if (device == NULL)
    {
        return result;
    }
    result = fat_root_read(device);
    if (result < 0)
    {
        printf("fat device root init failed.\r\n");
    }

    fatdev_close(device);
	return result;
}
