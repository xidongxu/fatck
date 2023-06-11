// fatdev.c : fat device operate source file
#include "fatdev.h"

fat_dev_t* fat_dev_open(const char* path, int sector_size)
{
    fat_dev_t* device = NULL;
    struct stat file_stat = { 0x00 };

    if ((path == NULL) || (sector_size == 0))
    {
        printf("fatdev args is null.\r\n");
        return NULL;
    }
    // file is exist.
    if (stat(path, &file_stat) < 0)
    {
        printf("file %s is not exist.\r\n", path);
        return NULL;
    }
    if (!(file_stat.st_mode & S_IFREG))
    {
        printf("file %s is not a file.\r\n", path);
        return NULL;
    }
    // create fat device.
    device = (fat_dev_t*)calloc(1, sizeof(fat_dev_t));
    if (device == NULL)
    {
        printf("fatdev build failed.\r\n");
        return NULL;
    }
    // get file size.
    device->file_size = file_stat.st_size;
    // open file.
    device->file_hand = open(path, O_RDONLY | O_BINARY);
    if (device->file_hand < 0)
    {
        printf("file %s open failed.\r\n", path);
        free(device);
        return NULL;
    }
    device->sector_size = sector_size;
    return device;
}

int fat_dev_read(fat_dev_t* device, size_t offset, uint8_t *buff, size_t size)
{
    int result = 0;
    if ((device == NULL) || (device->file_hand < 0) || (buff == NULL) || (size <= 0))
    {
        printf("fat device read failed, parameter is null.\r\n");
        return -1;
    }
    result = lseek(device->file_hand, offset, SEEK_SET);
    if (result < 0)
    {
        printf("fat device read lseek offset %d failed.\r\n", offset);
        return -1;
    }
    result = read(device->file_hand, buff, size);
    if (result != size)
    {
        printf("fat device read failed.\r\n");
        return -1;
    }
    return result;
}

int fat_dev_write(fat_dev_t* device, size_t offset, uint8_t *buff, size_t size)
{
    int result = 0;
    if ((device == NULL) || (device->file_hand < 0) || (buff == NULL) || (size <= 0))
    {
        printf("fat device write failed, parameter is null.\r\n");
        return -1;
    }
    result = lseek(device->file_hand, offset, SEEK_SET);
    if (result < 0)
    {
        printf("fat device write lseek to %d failed.\r\n", offset);
        return -1;
    }
    result = write(device->file_hand, buff, size);
    if (result != size)
    {
        printf("fat device write failed.\r\n");
        return -1;
    }
    return result;
}

int fat_dev_close(fat_dev_t* device)
{
    int result = 0;
    if ((device == NULL) || (device->file_hand < 0))
    {
        printf("fat device close failed, parameter is null.\r\n");
        return -1;
    }
    result = close(device->file_hand);
    return result;
}
