/*
 * base64.h — Base64 encode/decode for nbs-chat messages
 *
 * Each chat message is stored as a single base64-encoded line.
 * Standard base64 alphabet (A-Z, a-z, 0-9, +, /) with = padding.
 */

#ifndef NBS_BASE64_H
#define NBS_BASE64_H

#include <stddef.h>

/*
 * base64_encode — Encode binary data to base64 string.
 *
 * Preconditions:
 *   - input != NULL (or input_len == 0)
 *   - output != NULL
 *   - output_size >= base64_encoded_size(input_len)
 *
 * Postconditions:
 *   - output contains null-terminated base64 string
 *   - return value is length of encoded string (excluding null)
 *   - return -1 on error (output too small)
 */
int base64_encode(const unsigned char *input, size_t input_len,
                  char *output, size_t output_size);

/*
 * base64_decode — Decode base64 string to binary data.
 *
 * Preconditions:
 *   - input != NULL
 *   - output != NULL
 *   - output_size >= base64_decoded_size(input_len)
 *
 * Postconditions:
 *   - output contains decoded binary data
 *   - return value is length of decoded data
 *   - return -1 on error (invalid base64 or output too small)
 */
int base64_decode(const char *input, size_t input_len,
                  unsigned char *output, size_t output_size);

/*
 * Size calculation helpers.
 */
static inline size_t base64_encoded_size(size_t input_len) {
    return ((input_len + 2) / 3) * 4 + 1; /* +1 for null terminator */
}

static inline size_t base64_decoded_size(size_t input_len) {
    return (input_len / 4) * 3 + 3; /* conservative upper bound */
}

#endif /* NBS_BASE64_H */
