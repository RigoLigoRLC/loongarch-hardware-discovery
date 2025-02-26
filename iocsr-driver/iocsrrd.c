#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <hex_address> [size]\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint32_t address = strtoul(argv[1], NULL, 16);
    uint32_t size = (argc == 3) ? strtoul(argv[2], NULL, 10) : 4;
    if (size != 1 && size != 2 && size != 4 && size != 8) {
        fprintf(stderr, "Invalid size. Must be 1, 2, 4, or 8 bytes.\n");
        return EXIT_FAILURE;
    }

    int fd = open("/dev/iocsr", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open /dev/iocsr");
        return EXIT_FAILURE;
    }

    if (lseek(fd, address, SEEK_SET) == (off_t)-1) {
        perror("lseek failed");
        close(fd);
        return EXIT_FAILURE;
    }

    uint64_t buffer = 0;
    ssize_t bytesRead = read(fd, &buffer, size);
    if (bytesRead != size) {
        perror("Failed to read from /dev/iocsr");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    // Print output in the largest applicable format
    if (size == 1)
        printf("0x%02X\n", (unsigned int)(buffer & 0xFF));
    else if (size == 2)
        printf("0x%04X\n", (unsigned int)(buffer & 0xFFFF));
    else if (size == 4)
        printf("0x%08X\n", (unsigned int)(buffer & 0xFFFFFFFF));
    else if (size == 8)
        printf("0x%016llX\n", (unsigned long long)buffer);

    return EXIT_SUCCESS;
}
