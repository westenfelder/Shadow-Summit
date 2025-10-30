#define _GNU_SOURCE
#include <sys/mman.h>
#include "rvemu.h"
#include "crack.h"

#include "aes.h"

// --- PKCS#7 UNPADDING HELPER ---
// Removes the PKCS#7 padding added by the Python cryptomodule.
// Returns the final length of the unpadded plaintext.
int pkcs7_unpad(uint8_t *data, int data_len) {
    if (data_len == 0) return 0;

    // The value of the last byte indicates the number of padding bytes
    uint8_t padding_len = data[data_len - 1];

    // Basic validity check: padding must be between 1 and block size (16)
    if (padding_len < 1 || padding_len > 16 || data_len < padding_len) {
        // This indicates corrupted data or wrong padding scheme
        fprintf(stderr, "Error: Invalid PKCS#7 padding found.\n");
        return -1; 
    }
    
    // Check if all padding bytes are correct (optional but robust)
    for (int i = 0; i < padding_len; i++) {
        if (data[data_len - 1 - i] != padding_len) {
            fprintf(stderr, "Error: Padding integrity check failed.\n");
            return -1;
        }
    }

    return data_len - padding_len;
}

int decrypt(unsigned char **data_out) {

    uint8_t *key = (uint8_t *)"my_super_secret_key_1234567890!!"; //todo changeme
    
    // The IV is the first 16 bytes of the array
    uint8_t *iv = crack; 
    uint8_t *ciphertext = crack + 16;
    int ciphertext_len = crack_len - 16;
    
    uint8_t *plaintext = malloc(ciphertext_len);
    if (!plaintext) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }
    memcpy(plaintext, ciphertext, ciphertext_len);

    // initialize context
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);

    // Decrypts the data in the 'plaintext' buffer in-place.
    AES_CBC_decrypt_buffer(&ctx, plaintext, ciphertext_len);
    int plaintext_total_len = pkcs7_unpad(plaintext, ciphertext_len);
    if (plaintext_total_len == -1) {
        free(plaintext);
        return 1;
    }

    *data_out = (unsigned char*) plaintext;
    return plaintext_total_len; // return the len
}

int main(int argc, char *argv[]) {
    machine_t machine = {0};
    machine.cache = new_cache();
    // const unsigned char *data = crack;
    // size_t size = crack_len;
    unsigned char *data = NULL;
    size_t size = decrypt(&data);
    int fd = memfd_create("embed", 0);
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
    free(data);
    return 0;
}
