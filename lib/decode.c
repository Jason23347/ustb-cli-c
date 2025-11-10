#include "conf.h"

#include "decode.h"

#ifdef USE_ICONV

#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>

int
decode_gb2312(gstr_t *utf8_out, const gstr_t *gb_in) {
    size_t in_bytes_left = gb_in->len;
    size_t out_bytes_left = utf8_out->cap;
    char *inbuf = gb_in->data;
    char *outbuf = utf8_out->data;

    iconv_t cd = iconv_open("UTF-8", "GB2312");
    if (cd == (iconv_t)-1) {
        perror("iconv_open");
        return -1;
    }

    if ((size_t)-1 ==
        iconv(cd, &inbuf, &in_bytes_left, &outbuf, &out_bytes_left)) {
        perror("iconv");
        iconv_close(cd);
        return -1;
    }

    return 0;
}

#elif defined(USE_BUILTIN_DECODER)

gstr_t *
decode_gb2312(const gstr_t *gb_in) {
    /* NOT IMPLEMENT */
    return gb_in;
}

#else /* GB2312_DECODER_DISABLED */

gstr_t *
decode_gb2312(const gstr_t *gb_in) {
    /* NOT IMPLEMENT */
    return gb_in;
}

#endif
