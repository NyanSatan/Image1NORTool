#ifndef IMAGE1_NOR_H
#define IMAGE1_NOR_H

#include <stdint.h>
#include <unistd.h>

#define AES128_KEY_SIZE 16

#define NOR_SIZE        0x100000
#define NOR_LLB_OFFSET  0x8000

#define HEADER_SIZE     0x800
#define FULL_IMAGE_SIZE 0x20000

#define MAX_IMAGE_SIZE  (FULL_IMAGE_SIZE - HEADER_SIZE)
#define MIN_IMAGE_SIZE  16

#define HEADER_MAGIC    "8900"
#define HEADER_VERSION  "1.0"

enum {
    IMAGE1_NOR_TYPE_ENCRYPTED = 1,
    IMAGE1_NOR_TYPE_UNENCRYPTED = 2
};

typedef struct {
    char magic[4];
    char version[3];
    uint8_t type;
    uint32_t entry_point;
    uint32_t body_size;
    uint8_t data_hash[16];
    uint8_t padding[32];
    uint8_t header_hash[16];
} __attribute__((packed)) image1_nor_header_t;

#define HEADER_EMPTINESS_SIZE   HEADER_SIZE - sizeof(image1_nor_header_t)

int image1_nor_fix_header(
    uint8_t *data, 
    size_t size,
    uint8_t key[16]
);

int image1_nor_make_header(
    image1_nor_header_t *new_header,
    uint8_t *data, 
    size_t size,
    uint8_t key[16]
);

#endif
