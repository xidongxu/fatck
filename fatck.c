// fatck.c : fat file system check tools source file
#include "fatck.h"

#ifndef FATDEV_DPT_ADDRESS
#define FATDEV_DPT_ADDRESS  (0x1BE)
#endif

#define GET_UINT32(x) (uint32_t)(*(x)) | ((uint32_t)*((x) + 1) << 8) | ((uint32_t)*((x) + 2) << 16) | ((uint32_t)*((x) + 3) << 24)

static int fat_root_read(fatdev_t* device)
{
    int result = -1;
    uint8_t dpt_addr = 0;
    // get fat device sector count.
    device->sector_count = device->file_size / device->sector_size;
    // get fat device block size.
    device->sector_buff = (uint8_t*)malloc(device->sector_size * sizeof(uint8_t));
    if (device->sector_buff == NULL)
    {
        printf("fat device sector buffer malloc failed.\r\n");
        return result;
    }
    result = fatdev_read(device, device->sector_buff, device->sector_size);
    if (result != device->sector_size)
    {
        printf("fat root read failed.\r\n");
        return result;
    }
    // get fat device start parttion.
    dpt_addr = device->sector_buff[FATDEV_DPT_ADDRESS];
    if ((dpt_addr != 0x80) && (dpt_addr != 0x00))
    {
        device->sector_start = 0;
    }
    else
    {
        device->sector_start = GET_UINT32(&device->sector_buff[FATDEV_DPT_ADDRESS] + 8);
    }
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
