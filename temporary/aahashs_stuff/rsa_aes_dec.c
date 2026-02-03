#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

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

    FILE *encrypted_load = fopen("output.bin", "rb");
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

    EVP_PKEY *rsa_priv_key = NULL;
    FILE *fp = fopen("usb_priv.pem", "rb");
    if(!fp){
        printf("Error: Could not read file\n");
        return 1;
    }

    rsa_priv_key = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    if(!rsa_priv_key){
        printf("Error: rsa_priv_key is NULL\n");
        OPENSSL_free(encrypted_key);
        return 1;
    }
    PEM_write_PrivateKey(stdout, rsa_priv_key, NULL, NULL, 0, NULL, NULL);
    fflush(stdout);
    unsigned char *decrypted_key;
    size_t dec_key_len = 0;
    decrypted_key = rsa_decrypt(rsa_priv_key, encrypted_key, enc_key_len, &dec_key_len);

    if(!decrypted_key){
        printf("Error: decrypted_key is NULL\n");
        EVP_PKEY_free(rsa_priv_key);
        OPENSSL_free(encrypted_key);
        return 1;
    }

    printf("Decrypted Key: ");
    for (size_t i = 0; i < dec_key_len; i++){
        printf("%02x", decrypted_key[i]);
    }
    printf("\n");

    
    // Write unsigned char encrypted_file to a file as aesGcm function takes a file pointer
    FILE *outfile_encrypted = fopen("output.enc", "wb");
    if(!outfile_encrypted) {
        perror("File open failed");
        EVP_PKEY_free(rsa_priv_key);
        OPENSSL_free(encrypted_key);
        OPENSSL_free(decrypted_key);
        return 1;
    }
    fwrite(encrypted_file, 1, enc_file_len, outfile_encrypted);
    fclose(outfile_encrypted);

    // Now read it into infile_encrypted to pass to aes function
    FILE *infile_encrypted = fopen("output.enc", "rb");
    if(!infile_encrypted) {
        perror("File open failed");
        EVP_PKEY_free(rsa_priv_key);
        OPENSSL_free(encrypted_key);
        OPENSSL_free(decrypted_key);
        return 1;
    }


    // Create a file to store decrypted file
    FILE *outfile_decrypted= fopen("input_decrypted.pdf", "wb");
    if(!outfile_decrypted) {
        perror("File open failed");
        fclose(infile_encrypted);
        EVP_PKEY_free(rsa_priv_key);
        OPENSSL_free(encrypted_key);
        OPENSSL_free(decrypted_key);
        return 1;
    }

    if (1 != aesGcm(infile_encrypted, outfile_decrypted, 0, decrypted_key, iv, tag)){
        printf("An error occurred\n");
    }

    fclose(infile_encrypted);
    fclose(outfile_decrypted);
    EVP_PKEY_free(rsa_priv_key);
    OPENSSL_free(encrypted_key);
    OPENSSL_free(decrypted_key);
    OPENSSL_free(encrypted_file);

}