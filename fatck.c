// fatck.c : fat file system check tools source file
#include "fatck.h"

#define GET32(x) (uint32_t)(*(x)) | ((uint32_t)*((x) + 1) << 8) | ((uint32_t)*((x) + 2) << 16) | ((uint32_t)*((x) + 3) << 24)

static int fat_root_read(fatdev_t* device)
{
    int result = -1;
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
    if ((device->sector_buff[0x1BE] != 0x80) && (device->sector_buff[0x1BE] != 0x00))
    {
        device->part_begain = 0;
    }
    else
    {
        device->part_begain = GET32(&device->sector_buff[0x1BE] + 8);
        printf("========== part begain = %d =========\r\n", device->part_begain);
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
