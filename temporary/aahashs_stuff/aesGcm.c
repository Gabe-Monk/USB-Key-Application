#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include "aesGcm.h"

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

    // Encrypt/Decrypt file in chunks of 4096 bytes
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

    /*
    When decrypting, this call sets the expected tag to taglen bytes from tag. 
    taglen must be between 1 and 16 inclusive. The tag must be set prior to any call to 
    EVP_DecryptFinal() or EVP_DecryptFinal_ex().
    */
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
    
    /*
    Writes taglen bytes of the tag value to the buffer indicated by tag. This call can only be made when encrypting data
    and after all data has been processed (e.g. after an EVP_EncryptFinal() call).
    */
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