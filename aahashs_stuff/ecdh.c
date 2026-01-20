#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/rand.h>
#include "hkdf.h"
#include "aesGcm.h"

static void hexdump(const char *label, const unsigned char *buf, size_t len) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) printf("%02X", buf[i]);
    printf("\n");
}

EVP_PKEY *generate_key(void){

    /*
    EVP_PKEY is a generic structure to hold diverse types of asymmetric keys (also known as "key pairs"), 
    and can be used for diverse operations, like signing, verifying signatures, key derivation, etc. 
    The asymmetric keys themselves are often referred to as the "internal key".

    Conceptually, an EVP_PKEY internal key may hold a private key, a public key, or both (a keypair), 
    and along with those, key parameters if the key type requires them. The presence of these components determine 
    what operations can be made; for example, signing normally requires the presence of a private key, 
    and verifying normally requires the presence of a public key.
    */
    EVP_PKEY *key = NULL;

    // The EVP_PKEY_CTX structure is an opaque public key algorithm context used by the OpenSSL high-level public key API
    // Think of EVP_PKEY_CTX like an object with multiple possible modes (derive, sign, verify, encrypt, etc.).
    EVP_PKEY_CTX *ctx = NULL;

    // Allocate public key algorithm context using the pkey key type and ENGINE e
    // https://docs.openssl.org/3.0/man7/OSSL_PROVIDER-default/#asymmetric-cipher
    if(!(ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL))){
        printf("Error (generate_key): EVP_PKEY_CTX_new_from_name\n");
    }
    
    // Initializes a public key algorithm context ctx for a key generation operation
    // https://docs.openssl.org/3.0/man3/EVP_PKEY_keygen/#description
    if(!EVP_PKEY_keygen_init(ctx)){
        printf("Error: (generate_key): EVP_PKEY_keygen_init\n");
    }

    // The params field is a pointer to a list of OSSL_PARAM structures, 
    // terminated with a OSSL_PARAM_END(3) struct
    // Elliptic curve type will be prime256v1 specified in https://docs.openssl.org/3.0/man3/OSSL_PARAM/#synopsis
    // May need a more secure elliptic curve, can find in here https://safecurves.cr.yp.to/
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0);
    params[1] = OSSL_PARAM_construct_end();

    // Allow transfer of arbitrary key parameters to and from providers
    // https://docs.openssl.org/3.0/man3/EVP_PKEY_CTX_set_params/#name
    EVP_PKEY_CTX_set_params(ctx, params);

    // Performs the generation operation, the resulting key parameters or key are written to *ppkey 
    // (points to address) of key. If *ppkey is NULL when this function is called, it will be allocated
    // https://docs.openssl.org/3.0/man3/EVP_PKEY_keygen/#description
    EVP_PKEY_generate(ctx, &key);

    EVP_PKEY_CTX_free(ctx);

    return key;
}


// Private key from host A
// Public key from peer host B
// Derives the shared secret key
unsigned char *derive_secret(EVP_PKEY *priv_key, EVP_PKEY *peer_pub_key, size_t *sc_len)
{
    
    EVP_PKEY_CTX *dctx = NULL;
    
    // Function allocates memory to dctx. Binds a public key algorithm context using the algorithm specified by pkey
    if(!(dctx = EVP_PKEY_CTX_new_from_pkey(NULL, priv_key, NULL))){
        printf("Error (derive_secret): EVP_PKEY_CTX_new_from_pkey\n");
        return 0;
    }

    // Initializes a public key algorithm context for shared secret derivation using the algorithm
    // given when the context was created using EVP_PKEY_CTX_new(3) or variants thereof
    // In other words it configures the context specifically for key derivation
    if(!EVP_PKEY_derive_init(dctx)){
        printf("Error (derive_secret): EVP_PKEY_derive_init\n");
        return 0;
    }

    // EVP_PKEY_derive_set_peer_ex() sets the peer key: this will normally be a public key. 
    // The validate_peer (internal) will validate the public key if this value is non zero.
    if(!(EVP_PKEY_derive_set_peer(dctx, peer_pub_key))){
        printf("Error (derive_secret): EVP_PKEY_derive_set_peer\n");
        return 0;
    }

    // Derives a shared secret using ctx. If key is NULL then the maximum size of the output buffer is written to the 
    // keylen parameter. If key is not NULL then before the call the keylen parameter should contain the length of the 
    // key buffer, if the call is successful the shared secret is written to key and the amount of data written to keylen.
    // Determine length of buffer for shared_secret and write to keylen parameter, sc_len
    // Compute the shared secret
    if(EVP_PKEY_derive(dctx, NULL, sc_len) <= 0){
        printf("Error (derive_secret): EVP_PKEY_derive\n");
        return 0;
    }

    // Allocate buffer length to shared_secret
    unsigned char *shared_secret = NULL;
	if(!(shared_secret = OPENSSL_malloc(*sc_len))){
        printf("Error (derive_secret): Failed to allocate buffer length to shared_secret\n");
        return 0;
    }

    // If succesful, write the shared secret to the key, shared_secret
    if(EVP_PKEY_derive(dctx, shared_secret, sc_len) <= 0){
        printf("Error (derive_secret): EVP_PKEY_derive, could not write to shared_secret\n");
        return 0;
    }

    // Free Resources
    EVP_PKEY_CTX_free(dctx);
    return shared_secret;
}

int main(int argc, char** argv){

    // Need a function to get the application private key
    // Same for USB private key
    // Need to generate usb public key
    // Shared secret is app_secret = (usb_pub, app_priv) or (usb_priv, app_pub)

    // EVP_PKEY *usb_priv = load_usb();
    // EVP_PKEY *app_priv = load_app();
    // But for now we'll use test vectors

    // USB Private Key
    EVP_PKEY *userA = NULL;

    // Application public key
    EVP_PKEY *userB = NULL;

    if(!(userA = generate_key())){
        printf("Could not generate key for userA\n");
        return 0;
    }
    if(!(userB = generate_key())){
        printf("Could not generate key for userB\n");
        return 0;
    }
    
    size_t userA_len = 0, userB_len = 0;

    unsigned char *shared_secret = derive_secret(userA, userB, &userA_len);
    if (!shared_secret){
        printf("Error: Could not find shared_secret\n");
        return 0;
    }

    printf("Shared secret: ");
    for (size_t i = 0; i < userA_len; i++){
        printf("%02x", shared_secret[i]);
    }
    printf("\n");

    // Derive AES Key using HMAC-based Key Derivation Function (HKDF)
    unsigned char aes_key[32];
    const unsigned char info[] = "aes-256-gmc-cryptography-key";
    unsigned char salt[32];
    if(!(RAND_bytes(salt, sizeof(salt)))){
        printf("Error: Could not generate random salt\n");
    }
    derive_aes_key_hkdf(shared_secret, userA_len, info, salt, sizeof(salt), aes_key, sizeof(aes_key));
    if (!aes_key) {
        printf("HKDF failed\n");
    }else{
        printf("HKDF passed\n");
    }
    
    hexdump("\nKDF: ",aes_key, sizeof(aes_key));

    // unsigned char IV[12] = {
    //     0x51, 0x6c, 0x33, 0x92, 0x9d, 0xf5, 0xa3, 0x28, 0x4f, 0xf4, 0x63, 0xd7
    // };

    // Generate random IV (nonce) value for AES-GCM
    unsigned char iv[12];
    if(!(RAND_bytes(iv, sizeof(iv)))){
        printf("Error: Could not generate random IV\n");
    }

    unsigned char tag[16];

    FILE *infile = fopen("input.pdf", "rb");
    FILE *outfile_encrypted = fopen("output.enc", "wb");
    if(!infile || !outfile_encrypted) {
        perror("File open failed");
        return 1;
    }

    if (1 != aesGcm(infile, outfile_encrypted, 1, aes_key, iv, tag)){
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
    if (1 != aesGcm(infile_encrypted, outfile_decrypted, 0, aes_key, iv, dec_tag)){
        printf("An error occurred\n");
    }

    fclose(infile_encrypted);
    fclose(outfile_decrypted);

    OPENSSL_free(shared_secret);
    EVP_PKEY_free(userA);
    EVP_PKEY_free(userB);
    return 0;
}