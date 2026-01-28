#include "conf.h"

#include "cmd.h"

#include "lib/gbuff.h"

#define __info_extract(destPtr, srcStr, fmtStr, prefixStr, extQuoted)          \
    do {                                                                       \
        const struct extract ext[1] = {{                                       \
            .dest = (destPtr),                                                 \
            .src = (srcStr),                                                   \
            .fmt = &gbuff_from_const(fmtStr),                                  \
            .prefix = &gbuff_from_const(prefixStr),                            \
            .quoted = extQuoted,                                               \
        }};                                                                    \
        int res = gbuff_extract(ext);                                          \
        if (res < 0) {                                                         \
            return USTB_ERR;                                                   \
        }                                                                      \
    } while (0)

USTB_RET
info_extract(info_t *info, const char *content) {
    memset(info, 0, sizeof(info_t));

    const char *script_str = strstr(content, "<script");
    if (script_str == NULL) {
        return USTB_ERR;
    }

    __info_extract(&info->username, content, "%[^'\"]s", "uid", EXT_QUOTED);
    __info_extract(&info->nid, content, "%[^'\"]s", "NID", EXT_QUOTED);

    __info_extract(&info->flow, script_str, uint64_spec, "flow", EXT_QUOTED);
    __info_extract(&info->flow_v6, script_str, uint64_spec, "v6df",
                   EXT_UNQUOTED);
    __info_extract(&info->ipv6_mode, script_str, "%u", "v46m", EXT_UNQUOTED);
    __info_extract(&info->fee_num, script_str, "%u", "fee", EXT_QUOTED);
    __info_extract(&info->ipv4_addr, script_str, "%15[^']", "v4ip", EXT_QUOTED);
    __info_extract(&info->ipv6_addr, script_str, "%39[^']", "v6ip", EXT_QUOTED);

    /* FIXME: Don't know why */
    info->flow_v6 /= 4;

    return USTB_OK;
}

int
logged_in(const info_t *info) {
    return (strlen(info->nid) != 0);
}

int
has_ipv6(const info_t *info) {
    int mode = info->ipv6_mode;
    return ((mode == 4) || (mode == 12));
}
