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

typedef struct _fatdev
{
	int file_hand;
	int file_size;
	int sector_count;
	int sector_size;
	int block_size;
} fatdev_t;

fatdev_t* fatdev_open(const char* path, int sector_size);
int fatdev_read(fatdev_t* device, uint8_t* buff, size_t size);
int fatdev_write(fatdev_t* device, uint8_t* buff, size_t size);
int fatdev_close(fatdev_t* device);

#endif /* __FATDEV_H__ */