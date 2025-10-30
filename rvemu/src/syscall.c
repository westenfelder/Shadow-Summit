#include "rvemu.h"
#include "syscall.h"

#define GET(reg, name) u64 name = machine_get_gp_reg(m, reg);

typedef u64 (* syscall_t)(machine_t *);

static u64 sys_unimplemented(machine_t *m) {
    fatalf("unimplemented syscall: %lu", machine_get_gp_reg(m, a7));
}

static u64 sys_exit(machine_t *m) {
    GET(a0, code);
    exit(code);
}

static u64 sys_close(machine_t *m) {
    GET(a0, fd);
    if (fd > 2) return close(fd);
    return 0;
}

static u64 sys_write(machine_t *m) {
    GET(a0, fd); GET(a1, ptr); GET(a2, len);
    return write(fd, (void *)TO_HOST(ptr), (size_t)len);
}

static u64 sys_fstat(machine_t *m) {
    GET(a0, fd); GET(a1, addr);
    return fstat(fd, (struct stat *)TO_HOST(addr));
}

static u64 sys_gettimeofday(machine_t *m) {
    GET(a0, tv_addr); GET(a1, tz_addr);
    struct timeval *tv = (struct timeval *)TO_HOST(tv_addr);
    struct timezone *tz = NULL;
    if (tz_addr != 0) tz = (struct timezone *)TO_HOST(tz_addr);
    return gettimeofday(tv, tz);
}

static u64 sys_brk(machine_t *m) {
    GET(a0, addr);
    if (addr == 0) addr = m->mmu.alloc;
    assert(addr >= m->mmu.base);
    i64 incr = (i64)addr - m->mmu.alloc;
    mmu_alloc(&m->mmu, incr);
    return addr;
}

// the O_* macros is OS dependent.
// here is a workaround to convert newlib flags to the host.
#define NEWLIB_O_RDONLY   0x0
#define NEWLIB_O_WRONLY   0x1
#define NEWLIB_O_RDWR     0x2
#define NEWLIB_O_APPEND   0x8
#define NEWLIB_O_CREAT  0x200
#define NEWLIB_O_TRUNC  0x400
#define NEWLIB_O_EXCL   0x800
#define REWRITE_FLAG(flag) if (flags & NEWLIB_ ##flag) hostflags |= flag;

static int convert_flags(int flags) {
    int hostflags = 0;
    REWRITE_FLAG(O_RDONLY);
    REWRITE_FLAG(O_WRONLY);
    REWRITE_FLAG(O_RDWR);
    REWRITE_FLAG(O_APPEND);
    REWRITE_FLAG(O_CREAT);
    REWRITE_FLAG(O_TRUNC);
    REWRITE_FLAG(O_EXCL);
    return hostflags;
}

static u64 sys_openat(machine_t *m) {
    GET(a0, dirfd); GET(a1, nameptr); GET(a2, flags); GET(a3, mode);
    return openat(dirfd, (char *)TO_HOST(nameptr), convert_flags(flags), mode);
}

static u64 sys_open(machine_t *m) {
    GET(a0, nameptr); GET(a1, flags); GET(a2, mode);
    u64 ret = open((char *)TO_HOST(nameptr), convert_flags(flags), (mode_t)mode);
    return ret;
}

static u64 sys_lseek(machine_t *m) {
    GET(a0, fd); GET(a1, offset); GET(a2, whence);
    return lseek(fd, offset, whence);
}

static u64 sys_read(machine_t *m) {
    GET(a0, fd); GET(a1, bufptr); GET(a2, count);
    return read(fd, (char *)TO_HOST(bufptr), (size_t)count);
}

static syscall_t syscall_table[] = {
    [SYS_exit] =           sys_exit,
    [SYS_exit_group] =     sys_exit,
    [SYS_read] =           sys_read,
    [SYS_pread] =          sys_unimplemented,
    [SYS_write] =          sys_write,
    [SYS_openat] =         sys_openat,
    [SYS_close] =          sys_close,
    [SYS_fstat] =          sys_fstat,
    [SYS_statx] =          sys_unimplemented,
    [SYS_lseek] =          sys_lseek,
    [SYS_fstatat] =        sys_unimplemented,
    [SYS_linkat] =         sys_unimplemented,
    [SYS_unlinkat] =       sys_unimplemented,
    [SYS_mkdirat] =        sys_unimplemented,
    [SYS_renameat] =       sys_unimplemented,
    [SYS_getcwd] =         sys_unimplemented,
    [SYS_brk] =            sys_brk,
    [SYS_uname] =          sys_unimplemented,
    [SYS_getpid] =         sys_unimplemented,
    [SYS_getuid] =         sys_unimplemented,
    [SYS_geteuid] =        sys_unimplemented,
    [SYS_getgid] =         sys_unimplemented,
    [SYS_getegid] =        sys_unimplemented,
    [SYS_gettid] =         sys_unimplemented,
    [SYS_tgkill] =         sys_unimplemented,
    [SYS_mmap] =           sys_unimplemented,
    [SYS_munmap] =         sys_unimplemented,
    [SYS_mremap] =         sys_unimplemented,
    [SYS_mprotect] =       sys_unimplemented,
    [SYS_rt_sigaction] =   sys_unimplemented,
    [SYS_gettimeofday] =   sys_gettimeofday,
    [SYS_times] =          sys_unimplemented,
    [SYS_writev] =         sys_unimplemented,
    [SYS_faccessat] =      sys_unimplemented,
    [SYS_fcntl] =          sys_unimplemented,
    [SYS_ftruncate] =      sys_unimplemented,
    [SYS_getdents] =       sys_unimplemented,
    [SYS_dup] =            sys_unimplemented,
    [SYS_dup3] =           sys_unimplemented,
    [SYS_rt_sigprocmask] = sys_unimplemented,
    [SYS_clock_gettime] =  sys_unimplemented,
    [SYS_chdir] =          sys_unimplemented,
};

static syscall_t old_syscall_table[] = {
    [-OLD_SYSCALL_THRESHOLD + SYS_open] =   sys_open,
    [-OLD_SYSCALL_THRESHOLD + SYS_link] =   sys_unimplemented,
    [-OLD_SYSCALL_THRESHOLD + SYS_unlink] = sys_unimplemented,
    [-OLD_SYSCALL_THRESHOLD + SYS_mkdir] =  sys_unimplemented,
    [-OLD_SYSCALL_THRESHOLD + SYS_access] = sys_unimplemented,
    [-OLD_SYSCALL_THRESHOLD + SYS_stat] =   sys_unimplemented,
    [-OLD_SYSCALL_THRESHOLD + SYS_lstat] =  sys_unimplemented,
    [-OLD_SYSCALL_THRESHOLD + SYS_time] =   sys_unimplemented,
};

u64 do_syscall(machine_t *m, u64 n) {
    syscall_t f = NULL;
    if (n < ARRAY_SIZE(syscall_table))
        f = syscall_table[n];
    else if (n - OLD_SYSCALL_THRESHOLD < ARRAY_SIZE(old_syscall_table))
        f = old_syscall_table[n - OLD_SYSCALL_THRESHOLD];

    if (!f) fatal("unknown syscall");

    return f(m);
}
