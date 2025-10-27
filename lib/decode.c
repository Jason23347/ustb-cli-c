#include "conf.h"

#include "decode.h"

#ifdef USE_ICONV

#include <iconv.h>
#include <stdlib.h>

gstr_t *
decode_gb2312(const gstr_t *gb_str) {
    size_t maxlen = (gb_str->len + 1) / 2 * 3;
    char *buff = calloc(maxlen, 1);
    gstr_t *utf8_str = malloc(sizeof(gstr_t));
    if ((buff == NULL) || (utf8_str == NULL)) {
        return NULL;
    }

    utf8_str->s = buff;
    utf8_str->cap = maxlen;
    utf8_str->len = 0;

    size_t in_bytes_left = gb_str->len;
    size_t out_bytes_left = utf8_str->cap;
    char *inbuf = gb_str->s;
    char *outbuf = utf8_str->s;

    iconv_t cd = iconv_open("UTF-8", "GB2312");
    if (cd == (iconv_t)-1) {
        perror("iconv_open");
        free(buff);
        free(utf8_str);
        return NULL;
    }

    if (iconv(cd, &inbuf, &in_bytes_left, &outbuf, &out_bytes_left) ==
        (size_t)-1) {
        perror("iconv");
        iconv_close(cd);
        free(buff);
        free(utf8_str);
        return NULL;
    }

    return utf8_str;
}

#elif defined(USE_BUILTIN_DECODER)

gstr_t *
decode_gb2312(const gstr_t *gb_str) {
    /* NOT IMPLEMENT */
    return gb_str;
}

#else /* GB2312_DECODER_DISABLED */

gstr_t *
decode_gb2312(const gstr_t *gb_str) {
    /* NOT IMPLEMENT */
    return gb_str;
}

#endif
