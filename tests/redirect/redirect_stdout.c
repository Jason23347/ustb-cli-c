// capture_stdout.c
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 全局状态（测试用，线程不安全）
static int g_pipe_fd[2] = {-1, -1}; // [0]=read, [1]=write (原始)
static int g_stdout_backup = -1;
static char *g_captured = NULL;

// 开始重定向 stdout 到 pipe
void
redirect_stdout(void) {
    if (g_pipe_fd[0] != -1)
        return; // 已经重定向

    if (pipe(g_pipe_fd) != 0) {
        perror("pipe");
        exit(1);
    }

    // 备份 stdout
    fflush(stdout);
    g_stdout_backup = dup(fileno(stdout));
    if (g_stdout_backup == -1) {
        perror("dup");
        exit(1);
    }

    // 将 stdout 指向 pipe 的写端
    if (dup2(g_pipe_fd[1], fileno(stdout)) == -1) {
        perror("dup2");
        exit(1);
    }

    // 关闭我们不需要的写端副本（stdout 已经有一个 fd 指向它）
    close(g_pipe_fd[1]);
    g_pipe_fd[1] = -1;

    // 清除上次捕获
    free(g_captured);
    g_captured = NULL;
}

// 恢复 stdout，并读取 pipe 中的全部内容到内存（g_captured）
void
restore_stdout(void) {
    if (g_pipe_fd[0] == -1 && g_stdout_backup == -1)
        return; // 未重定向

    // 确保所有缓冲数据都写出到 pipe
    fflush(stdout);

    // 还原 stdout
    if (g_stdout_backup != -1) {
        if (dup2(g_stdout_backup, fileno(stdout)) == -1) {
            perror("dup2 restore");
            // 继续尝试读取 pipe
        }
        close(g_stdout_backup);
        g_stdout_backup = -1;
    }

    // 现在从 pipe 读端读取全部数据
    if (g_pipe_fd[0] != -1) {
        size_t cap = 0, len = 0;
        char *buf = NULL;
        ssize_t n;
        char tmp[4096];

        // 将读端设为 blocking（默认即可），然后读取直到 EOF（写端被关闭/还原）
        while ((n = read(g_pipe_fd[0], tmp, sizeof(tmp))) > 0) {
            if (len + (size_t)n + 1 > cap) {
                size_t newcap = cap ? cap * 2 : (size_t)n + 1;
                while (newcap < len + (size_t)n + 1)
                    newcap *= 2;
                char *nb = realloc(buf, newcap);
                if (!nb) {
                    perror("realloc");
                    free(buf);
                    close(g_pipe_fd[0]);
                    g_pipe_fd[0] = -1;
                    return;
                }
                buf = nb;
                cap = newcap;
            }
            memcpy(buf + len, tmp, (size_t)n);
            len += (size_t)n;
        }

        // 关闭读端
        close(g_pipe_fd[0]);
        g_pipe_fd[0] = -1;

        if (buf) {
            buf[len] = '\0';
            g_captured = buf;
        } else {
            g_captured = strdup("");
        }
    }
}

// 返回捕获字符串（只读），如果没有捕获则返回 ""，调用者不要 free
const char *
get_captured_stdout(void) {
    return g_captured ? g_captured : "";
}

// 释放捕获字符串（测试结束时调用）
void
free_captured_stdout(void) {
    free(g_captured);
    g_captured = NULL;
}
