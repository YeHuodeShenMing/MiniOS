#include "stdio.h"

int main(int arg, char *argv[])
{

    int pid = fork();
    if (pid != 0)
    {
        printf("\n [this is father's process]: son's pid == %d \n", pid);
    }
    else
    {

        printf("\n [this is son's process] \n");
    }
    return 0;
}