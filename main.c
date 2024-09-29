#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <copyfile.h>

#include <image1_nor.h>
#include <utils.h>


void usage(const char *program_name) {
    printf("usage: %s VERB <options>\n", program_name);
    printf("where VERB is one of the following:\n");
    printf("\tfix <input NOR> <output NOR> <key 0x836>\n");
    printf("\tmake <raw input> <output Image1> <key 0x836>\n");
    printf("\n");
    printf("only S5L8900X is supported for now\n");
}

typedef enum {
    VERB_FIX,
    VERB_MAKE
} verb_t;

int main(int argc, const char *argv[]) {
    printf("\033[1m" PROJ_NAME ", made by @nyan_satan\033[0m\n");

    if (argc != 5) {
        usage(argv[0]);
        return -1;
    }

    const char *verb_raw = argv[1];
    const char *input = argv[2];
    const char *output = argv[3];
    const char *key_raw = argv[4];

    verb_t verb;
    uint8_t key[AES128_KEY_SIZE];

    if (strcmp(verb_raw, "fix") == 0) {
        verb = VERB_FIX;
    } else if (strcmp(verb_raw, "make") == 0) {
        verb = VERB_MAKE;
    } else {
        usage(argv[0]);
        return -1;
    }

    if (strlen(key_raw) != AES128_KEY_SIZE * 2) {
        printf("ERROR: bad key length, must be %d bytes\n", AES128_KEY_SIZE);
        return -1;
    }

    if (str2hex(sizeof(key), key, key_raw) != sizeof(key)) {
        printf("ERROR: failed to parse key\n");
        return -1;
    }

    if (access(input, R_OK) != 0) {
        printf("ERROR: input file is inaccessible\n");
        return -1;
    }

    if (access(output, F_OK) == 0) {
        printf("ERROR: output file already exists, won't overwrite\n");
        return -1;
    }

    int input_fd = open(input, O_RDONLY);
    if (input_fd < 0) {
        printf("ERROR: failed to open input file\n");
        return -1;
    }

    size_t input_size = lseek(input_fd, 0, SEEK_END);

    off_t offset;
    size_t size;

    if (verb == VERB_FIX) {
        if (input_size != NOR_SIZE) {
            printf("ERROR: bad NOR size, must be %d bytes\n", NOR_SIZE);
            close(input_fd);
            return -1;
        }

        offset = NOR_LLB_OFFSET;
        size = FULL_IMAGE_SIZE;

    } else if (verb == VERB_MAKE) {
        if (input_size > MAX_IMAGE_SIZE || input_size < MIN_IMAGE_SIZE) {
            printf("ERROR: bad image size, must be not bigger than %d bytes and not smaller than %d\n", MAX_IMAGE_SIZE, MIN_IMAGE_SIZE);
            close(input_fd);
            return -1;
        }

        if (input_size % 16) {
            printf("ERROR: bad image size, must be aligned with 16\n");
            close(input_fd);
            return -1;
        }

        offset = 0;
        size = input_size;
    }

    uint8_t buffer[size];

    int read = pread(input_fd, buffer, size, offset);

    close(input_fd);

    if (read != size) {
        printf("ERROR: falied to read input file\n");
        return -1;
    }

    image1_nor_header_t new_header;

    if (verb == VERB_FIX) {
        if (image1_nor_fix_header(buffer, size, key) != 0) {
            printf("ERROR: failed to fix NOR\n");
            return -1;
        }
    } else {
        if (image1_nor_make_header(&new_header, buffer, size, key) != 0) {
            printf("ERROR: failed to fix NOR\n");
            return -1;
        }
    }

    int success = -1;
    int output_fd;

    if (verb == VERB_FIX) {
        if (copyfile(input, output, 0, COPYFILE_ALL) != 0) {
            printf("ERROR: failed to copy input\n");
            return -1;
        }

        output_fd = open(output, O_WRONLY);
        if (output_fd < 0) {
            printf("ERROR: failed to open output for writing\n");
            return -1;
        }

        if (pwrite(output_fd, buffer, size, offset) != size) {
            printf("ERROR: failed to write result\n");
        }

        success = 0;
    
    } else {
        output_fd = open(output, O_WRONLY | O_CREAT, 0644);
        if (output_fd < 0) {
            printf("ERROR: failed to open output for writing\n");
            return -1;
        }

        if (write(output_fd, &new_header, sizeof(new_header)) == sizeof(new_header)) {
            uint8_t emptiness[HEADER_EMPTINESS_SIZE];
            memset(emptiness, 0, HEADER_EMPTINESS_SIZE);

            if (write(output_fd, emptiness, HEADER_EMPTINESS_SIZE) == HEADER_EMPTINESS_SIZE) {
                if (write(output_fd, buffer, size) == size) {
                    success = 0;
                } else {
                    printf("ERROR: failed to write image body\n");
                }
            } else {
                printf("ERROR: failed to write emptiness\n");
            }
        } else {
            printf("ERROR: failed to write header\n");
        }
    }

    close(output_fd);

    if (success == 0) {
        printf("DONE!\n");
    }

    return success;
}
