#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE 4096

uint8_t buffer[BUFFER_SIZE];
#define BUFFER_INT16 ((int16_t *)buffer)

int main(int argc, char **argv) {
    // Show usage and terminate if exactly two parameters are not provided
    if (argc != 3) {
        fprintf(stderr, "Usage: %s sourcewav destwav\n", argc > 0 ? argv[0] : "remvocals");
        return 1;
    }

    // Open the source file and terminate if unable to
    FILE *sourcewav = fopen(argv[1], "rb");
    if (sourcewav == NULL) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        return 1;
    }

    // Open the destination file and terminate if unable to
    FILE *destwav = fopen(argv[2], "wb");
    if (destwav == NULL) {
        fprintf(stderr, "Failed to open %s\n", argv[2]);
        return 1;
    }

    // Read the first 44 bytes
    if (fread(buffer, 1, 44, sourcewav) != 44) {
        if (ferror(sourcewav)) {
            fprintf(stderr, "Failed to read from %s\n", argv[1]);
        } else {
            fprintf(stderr, "%s is less than 44 bytes long\n", argv[1]);
        }
        return 1;
    }

    // Write the first 44 bytes
    if (fwrite(buffer, 1, 44, destwav) != 44) {
        fprintf(stderr, "Failed to write to %s\n", argv[2]);
        return 1;
    }

    for (size_t size; (size = fread(buffer, 1, BUFFER_SIZE, sourcewav));) {
        // For each pair of 16-bit ints, we subtract the second from the first and divide the result by 2, and then replace both with the result
        for (size_t i = 0; i + 1 < size / 2; i += 2) {
            BUFFER_INT16[i] = BUFFER_INT16[i + 1] = (BUFFER_INT16[i] - BUFFER_INT16[i + 1]) / 2;
        }

        // Write the result to the file
        if (fwrite(buffer, 1, size, destwav) != size) {
            fprintf(stderr, "Failed to write to %s\n", argv[2]);
            return 1;
        }
    }

    // Check if there was an error reading from the source file
    if (ferror(sourcewav)) {
        fprintf(stderr, "Failed to read from %s\n", argv[1]);
        return 1;
    }
}
