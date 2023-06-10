// main.c : vs2019 project main process

#include <stdio.h>
#include "fatck.h"

static const char* file_fail_path = "../test/read_fail.bin";
static const char* file_good_path = "../test/read_good.bin";

int main(void)
{
    int result = 0;
    printf("Hello World!\n");
    result = fatck(file_good_path, 4096);
    return result;
}
