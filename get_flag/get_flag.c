#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

const char* flag_file = "./flag";

int main(void)
{
    struct passwd* todoapp_flag = getpwnam("todoapp_flag");
    if (todoapp_flag == NULL || getegid() != todoapp_flag->pw_gid)
    {
        perror("what are you doing???");
        exit(-1);
    }

    struct passwd* todoapp = getpwnam("todoapp");
    if (todoapp == NULL || getuid() != todoapp->pw_uid)
    {
        perror("you are not authorized user..");
        exit(-1);
    }

    FILE *fp = fopen(flag_file, "rt");
    char flag[256];
    fgets(flag, sizeof(flag), fp);
    printf("%s", flag);

    exit(0);
}
