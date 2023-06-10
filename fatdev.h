// fatdev.h : fat device operate source file
#ifndef __FATDEV_H__
#define __FATDEV_H__

#include <stdint.h>

void* fat_dev_open(const char* name);
int fat_dev_read(void* dev, uint8_t* buff, size_t size);
int fat_dev_write(void* dev, uint8_t* buff, size_t size);
int fat_dev_close(void* dev);
int fat_dev_ctrl(void* dev, int cmd, void* args);

#endif /* __FATDEV_H__ */