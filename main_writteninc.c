#include <unistd.h>

int my_strcmp(char *a, unsigned char *b) {
    while (*a && (*a == *b)) { ++a; ++b; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

unsigned char hex_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 'a';
}

static void hex_to_bytes(const char *hex, size_t hex_len, unsigned char *out, size_t *out_len)
{
    *out_len = hex_len / 2;
    for (size_t i = 0; i < *out_len; ++i) {
        unsigned char hi = hex_to_nibble(hex[2 * i]);
        unsigned char lo = hex_to_nibble(hex[2 * i + 1]);
        out[i] = (hi << 4) | lo;
    }
}

int main(void)
{
    char input[22] = {0};         

    write(STDOUT_FILENO, "Input Flag: ", 12);

    ssize_t nread = read(STDIN_FILENO, input, sizeof(input) - 1);
    if (nread > 0) {
        if (input[nread - 1] == '\n')
            input[nread - 1] = '\0';
        else
            input[nread] = '\0';
    } else {
        input[0] = '\0';
    }

    const char *hex_flag = "010b06001c350e140c1e4a34040e020904021a";
    const size_t hex_flag_len = 38; 

    unsigned char data[64] = {0};       
    size_t data_len = 0;

    hex_to_bytes(hex_flag, hex_flag_len, data, &data_len);

   
    const unsigned char key = 0x67;
    for (size_t i = 0; i < data_len; ++i)
        data[i] ^= key;

    if (my_strcmp(input, data) == 0) {
        write(STDOUT_FILENO, "Correct!\n", 9);
    } else {
        write(STDOUT_FILENO, "Wrong\n", 6);
    }

    return 0;
}







