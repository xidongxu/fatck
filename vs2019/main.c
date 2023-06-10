// main.c : vs2019 project main process

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fatck.h"

static const char* file_path = "../test/fatfs.bin";
static uint8_t* file_data = NULL;

int main(void)
{
    int file_read = 0;
    int file_hand = 0;
    int file_size = 0;
    struct stat file_stat = { 0x00 };

    if (file_path == NULL)
    {
        printf("file path is null.\r\n");
        return -1;
    }
    if (stat(file_path, &file_stat) < 0)
    {
        printf("file %s is not exsits.\r\n", file_path);
        return -1;
    }
    if(!(file_stat.st_mode & S_IFREG))
    {
        printf("file %s is not a file.\r\n", file_path);
        return -1;
    }
    // get file size.
    file_size = file_stat.st_size;
    // open file and read data.
    file_hand = open(file_path, O_RDONLY | O_BINARY);
    if (file_hand < 0)
    {
        printf("file %s open failed.\r\n", file_path);
        return -1;
    }
    file_data = malloc(file_size);
    if (file_data == NULL)
    {
        printf("file %s malloc space failed.\r\n", file_path);
        close(file_hand);
        return -1;
    }
    file_read = read(file_hand, file_data, file_size);
    if (file_read != file_size)
    {
        printf("file %s read failed, file_size = %d, read_size = %d\r\n", file_path, file_size, file_read);
        close(file_hand);
        free(file_data);
        return -1;
    }
    printf("Hello World!\n");
    return 0;
}
