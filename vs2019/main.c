// main.c : vs2019 project main process

#include <stdio.h>
#include "fatck.h"

static const char* file_path = "../test/fatfs.bin";

int main(void)
{
    int result = 0;
    fatdev_t* device = NULL;

    printf("Hello World!\n");
    device = fatdev_open(file_path, 0, 4096, 4096);
    if (device != NULL)
    {
        result = fatck(device);
    }
    return result;
}
