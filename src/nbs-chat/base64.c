/*
 * base64.c — Base64 encode/decode implementation
 */

#include "base64.h"
#include <assert.h>
#include <string.h>

static const char encode_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Decode table: maps ASCII byte to 6-bit value, 255 = invalid, 64 = padding */
static const unsigned char decode_table[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,
    ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
    ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19,
    ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25,
    ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33,
    ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37,
    ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
    ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45,
    ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49,
    ['y'] = 50, ['z'] = 51,
    ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60, ['9'] = 61,
    ['+'] = 62, ['/'] = 63,
    ['='] = 64,
};

/* All other entries are zero-initialised (invalid) */
static int is_valid_base64_char(unsigned char c) {
    if (c == '=' || c == '+' || c == '/') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= '0' && c <= '9') return 1;
    return 0;
}

int base64_encode(const unsigned char *input, size_t input_len,
                  char *output, size_t output_size) {
    /* Preconditions */
    assert(output != NULL);
    assert(input != NULL || input_len == 0);

    size_t needed = base64_encoded_size(input_len);
    if (output_size < needed) return -1;

    size_t i = 0, j = 0;

    /* Process 3-byte groups */
    for (; i + 2 < input_len; i += 3) {
        unsigned int triple = ((unsigned int)input[i] << 16) |
                              ((unsigned int)input[i + 1] << 8) |
                              ((unsigned int)input[i + 2]);
        output[j++] = encode_table[(triple >> 18) & 0x3F];
        output[j++] = encode_table[(triple >> 12) & 0x3F];
        output[j++] = encode_table[(triple >> 6) & 0x3F];
        output[j++] = encode_table[triple & 0x3F];
    }

    /* Handle remaining bytes */
    if (i < input_len) {
        unsigned int triple = (unsigned int)input[i] << 16;
        if (i + 1 < input_len) {
            triple |= (unsigned int)input[i + 1] << 8;
        }

        output[j++] = encode_table[(triple >> 18) & 0x3F];
        output[j++] = encode_table[(triple >> 12) & 0x3F];

        if (i + 1 < input_len) {
            output[j++] = encode_table[(triple >> 6) & 0x3F];
        } else {
            output[j++] = '=';
        }
        output[j++] = '=';
    }

    output[j] = '\0';

    /* Postcondition */
    assert(j == needed - 1);

    return (int)j;
}

int base64_decode(const char *input, size_t input_len,
                  unsigned char *output, size_t output_size) {
    /* Preconditions */
    assert(input != NULL);
    assert(output != NULL);

    /* Strip trailing whitespace/newlines */
    while (input_len > 0 && (input[input_len - 1] == '\n' ||
                              input[input_len - 1] == '\r' ||
                              input[input_len - 1] == ' ')) {
        input_len--;
    }

    if (input_len == 0) return 0;
    if (input_len % 4 != 0) return -1; /* Invalid length */

    /* Validate all characters */
    for (size_t i = 0; i < input_len; i++) {
        if (!is_valid_base64_char((unsigned char)input[i])) return -1;
    }

    size_t out_len = (input_len / 4) * 3;
    if (input[input_len - 1] == '=') out_len--;
    if (input[input_len - 2] == '=') out_len--;

    if (output_size < out_len) return -1;

    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 4) {
        unsigned int sextet_a = decode_table[(unsigned char)input[i]];
        unsigned int sextet_b = decode_table[(unsigned char)input[i + 1]];
        unsigned int sextet_c = decode_table[(unsigned char)input[i + 2]];
        unsigned int sextet_d = decode_table[(unsigned char)input[i + 3]];

        unsigned int triple = (sextet_a << 18) | (sextet_b << 12) |
                              ((sextet_c & 0x3F) << 6) | (sextet_d & 0x3F);

        if (j < out_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < out_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < out_len) output[j++] = triple & 0xFF;
    }

    /* Postcondition */
    assert(j == out_len);

    return (int)out_len;
}
