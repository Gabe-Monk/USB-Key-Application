#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/bio.h>

#include "aesGcm.h"

static void hexdump(const char *label, const unsigned char *buf, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) printf("%02X", buf[i]);
    printf("\n");
}

unsigned char *rsa_encrypt(EVP_PKEY *rsa_pub_key, unsigned char *aes_key, size_t aes_key_size, size_t *outlen){
    
    EVP_PKEY_CTX *ctx;
    unsigned char *out;

    ctx = EVP_PKEY_CTX_new(rsa_pub_key, NULL);

    if (EVP_PKEY_encrypt_init(ctx) <= 0){
        printf("Error: \n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0){
        printf("Error: \n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    /* Determine buffer length */
    if (EVP_PKEY_encrypt(ctx, NULL, outlen, aes_key, aes_key_size) <= 0){
        printf("Error: \n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    out = OPENSSL_malloc(*outlen);
    if (!out){
        printf("Error: \n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    if (EVP_PKEY_encrypt(ctx, out, outlen, aes_key, aes_key_size) <= 0){
        printf("Error: \n");
        OPENSSL_free(out);
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);
    return out;
}

int main(int argc, char** argv){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <base64-encoded public RSA key>\n", argv[0]);
        return 1;
    }

    // Generate symmetric key for AES algorithm
    unsigned char aes_key[16];
    if(!(RAND_bytes(aes_key, sizeof(aes_key)))){
        printf("Error: Could not generate random aes_key\n");
        return 1;
    }

    // hexdump("\nRandomly Generated AES key: ",aes_key, sizeof(aes_key));

    // Get public key argument (still encoded in base64)
    char *rsaPubkeyBase64Encoded = argv[1];
    EVP_PKEY *rsaPubKey = NULL;

    // Decode public RSA key from Base64 to DER
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new_mem_buf(rsaPubkeyBase64Encoded, -1);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // CSV key has no line breaks
    mem = BIO_push(b64, mem);

    unsigned char der[4096];
    int der_len = BIO_read(mem, der, sizeof(der));
    BIO_free_all(mem);

    if (der_len <= 0) {
        fprintf(stderr, "Error: Base64 decode of public RSA key failed\n");
        return 1;
    }

    // Convert pubkey from DER to EVP_PKEY
    const unsigned char *p = der;
    rsaPubKey = d2i_PUBKEY(NULL, &p, der_len);

    if (!rsaPubKey) {
        printf("Error: rsaPubKey is NULL\n");
        return 1;
    }

    // Encrypt pub key
    unsigned char *encrypted_key;
    size_t enc_key_len = 0;
    encrypted_key = rsa_encrypt(rsaPubKey, aes_key, sizeof(aes_key), &enc_key_len);
    if(!encrypted_key){
        printf("Error: encrypted_key is NULL\n");
        return 1;
    }

    printf("Encrypted Key: ");
    for (size_t i = 0; i < enc_key_len; i++){
        printf("%02x", encrypted_key[i]);
    }
    printf("\n");

    // Generate random IV (nonce) value for AES-GCM
    unsigned char iv[12];
    if(!(RAND_bytes(iv, sizeof(iv)))){
        printf("Error: Could not generate random IV\n");
        return 1;
    }

    unsigned char tag[16];

    FILE *infile = fopen("input.pdf", "rb");
    if(!infile) {
        perror("File open failed");
        return 1;
    }
    // Send encrypted key and file together
    FILE *output;
    if ((output = fopen("output.bin", "wb")) == NULL){
        printf("Error: output pointer is NULL\n");
        return 1;
    }

    int iv_size = sizeof(iv);
    int tag_size = sizeof(tag);

    // Might be an issue if file is large (keep note of this)
    uint32_t enc_key_len_in_32 = (uint32_t)enc_key_len;
    
    // Write encrypted aes_key, sizeof(unsigned char) is 1
    fwrite(&enc_key_len_in_32, sizeof(enc_key_len_in_32), 1, output);
    fwrite(encrypted_key, 1, enc_key_len, output);
    fwrite(iv, 1, iv_size, output);

    if (1 != aesGcm(infile, output, 1, aes_key, iv, tag)){
        printf("An error occurred\n");
    }

    fwrite(tag, 1, tag_size, output);

    fclose(infile);
    fclose(output);
    EVP_PKEY_free(rsaPubKey);
    OPENSSL_free(encrypted_key);
}


// unsigned char *encrypted_key;
    // size_t encrypted_key_len;
    // rsa_encrypt_aes_key(pubkey, aes_key, 32, &encrypted_key, &encrypted_key_len);


    // // Generate an RSA key (FOR TESTING, should load from USB)
    // // From (https://docs.openssl.org/master/man3/EVP_PKEY_keygen/#notes)
    // EVP_PKEY *pkey = NULL;
    // EVP_PKEY_CTX *kctx = NULL;

    // kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    // if (!kctx){
    //     printf("Error: \n");
    // }
    // /* Error occurred */
    // if (EVP_PKEY_keygen_init(kctx) <= 0){
    //     printf("Error: \n");
    // }
    // /* Error */
    // if (EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, 2048) <= 0){
    //     printf("Error: \n");
    // }
    // /* Generate key */
    // if (EVP_PKEY_keygen(kctx, &pkey) <= 0){
    //     printf("Error: \n");
    // }
    

    // Encrypt key using RSA


    //EVP_PKEY_CTX_free(kctx);