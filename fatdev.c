// fatdev.c : fat device operate source file
#include "fatdev.h"

fatdev_t* fatdev_open(const char* path, int sector_count, int sector_size, int block_size)
{
	fatdev_t* device = NULL;
	struct stat file_stat = { 0x00 };

	if ((path == NULL) || (sector_count == 0) || (sector_size == 0) || (block_size == 0))
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
	// get file size.
	if (!(file_stat.st_mode & S_IFREG))
	{
		printf("file %s is not a file.\r\n", path);
		return NULL;
	}
	device->file_size = file_stat.st_size;
	// create fat device.
	device = (fatdev_t*)calloc(1, sizeof(fatdev_t));
	if (device == NULL)
	{
		printf("fatdev build failed.\r\n");
		return NULL;
	}
	// open file.
	device->file_hand = open(path, O_RDONLY | O_BINARY);
	if (device->file_hand < 0)
	{
		printf("file %s open failed.\r\n", path);
		free(device);
		return NULL;
	}
	device->sector_count = sector_count;
	device->sector_size = sector_size;
	device->block_size = block_size;
	return device;
}

int fatdev_read(fatdev_t* device, uint8_t *buff, size_t size)
{
	int result = 0;
	if ((device == NULL) || (device->file_hand < 0) || (buff == NULL) || (size <= 0))
	{
		printf("fat device read failed, parameter is null.\r\n");
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

int fatdev_write(fatdev_t* device, uint8_t *buff, size_t size)
{
	int result = 0;
	if ((device == NULL) || (device->file_hand < 0) || (buff == NULL) || (size <= 0))
	{
		printf("fat device write failed, parameter is null.\r\n");
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

int fatdev_close(fatdev_t* device)
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
