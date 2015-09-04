#include "permission.h"

#define _GNU_SOURCE
#include <pwd.h>
#include <stddef.h>
#include <unistd.h>

#define user "bulletin"

bool drop_privileges()
{
    struct passwd *passwd = getpwnam(user);
    if (passwd == NULL)
    {
        return false;
    }
    else if (setresuid(passwd->pw_uid, passwd->pw_uid, passwd->pw_uid) == -1)
    {
        return false;
    }
    else if (setresgid(passwd->pw_gid, passwd->pw_gid, passwd->pw_gid) == -1)
    {
        return false;
    }

    return true;
}
