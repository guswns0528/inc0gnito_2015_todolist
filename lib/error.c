#include <stdio.h>
#include <unistd.h>

#include "error.h"

void exit_on_error(const char* error)
{
    if (error != NULL)
    {
        fputs(error, stderr);
    }

    _exit(-1);
}
