#include "base64.h"
#include <stdint.h>

static char s_base64_enc[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};
static char s_base64_url[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
};
static const uint8_t s_base64_dec[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 62, 0, 63,  /* +/-/ */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, /* 0 - 9 */
        0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  /* A - Z */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 63, /* _ */
        00, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* a - z */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0,
};

static size_t base64_encode_table(char *target, const void *source, size_t bytes, const char *table) {
    size_t i, j;
    const uint8_t *ptr = (const uint8_t *) source;

    for (j = i = 0; i < bytes / 3 * 3; i += 3) {
        target[j++] = table[(ptr[i] >> 2) & 0x3F]; /* c1 */
        target[j++] = table[((ptr[i] & 0x03) << 4) | ((ptr[i + 1] >> 4) & 0x0F)]; /*c2*/
        target[j++] = table[((ptr[i + 1] & 0x0F) << 2) | ((ptr[i + 2] >> 6) & 0x03)];/*c3*/
        target[j++] = table[ptr[i + 2] & 0x3F]; /* c4 */
    }

    if (i < bytes) {
        /* There were only 2 bytes in that last group */
        target[j++] = table[(ptr[i] >> 2) & 0x3F];

        if (i + 1 < bytes) {
            target[j++] = table[((ptr[i] & 0x03) << 4) | ((ptr[i + 1] >> 4) & 0x0F)]; /*c2*/
            target[j++] = table[((ptr[i + 1] & 0x0F) << 2)]; /*c3*/
        } else {
            /* There was only 1 byte in that last group */
            target[j++] = table[((ptr[i] & 0x03) << 4)]; /*c2*/
            target[j++] = '='; /*c3*/
        }

        target[j++] = '='; /*c4*/
    }

    return j;
}

size_t base64_encode(char *target, const void *source, size_t bytes) {
    return base64_encode_table(target, source, bytes, s_base64_enc);
}

size_t base64_encode_url(char *target, const void *source, size_t bytes) {
    return base64_encode_table(target, source, bytes, s_base64_url);
}

size_t base64_decode(void *target, const char *src, size_t bytes) {
    size_t i, j;
    uint8_t *p = (uint8_t *) target;
    const uint8_t *source = (const uint8_t *) src;
    const uint8_t *end;

//    assert(0 == bytes % 4);

    i = 0;
    end = source + bytes;
    for (j = 1; j < bytes / 4; j++) {
        p[i++] = (s_base64_dec[source[0]] << 2) | (s_base64_dec[source[1]] >> 4);
        p[i++] = (s_base64_dec[source[1]] << 4) | (s_base64_dec[source[2]] >> 2);
        p[i++] = (s_base64_dec[source[2]] << 6) | s_base64_dec[source[3]];
        source += 4;
    }

    if (source + 4 == end) {
        p[i++] = (s_base64_dec[source[0]] << 2) | (s_base64_dec[source[1]] >> 4);
        if ('=' != source[2]) p[i++] = (s_base64_dec[source[1]] << 4) | (s_base64_dec[source[2]] >> 2);
        if ('=' != source[3]) p[i++] = (s_base64_dec[source[2]] << 6) | s_base64_dec[source[3]];
    }
    return i;
}

static const char *s_base16_enc = "0123456789ABCDEF";
static const uint8_t s_base16_dec[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0 - 9 */
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A - F */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* a - f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// RFC3548 6. Base 16 Encoding (p8)
size_t base16_encode(char *target, const void *source, size_t bytes) {
    size_t i;
    const uint8_t *p;
    p = (const uint8_t *) source;
    for (i = 0; i < bytes; i++) {
        target[i * 2] = s_base16_enc[(*p >> 4) & 0x0F];
        target[i * 2 + 1] = s_base16_enc[*p & 0x0F];
        ++p;
    }
    return bytes * 2;
}

size_t base16_decode(void *target, const char *source, size_t bytes) {
    size_t i;
    uint8_t *p;
    p = (uint8_t *) target;
//    assert(0 == bytes % 2);
    for (i = 0; i < bytes / 2; i++) {
        p[i] = s_base16_dec[source[i * 2] & 0x7F] << 4;
        p[i] |= s_base16_dec[source[i * 2 + 1] & 0x7F];
    }
    return i;
}

#if defined(DEBUG) || defined(_DEBUG)
#include <string.h>
void base64_test(void)
{
    const char* s;
    const char* r;
    char source[512];
    char target[512];

//    assert(8 == base64_encode(source, "4444", 4));
//    assert(4 == base64_decode(target, source, 8));
//    assert(0 == memcmp(source, "NDQ0NA==", 8));
//    assert(0 == memcmp(target, "4444", 4));
//
//    assert(8 == base64_encode(source, "55555", 5));
//    assert(5 == base64_decode(target, source, 8));
//    assert(0 == memcmp(source, "NTU1NTU=", 8));
//    assert(0 == memcmp(target, "55555", 5));
//
//    assert(8 == base64_encode(source, "666666", 6));
//    assert(6 == base64_decode(target, source, 8));
//    assert(0 == memcmp(source, "NjY2NjY2", 8));
//    assert(0 == memcmp(target, "666666", 6));

    // RFC2617 2.Basic Authentication Scheme (p6)
    s = "Aladdin:open sesame";
    r = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";
//    assert(strlen(r) == base64_encode(source, s, strlen(s)));
//    assert(strlen(s) == base64_decode(target, source, strlen(r)));
//    assert(0 == memcmp(source, r, strlen(r)));
//    assert(0 == memcmp(target, s, strlen(s)));
//
//    assert(8 == base16_encode(source, "4444", 4));
//    assert(4 == base16_decode(target, source, 8));
//    assert(0 == memcmp(target, "4444", 4));
}
#endif
