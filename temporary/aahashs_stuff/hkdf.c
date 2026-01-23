#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#include <openssl/rand.h>
#include "hkdf.h"

static void hexdump(const char *label, const unsigned char *buf, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) printf("%02X", buf[i]);
    printf("\n");
}

int derive_aes_key_hkdf(const unsigned char *shared_secret_k, size_t sc_len,
                        const unsigned char *info, const unsigned char *salt, size_t salt_len,
                        unsigned char *aes_key, size_t aes_key_len)
{
    EVP_KDF *kdf = NULL;
    EVP_KDF_CTX *kdf_ctx = NULL;
    // out = output aes key generated from hkdf, key = input key from ECDH
    // WARNING, shared_secret is set to 32 bytes but may not work with different curves
    // Better to dynamically allocate the size (similar to the malloc function below)
    // unsigned char aes_key[32], shared_secret_k[32], salt[32];

    // Info make it unchangeable
    //const unsigned char *info = "aes-256-gmc-cryptography-key";
    OSSL_PARAM params[5];

    
    if (!(kdf = EVP_KDF_fetch(NULL, "HKDF", NULL))) {
        printf("Error: Could not Fetch EVP_KDF\n");
        return 0;
    }

    if(!(kdf_ctx = EVP_KDF_CTX_new(kdf))){
        printf("Error: Could not fetch KDF CTX\n");
        return 0;
    }

    // https://docs.openssl.org/3.0/man7/EVP_KDF-HKDF/#examples
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, SN_sha256, strlen(SN_sha256));

    // Set parameters of input key
    params[1] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, shared_secret_k, sc_len);

    // Set info parameter (cast as char * because strlen expect 'const char *' not 'unsigned char *')
    params[2] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, info, strlen((char *)info));
    params[3] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, salt, salt_len);
    params[4] = OSSL_PARAM_construct_end();

    /*
    Processes any parameters in params and then derives keylen bytes of key material and places it in the key buffer. 
    If the algorithm produces a fixed amount of output then an error will occur unless the keylen parameter is equal to 
    that output size, as returned by EVP_KDF_CTX_get_kdf_size().
    */
    if (EVP_KDF_derive(kdf_ctx, aes_key, aes_key_len, params) <= 0) {
        printf("Error: Could not derive aes_key");
        return 0;
    }

    hexdump("Key:",shared_secret_k, sc_len);
    printf("Info: [%s]\n",info);
	hexdump("Salt", salt,salt_len);
    // hexdump("\nKDF: ",aes_key, aes_key_len);

    EVP_KDF_free(kdf);
    EVP_KDF_CTX_free(kdf_ctx);
    return 1;

}