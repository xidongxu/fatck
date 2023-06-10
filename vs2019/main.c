// main.c : vs2019 project main process

#include <stdio.h>
#include "fatck.h"

static const char* path = "../testcase/system.bin";

int main(void)
{
    int result = 0;
    printf("Hello World!\n");
    result = fatck(path, 4096);
    return result;
}
