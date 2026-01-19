#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/err.h>
#include <openssl/evp.h>

void handleErrors(void)
{
    unsigned long errCode;

    printf("An error occurred\n");
    while(errCode = ERR_get_error())
    {
        char *err = ERR_error_string(errCode, NULL);
        printf("%s\n", err);
    }
    abort();
}

// In Examples
int aesGcm(FILE *in, FILE *out, int do_encrypt, unsigned char *key, unsigned char *IV, unsigned char *tag){

    int inlen, outlen;
    unsigned char inbuf[4096], outbuf[4096];

    EVP_CIPHER_CTX *ctx = NULL;
    EVP_CIPHER *cipher = NULL;

    // Allocates and returns a cipher context.
    if (!(ctx = EVP_CIPHER_CTX_new())){
        printf("Error (EVP_CIPHER_CTX_new): Could not create EVP_CIPHER_CTX\n");
        handleErrors();
        return 0;
    }
    if(!(cipher = EVP_CIPHER_fetch(NULL, "AES-256-GCM", NULL))){
        printf("Error (EVP_CIPHER_fetch): Could not return a pointer to then EVP_CIPHER\n");
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
        return 0;
    }

    // These functions can be used for decryption or encryption. 
    // The operation performed depends on the value of the enc parameter. 
    // It should be set to 1 for encryption, 0 for decryption and -1 to leave the value unchanged 
    // (the actual value of 'enc' being supplied in a previous call).
    // Sets up cipher context ctx for encryption with cipher type. Initialize the key and Iv
    if(!EVP_CipherInit_ex2(ctx, cipher, key, IV, do_encrypt, NULL)){
        // Error handle function should abort so rest of code doesn't run
        printf("Error (EVP_CipherInit_ex2): Failed to setup cipher ctx\n");
        EVP_CIPHER_free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
        return 0;
    }

    // Encrypt file in chunks of 4096 bytes
    for (;;) {
        inlen = fread(inbuf, 1, 4096, in);
        if (inlen <= 0)
            break;
        if (!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, inlen)) {
            /* Error */
            printf("Error (EVP_CipherUpdate): Cipher Update Failure in for loop\n");
            EVP_CIPHER_free(cipher);
            EVP_CIPHER_CTX_free(ctx);
            handleErrors();
            return 0;
        }
        fwrite(outbuf, 1, outlen, out);
    }

    if (do_encrypt == 0){

        if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag)){
            printf("Error (EVP_CIPHER_CTX_ctrl): Failed to SET tag value\n");
            EVP_CIPHER_free(cipher);
            EVP_CIPHER_CTX_free(ctx);
            handleErrors();
            return 0;
        }
    }

    if (!EVP_CipherFinal_ex(ctx, outbuf, &outlen)) {
        /* Error */
        printf("Error (EVP_CipherFinal_ex): Failed to finalize\n");
        EVP_CIPHER_free(cipher);
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
        return 0;
    }
    fwrite(outbuf, 1, outlen, out);
    
    if (do_encrypt == 1){
        // Get the tag
        if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)){
            printf("Error (EVP_CIPHER_CTX_ctrl): Failed to GET tag value\n");
            EVP_CIPHER_free(cipher);
            EVP_CIPHER_CTX_free(ctx);
            handleErrors();
            return 0;
        }
    }

    EVP_CIPHER_free(cipher);
    EVP_CIPHER_CTX_free(ctx);

    return 1;
}

int main(int argc, char** argv){

    unsigned char key[32] = {
        0xb5, 0x2c, 0x50, 0x5a, 0x37, 0xd7, 0x8e, 0xda, 0x5d, 0xd3, 0x4f, 0x20, 0xc2, 0x25, 0x40, 0xea,
        0x1b, 0x58, 0x96, 0x3c, 0xf8, 0xe5, 0xbf, 0x8f, 0xfa, 0x85, 0xf9, 0xf2, 0x49, 0x25, 0x05, 0xb4
    };
    unsigned char IV[12] = {
        0x51, 0x6c, 0x33, 0x92, 0x9d, 0xf5, 0xa3, 0x28, 0x4f, 0xf4, 0x63, 0xd7
    };
    unsigned char tag[16];


    FILE *infile = fopen("input.pdf", "rb");
    FILE *outfile_encrypted = fopen("output.enc", "wb");
    if(!infile || !outfile_encrypted) {
        perror("File open failed");
        return 1;
    }

    if (1 != aesGcm(infile, outfile_encrypted, 1, key, IV, tag)){
        printf("An error occurred\n");
    }

    fclose(infile);
    fclose(outfile_encrypted);

    unsigned char dec_tag[16];
    memcpy(dec_tag, tag, 16);

    FILE *infile_encrypted = fopen("output.enc", "rb");
    FILE *outfile_decrypted= fopen("input_decrypted.pdf", "wb");

    if(!infile_encrypted || !outfile_decrypted) {
        perror("File open failed");
        return 1;
    }
    if (1 != aesGcm(infile_encrypted, outfile_decrypted, 0, key, IV, dec_tag)){
        printf("An error occurred\n");
    }

    fclose(infile_encrypted);
    fclose(outfile_decrypted);

    return 0;

}