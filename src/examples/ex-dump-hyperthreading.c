#include <stdio.h>

#include <variorum.h>

int main(int argc, char **argv)
{
    int ret;

    ret = dump_hyperthreading();
    if (ret != 0)
    {
        printf("Dump hyperthreading failed!\n");
    }
    return ret;
}
