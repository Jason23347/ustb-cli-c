#include "mem.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- site-mapping + 控制接口 ---------- */

/* 哈希表配置（可根据需要调大） */
#define SITE_HASH_SIZE 0x4000 /* 必须是2的幂以便高效掩码（非必要） */
#define SITE_HASH_MASK (SITE_HASH_SIZE - 1)

typedef struct {
    void *ret_addr; /* caller return address */
    int site_id;    /* 分配给该地址的连续 id（从1开始） */
} site_slot_t;

static site_slot_t site_table[SITE_HASH_SIZE];
static pthread_mutex_t site_lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_int next_site_id = ATOMIC_VAR_INIT(1);  /* 下一个分配的 site id */
static atomic_int fail_site_id = ATOMIC_VAR_INIT(-1); /* -1 表示不失败 */

static void mock_mem_init(void) __attribute__((constructor));
static void
mock_mem_init(void) {
    const char *v = getenv("TEST_FAIL_SITE");
    if (v) {
        int id = atoi(v);
        if (id > 0)
            atomic_store(&fail_site_id, id);
    }
}

/* 用于 discover 模式：打印 max site id 到 stdout（供外部脚本读取） */
void
mock_print_max_site_id(void) {
    int max_id = atomic_load(&next_site_id) - 1;
    printf("%d\n", max_id);
    fflush(stdout);
}

/* 简单的指针哈希（把指针转成整数并扰动） */
static inline uint32_t
ptr_hash(void *p) {
    uintptr_t v = (uintptr_t)p;
    /* xorshift-ish mixing */
    v ^= v >> 33;
    v *= 0xff51afd7ed558ccdULL;
    v ^= v >> 33;
    v *= 0xc4ceb9fe1a85ec53ULL;
    v ^= v >> 33;
    return (uint32_t)v;
}

/* 在哈希表中查找或插入 ret_addr，返回对应 site_id（>0） */
static int
site_get_or_create(void *ret_addr) {
    uint32_t h = ptr_hash(ret_addr) & SITE_HASH_MASK;
    pthread_mutex_lock(&site_lock);
    for (uint32_t i = 0; i < SITE_HASH_SIZE; ++i) {
        uint32_t idx = (h + i) & SITE_HASH_MASK;
        if (site_table[idx].ret_addr == NULL) {
            /* 新条目 */
            int id = atomic_fetch_add(&next_site_id, 1);
            site_table[idx].ret_addr = ret_addr;
            site_table[idx].site_id = id;
            pthread_mutex_unlock(&site_lock);
            return id;
        }
        if (site_table[idx].ret_addr == ret_addr) {
            int id = site_table[idx].site_id;
            pthread_mutex_unlock(&site_lock);
            return id;
        }
    }
    /* 表满（极端情况）*/
    pthread_mutex_unlock(&site_lock);
    return -1;
}

/* 查找 ret_addr 的 site_id，如果不存在返回 0 */
static int
site_lookup(void *ret_addr) {
    uint32_t h = ptr_hash(ret_addr) & SITE_HASH_MASK;
    pthread_mutex_lock(&site_lock);
    for (uint32_t i = 0; i < SITE_HASH_SIZE; ++i) {
        uint32_t idx = (h + i) & SITE_HASH_MASK;
        if (site_table[idx].ret_addr == NULL) {
            pthread_mutex_unlock(&site_lock);
            return 0;
        }
        if (site_table[idx].ret_addr == ret_addr) {
            int id = site_table[idx].site_id;
            pthread_mutex_unlock(&site_lock);
            return id;
        }
    }
    pthread_mutex_unlock(&site_lock);
    return 0;
}

/* ---------- 测试控制 API ---------- */

int
test_get_fail_site(void) {
    return atomic_load(&fail_site_id);
}

void
test_set_fail_site(int site_id) {
    atomic_store(&fail_site_id, site_id);
}

void
test_set_fail_site_by_addr(void *addr) {
    int id = site_lookup(addr);
    if (id == 0) {
        /* 如果还未见到该地址，插入并返回其id */
        id = site_get_or_create(addr);
    }
    atomic_store(&fail_site_id, id);
}

void
test_reset_fail_site(void) {
    atomic_store(&fail_site_id, -1);
}

int
test_get_max_site_id(void) {
    /* next_site_id 是下一个要分配的 id，因此最大已分配 id = next_site_id - 1 */
    return atomic_load(&next_site_id) - 1;
}

int
test_addr_to_site_id(void *addr) {
    return site_lookup(addr);
}

/* ---------- 实际的分配函数（mock 实现） ---------- */

/* 我们在 mock 中调用真实的 libc malloc/calloc/realloc/free。
   为了避免直接递归调用到本文件的 __wrap_*，直接使用 libc 的 malloc 系列。 */

void *__real__test_malloc(const size_t size, const char *file, const int line);
void *__real__test_realloc(void *ptr, const size_t size, const char *file,
                           const int line);
void *__real__test_calloc(const size_t number_of_elements, const size_t size,
                          const char *file, const int line);
void __real__test_free(void *const ptr, const char *file, const int line);

/* helper: 判断当前调用 site 是否应该失败 */
int
should_fail_for_caller(void *caller_retaddr) {
    if (!caller_retaddr)
        return 0;
    int site = site_lookup(caller_retaddr);
    if (site == 0) {
        /* 首次遇到这个 site，创建并获取 id */
        site = site_get_or_create(caller_retaddr);
    }
    int f = atomic_load(&fail_site_id);
    return (f > 0 && f == site);
}
