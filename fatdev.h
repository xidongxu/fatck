// fatdev.h : fat device operate source file
#ifndef __FATDEV_H__
#define __FATDEV_H__

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct fat_dev
{
    int file_hand;
    uint32_t file_size;
    uint32_t sector_count;
    uint32_t sector_size;
    uint32_t part_start;
    uint32_t block_size;
} fat_dev_t;

fat_dev_t* fat_dev_open(const char* path, int sector_size);
int fat_dev_read(fat_dev_t* device, size_t offset, uint8_t* buff, size_t size);
int fat_dev_write(fat_dev_t* device, size_t offset, uint8_t* buff, size_t size);
int fat_dev_close(fat_dev_t* device);

#endif /* __FATDEV_H__ */