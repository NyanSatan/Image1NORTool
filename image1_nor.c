#include <image1_nor.h>

#include <stdio.h>
#include <string.h>

#include <CommonCrypto/CommonCrypto.h>

static int aes128_decrypt(uint8_t key[16], uint8_t *data_in, size_t data_in_size, uint8_t *data_out, size_t data_out_size) {
    size_t decrypted_bytes = 0;
    CCCryptorStatus decrypt_status = CCCrypt(
                                            kCCDecrypt, kCCAlgorithmAES128, 0,
                                            key, kCCKeySizeAES128,
                                            NULL,
                                            data_in, data_in_size, 
                                            data_out, data_out_size, 
                                            &decrypted_bytes
                                        );

    if (decrypt_status != kCCSuccess || decrypted_bytes != data_in_size) {
        return -1;
    }

    return 0;
}

static int aes128_encrypt(uint8_t key[16], uint8_t *data_in, size_t data_in_size, uint8_t *data_out, size_t data_out_size) {
    size_t encrypted_bytes = 0;
    CCCryptorStatus encrypt_status = CCCrypt(
                                            kCCEncrypt, kCCAlgorithmAES128, 0,
                                            key, kCCKeySizeAES128,
                                            NULL,
                                            data_in, data_in_size, 
                                            data_out, data_out_size, 
                                            &encrypted_bytes
                                        );

    if (encrypt_status != kCCSuccess || encrypted_bytes != data_in_size) {
        return -1;
    }

    return 0;
}

static int sha1_calculate(void *data, size_t size, uint8_t digest[CC_SHA1_DIGEST_LENGTH]) {
    return !(CC_SHA1(data, size, digest) == digest);
}

int image1_nor_fix_header(
    uint8_t *data, 
    size_t size,
    uint8_t key[16]
) {
    image1_nor_header_t *header = (image1_nor_header_t *)data;

    if (memcmp(header->magic, HEADER_MAGIC, 4) != 0) {
        printf("ERROR: bad magic in input, must be \"%s\"\n", HEADER_MAGIC);
        return -1;
    }

    if (memcmp(header->version, HEADER_VERSION, 3) != 0) {
        printf("ERROR: bad version in input, must be \"%s\"\n", HEADER_VERSION);
        return -1;
    }

    if (header->entry_point != 0) {
        printf("ERROR: bad entry point in input, must be 0\n");
        return -1;
    }

    if (header->body_size > MAX_IMAGE_SIZE) {
        printf("ERROR: bad body size in input, must be not bigger than %d\n", MAX_IMAGE_SIZE);
        return -1;
    }

    if (header->body_size % 16) {
        printf("ERROR: bad body size in input, must be aligned with 16\n");
        return -1;
    }

    uint8_t *body = data + HEADER_SIZE;

    if (header->type == IMAGE1_NOR_TYPE_ENCRYPTED) {
        printf(
            "\033[1;33m"
            "this is encrypted Image1, which means it first\n"
            "needs to be processed with Key 0x836 of the\n"
            "device this dump originates from"
            "\033[0m\n"
        );

        if (aes128_decrypt(key, body, header->body_size, body, header->body_size) != 0) {
            printf("ERROR: failed to decrypt image body\n");
            return -1;
        }

        header->type = IMAGE1_NOR_TYPE_UNENCRYPTED;

    } else if (header->type != IMAGE1_NOR_TYPE_UNENCRYPTED) {
        printf("ERROR: bad image type in input\n");
        return -1;
    }

    uint8_t digest[CC_SHA1_DIGEST_LENGTH];

    if (sha1_calculate(body, header->body_size, digest) != 0) {
        printf("ERROR: failed to calculate SHA-1 for image body\n");
        return -1;
    }

    if (aes128_encrypt(key, digest, sizeof(header->data_hash), header->data_hash, sizeof(header->data_hash)) != 0) {
        printf("ERROR: failed to encrypt SHA-1 for image body\n");
        return -1;
    }

    if (sha1_calculate(data, offsetof(image1_nor_header_t, header_hash), digest) != 0) {
        printf("ERROR: failed to calculate SHA-1 for image header\n");
        return -1;
    }

    if (aes128_encrypt(key, digest, sizeof(header->header_hash), header->header_hash, sizeof(header->header_hash)) != 0) {
        printf("ERROR: failed to encrypt SHA-1 for image header\n");
        return -1;
    }

    return 0;
}

int image1_nor_make_header(
    image1_nor_header_t *new_header,
    uint8_t *data, 
    size_t size,
    uint8_t key[16]
) {
    if (size > MAX_IMAGE_SIZE || size < MIN_IMAGE_SIZE) {
        printf("ERROR: bad image size, must be not bigger than %d bytes and not smaller than %d\n", MAX_IMAGE_SIZE, MIN_IMAGE_SIZE);
        return -1;
    }

    if (size % 16) {
        printf("ERROR: bad image size, must be aligned with 16\n");
        return -1;
    }

    memset(new_header, 0, sizeof(*new_header));

    memcpy(new_header->magic, HEADER_MAGIC, 4);

    memcpy(new_header->version, HEADER_VERSION, 3);

    new_header->type = IMAGE1_NOR_TYPE_UNENCRYPTED;

    new_header->entry_point = 0;

    new_header->body_size = size;


    uint8_t digest[CC_SHA1_DIGEST_LENGTH];

    if (sha1_calculate(data, size, digest) != 0) {
        printf("ERROR: failed to calculate SHA-1 for image body\n");
        return -1;
    }

    if (aes128_encrypt(key, digest, sizeof(new_header->data_hash), new_header->data_hash, sizeof(new_header->data_hash)) != 0) {
        printf("ERROR: failed to encrypt SHA-1 for image body\n");
        return -1;
    }

    if (sha1_calculate(new_header, offsetof(image1_nor_header_t, header_hash), digest) != 0) {
        printf("ERROR: failed to calculate SHA-1 for image header\n");
        return -1;
    }

    if (aes128_encrypt(key, digest, sizeof(new_header->header_hash), new_header->header_hash, sizeof(new_header->header_hash)) != 0) {
        printf("ERROR: failed to encrypt SHA-1 for image header\n");
        return -1;
    }

    return 0;
}
