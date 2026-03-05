#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "aesGcm.h"

static void hexdump(const char *label, const unsigned char *buf, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) printf("%02X", buf[i]);
    printf("\n");
}

unsigned char *rsa_decrypt(EVP_PKEY *rsa_priv_key, unsigned char *aes_key, size_t aes_key_size, size_t *outlen){
    EVP_PKEY_CTX *ctx;
    unsigned char *out;

    ctx = EVP_PKEY_CTX_new(rsa_priv_key, NULL);

    if (EVP_PKEY_decrypt_init(ctx) <= 0){
        printf("Error: \n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0){
        printf("Error: \n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    if (EVP_PKEY_decrypt(ctx, NULL, outlen, aes_key, aes_key_size) <= 0){
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

    if (EVP_PKEY_decrypt(ctx, out, outlen, aes_key, aes_key_size) <= 0){
        printf("Error: \n");
        OPENSSL_free(out);
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    EVP_PKEY_CTX_free(ctx);
    return out;
}



int main(int argc, char** argv){
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <path to file to decrypt> <path to output file> <base64-encoded private RSA key>\n", argv[0]);
        return 1;
    }

    FILE *encrypted_load = fopen(argv[1], "rb");
    if (encrypted_load == NULL){
        printf("Error\n");
    }

    // Extract encrypted key
    uint32_t enc_key_len = 0;
    fread(&enc_key_len, sizeof(enc_key_len), 1, encrypted_load);
    unsigned char *encrypted_key = OPENSSL_malloc(enc_key_len);
    fread(encrypted_key, 1, enc_key_len, encrypted_load);

    unsigned char iv[12];
    fread(&iv, 1, 12, encrypted_load);

    long enc_file_offset = sizeof(uint32_t) + enc_key_len + 12;
    fseek(encrypted_load, 0, SEEK_END);
    long encrypted_load_size = ftell(encrypted_load);
    
    long enc_file_len = encrypted_load_size - enc_file_offset - 16;
    fseek(encrypted_load, enc_file_offset, SEEK_SET);
    unsigned char *encrypted_file = OPENSSL_malloc(enc_file_len);
    fread(encrypted_file, 1, enc_file_len, encrypted_load);

    unsigned char tag[16];
    fseek(encrypted_load, -16, SEEK_END);
    fread(&tag, 1, 16, encrypted_load);
    fclose(encrypted_load);

    // Get public key argument (still encoded in base64)
    char *rsaPrivKeyBase64Encoded = argv[3];
    EVP_PKEY *rsaPrivKey = NULL;

    // Decode private RSA key from Base64 to DER
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *mem = BIO_new_mem_buf(rsaPrivKeyBase64Encoded, -1);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // CSV key has no line breaks
    mem = BIO_push(b64, mem);

    unsigned char der[4096];
    int der_len = BIO_read(mem, der, sizeof(der));
    BIO_free_all(mem);

    if (der_len <= 0) {
        fprintf(stderr, "Error: Base64 decode of private RSA key failed\n");
        return 1;
    }

    // Convert pubkey from DER to EVP_PKEY
    const unsigned char *p = der;
    rsaPrivKey = d2i_AutoPrivateKey(NULL, &p, der_len);

    if (!rsaPrivKey) {
        printf("Error: rsaPrivKey is NULL\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }
    // PEM_write_PrivateKey(stdout, rsa_priv_key, NULL, NULL, 0, NULL, NULL);
    // fflush(stdout);
    unsigned char *decrypted_key;
    size_t dec_key_len = 0;
    decrypted_key = rsa_decrypt(rsaPrivKey, encrypted_key, enc_key_len, &dec_key_len);

    if(!decrypted_key){
        printf("Error: decrypted_key is NULL\n");
        EVP_PKEY_free(rsaPrivKey);
        OPENSSL_free(encrypted_key);
        return 1;
    }

    // printf("Decrypted Key: ");
    // for (size_t i = 0; i < dec_key_len; i++){
    //     printf("%02x", decrypted_key[i]);
    // }
    // printf("\n");

    
    // Write temporary encrypted file that doesn't contain any keys, just original encrypted file
    FILE *infile_tmp = fopen("output.enc", "wb");
    if(!infile_tmp) {
        perror("File open failed");
        EVP_PKEY_free(rsaPrivKey);
        OPENSSL_free(encrypted_key);
        OPENSSL_free(decrypted_key);
        return 1;
    }
    fwrite(encrypted_file, 1, enc_file_len, infile_tmp);
    fclose(infile_tmp);
    infile_tmp = NULL;

    // Reopen file to pass to AES function
    infile_tmp = fopen("output.enc", "rb");
    if(!infile_tmp) {
        perror("File open failed");
        EVP_PKEY_free(rsaPrivKey);
        OPENSSL_free(encrypted_key);
        OPENSSL_free(decrypted_key);
        return 1;
    }


    // Create a file to store decrypted file
    FILE *outfile= fopen(argv[2], "wb");
    if(!outfile) {
        perror("File open failed");
        fclose(infile_tmp);
        remove("output.enc");
        EVP_PKEY_free(rsaPrivKey);
        OPENSSL_free(encrypted_key);
        OPENSSL_free(decrypted_key);
        return 1;
    }

    if (1 != aesGcm(infile_tmp, outfile, 0, decrypted_key, iv, tag)){
        printf("An error occurred\n");
    }

    fclose(infile_tmp);
    fclose(outfile);
    remove("output.enc");
    EVP_PKEY_free(rsaPrivKey);
    OPENSSL_free(encrypted_key);
    OPENSSL_free(decrypted_key);
    OPENSSL_free(encrypted_file);

}