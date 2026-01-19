#ifndef AESGCM_UTIL_H
#define AESGCM_UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int aesGcm(FILE *in, FILE *out, int do_encrypt, unsigned char *key, unsigned char *IV, unsigned char *tag);


#ifdef __cplusplus
}
#endif

#endif