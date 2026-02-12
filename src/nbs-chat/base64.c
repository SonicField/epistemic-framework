/*
 * base64.c — Base64 encode/decode implementation
 *
 * Invariants:
 *   - decode_table uses 0xFF for invalid entries; only valid base64
 *     characters map to values 0-63, and '=' maps to 64.
 *   - All sextet extractions are masked with & 0x3F.
 *   - Input is validated via is_valid_base64_char() before decode_table lookup.
 *   - An assertion guards the decode_table lookup as a defence-in-depth
 *     measure against the 'A'/0 ambiguity.
 */

#include "base64.h"
#include "chat_file.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

static const char encode_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Decode table: maps ASCII byte to 6-bit value.
 *   0xFF = invalid (uninitialised/non-base64 characters)
 *   64   = padding ('=')
 *   0-63 = valid base64 sextet values
 *
 * C designated initialisers zero-fill unset entries, so we cannot rely on
 * the table alone to distinguish 'A' (value 0) from invalid (also 0).
 * Solution: initialise via init function that fills with 0xFF first,
 * then sets valid entries. The table is initialised once at first use.
 */
static unsigned char decode_table[256];
static int decode_table_initialised = 0;

static void init_decode_table(void) {
    if (decode_table_initialised) return;

    /* Fill entire table with invalid sentinel */
    memset(decode_table, 0xFF, sizeof(decode_table));

    /* Set valid base64 characters */
    for (int i = 0; i < 26; i++) {
        decode_table[(unsigned char)('A' + i)] = (unsigned char)i;
        decode_table[(unsigned char)('a' + i)] = (unsigned char)(26 + i);
    }
    for (int i = 0; i < 10; i++) {
        decode_table[(unsigned char)('0' + i)] = (unsigned char)(52 + i);
    }
    decode_table[(unsigned char)'+'] = 62;
    decode_table[(unsigned char)'/'] = 63;
    decode_table[(unsigned char)'='] = 64;

    decode_table_initialised = 1;
}

/* Validate a character is in the base64 alphabet (including '=' padding) */
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
    ASSERT_MSG(output != NULL,
               "base64_encode: output buffer is NULL; "
               "caller must provide a valid output buffer");
    ASSERT_MSG(input != NULL || input_len == 0,
               "base64_encode: input is NULL with non-zero length %zu; "
               "NULL input is only valid when input_len is 0", input_len);

    size_t needed = base64_encoded_size(input_len);
    if (output_size < needed) {
        fprintf(stderr,
                "base64_encode: output buffer too small: need %zu, got %zu\n",
                needed, output_size);
        return -1;
    }

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

    /* Postconditions */
    ASSERT_MSG(j == needed - 1,
               "base64_encode: output length %zu != expected %zu; "
               "encoding logic produced wrong number of characters", j, needed - 1);
    ASSERT_MSG(j <= (size_t)INT_MAX,
               "base64_encode: output length %zu exceeds INT_MAX; "
               "input too large for int return type", j);

    return (int)j;
}

int base64_decode(const char *input, size_t input_len,
                  unsigned char *output, size_t output_size) {
    /* Ensure decode table is ready */
    init_decode_table();

    /* Preconditions */
    ASSERT_MSG(input != NULL,
               "base64_decode: input buffer is NULL; "
               "caller must provide valid base64 input");
    ASSERT_MSG(output != NULL,
               "base64_decode: output buffer is NULL; "
               "caller must provide a valid output buffer");

    /* Strip trailing whitespace/newlines */
    while (input_len > 0 && (input[input_len - 1] == '\n' ||
                              input[input_len - 1] == '\r' ||
                              input[input_len - 1] == ' ')) {
        input_len--;
    }

    if (input_len == 0) return 0;

    if (input_len % 4 != 0) {
        fprintf(stderr,
                "base64_decode: input length %zu is not a multiple of 4; "
                "valid base64 input must be padded to a multiple of 4 bytes\n",
                input_len);
        return -1;
    }

    /* Validate all characters and padding position */
    for (size_t i = 0; i < input_len; i++) {
        if (!is_valid_base64_char((unsigned char)input[i])) {
            fprintf(stderr,
                    "base64_decode: invalid character 0x%02x at position %zu; "
                    "only A-Z, a-z, 0-9, +, /, = are valid base64 characters\n",
                    (unsigned char)input[i], i);
            return -1;
        }
        /* Padding '=' must only appear at the end (last 1 or 2 characters) */
        if (input[i] == '=' && i < input_len - 2) {
            fprintf(stderr,
                    "base64_decode: padding '=' at position %zu is invalid; "
                    "padding must only appear in the last two positions\n", i);
            return -1;
        }
    }

    size_t out_len = (input_len / 4) * 3;
    if (input[input_len - 1] == '=') out_len--;
    if (input[input_len - 2] == '=') out_len--;

    if (output_size < out_len) {
        fprintf(stderr,
                "base64_decode: output buffer too small: need %zu, got %zu\n",
                out_len, output_size);
        return -1;
    }

    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 4) {
        unsigned int sextet_a = decode_table[(unsigned char)input[i]] & 0x3F;
        unsigned int sextet_b = decode_table[(unsigned char)input[i + 1]] & 0x3F;
        unsigned int sextet_c = decode_table[(unsigned char)input[i + 2]] & 0x3F;
        unsigned int sextet_d = decode_table[(unsigned char)input[i + 3]] & 0x3F;

        /* Defence-in-depth: assert decode_table value is not 0xFF (invalid).
         * The validation loop above should have caught invalid characters,
         * but this guards against the table being bypassed or corrupted. */
        ASSERT_MSG(decode_table[(unsigned char)input[i]] != 0xFF,
                   "base64_decode: decode_table returned invalid sentinel for "
                   "character 0x%02x at position %zu; validation was bypassed",
                   (unsigned char)input[i], i);
        ASSERT_MSG(decode_table[(unsigned char)input[i + 1]] != 0xFF,
                   "base64_decode: decode_table returned invalid sentinel for "
                   "character 0x%02x at position %zu; validation was bypassed",
                   (unsigned char)input[i + 1], i + 1);

        unsigned int triple = (sextet_a << 18) | (sextet_b << 12) |
                              (sextet_c << 6) | sextet_d;

        if (j < out_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < out_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < out_len) output[j++] = triple & 0xFF;
    }

    /* Postconditions */
    ASSERT_MSG(j == out_len,
               "base64_decode: decoded length %zu != expected %zu; "
               "decode loop produced wrong number of bytes", j, out_len);
    ASSERT_MSG(out_len <= (size_t)INT_MAX,
               "base64_decode: output length %zu exceeds INT_MAX; "
               "input too large for int return type", out_len);

    return (int)out_len;
}
