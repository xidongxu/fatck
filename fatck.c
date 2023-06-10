// fatck.c : fat file system check tools source file
#include "fatck.h"

static int fat_root_read(fatdev_t* device)
{
    int result = 0;
    // get fat device sector count.
    device->sector_count = device->file_size / device->sector_size;
    // get fat device block size.

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
