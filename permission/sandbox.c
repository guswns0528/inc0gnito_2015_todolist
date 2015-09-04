#include "permission.h"

#include <sys/prctl.h>

#include <seccomp.h>

bool sandbox()
{
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);

    if (!ctx)
        return false;

    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 0) != 0)
        return false;
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(fork), 0) != 0)
        return false;
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(prctl), 1, SCMP_A0(SCMP_CMP_EQ, PR_SET_SECCOMP)) != 0)
        return false;
    /*
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(open), 0) != 0)
        return false;
    if (seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(openat), 0) != 0)
        return false;*/

    if (seccomp_load(ctx) != 0)
        return false;

    return true;
}
