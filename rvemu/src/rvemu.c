#define _GNU_SOURCE
#include <sys/mman.h>
#include "rvemu.h"
#include "crack.h"

int main(int argc, char *argv[]) {
    machine_t machine = {0};
    machine.cache = new_cache();
    const unsigned char *data = crack;
    size_t size = crack_len;
    int fd = memfd_create("embedded_program", 0);
    if (fd < 0) {
        perror("memfd_create");
        return 1;
    }

    // Copy embedded binary into memfd
    if (write(fd, data, size) != (ssize_t)size) {
        perror("write");
        close(fd);
        return 1;
    }

    lseek(fd, 0, SEEK_SET);
    // Make a fake "filename" via /proc/self/fd
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    // pass new file as arg to vm
    machine_load_program(&machine, path);
    machine_setup(&machine, argc, argv);
    while (true) {
        enum exit_reason_t reason = machine_step(&machine);
        assert(reason == ecall);
        u64 syscall = machine_get_gp_reg(&machine, a7);
        u64 ret = do_syscall(&machine, syscall);
        machine_set_gp_reg(&machine, a0, ret);
    }
    close(fd);
    return 0;
}
