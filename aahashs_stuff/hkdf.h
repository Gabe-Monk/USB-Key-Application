#ifndef HKDF_UTIL_H
#define HKDF_UTIL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * derive_aes_key_hkdf:
 * @shared_secret: ECDH shared secret
 * @sc_len: length of shared secret in bytes
 * @info: optional context string
 * @salt: optional salt, can be NULL
 * @salt_len: length of salt, 0 if salt is NULL
 * @aes_key: output buffer to hold derived key
 * @aes_key_len: length of output key (e.g., 32 for AES-256)
 *
 * Returns 1 on success, 0 on failure
 */

int derive_aes_key_hkdf(const unsigned char *shared_secret_k, size_t sc_len, const unsigned char *info, const unsigned char *salt, size_t salt_len, unsigned char *aes_key, size_t aes_key_len);


#ifdef __cplusplus
}
#endif

#endif
