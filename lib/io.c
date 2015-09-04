#include "io.h"

#include <unistd.h>
#include <string.h>

int read_exact_bytes(void *buffer, int size)
{
    int result = 0;
    while (result < size)
    {
        int t = read_bytes((char*)buffer + result, size - result);
        if (t == -1)
            return result;

        result += t;
    }

    return result;
}

int read_bytes(void *buffer, int size)
{
    char *p = (char *)buffer;
    if (p == NULL)
    {
        return -1;
    }

    int readed = read(STDIN_FILENO, p, size);
    return readed;
}

int write_bytes(const void *buffer, int size)
{
    const char *p = (const char *)buffer;
    if (p == NULL)
    {
        return -1;
    }

    int wrote = write(STDOUT_FILENO, p, size);
    return wrote;
}

int write_string(const char *string)
{
    int length = strlen(string);
    return write_bytes(string, length);
}

int get_int()
{
    int result = -1;
    read_exact_bytes(&result, sizeof(result));

    return result;
}

long get_long()
{
    long result = -1;
    read_exact_bytes(&result, sizeof(result));

    return result;
}
