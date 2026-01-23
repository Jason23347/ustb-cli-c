#include <string.h>

#include "openssl/evp.h"

void
md5(char *dest, const char *str) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
    EVP_DigestUpdate(ctx, str, strlen(str));
    EVP_DigestFinal_ex(ctx, md, &md_len);
    EVP_MD_CTX_free(ctx);

    // 转成十六进制字符串
    for (unsigned int i = 0; i < md_len; i++) {
        sprintf(&dest[i * 2], "%02x", md[i]);
    }
}
