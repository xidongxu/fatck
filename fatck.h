// fatck.h : fat file system check tools header file
#ifndef __FATCK_H__
#define __FATCK_H__

#include "fatdev.h"

int fatck(const char* path, int sector_size);

#endif /* __FATCK_H__ */