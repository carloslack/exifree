#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>


#include "fs.h"

status_e _is_error(void)
{
    if (errno == ENOENT || errno == EACCES
            || errno == EPERM || errno == EEXIST)
        return S_OK;

    return S_ERRNO;
}
static status_e _check_access(const char *path)
{
    status_e rc = S_OK;
    if (access(path, R_OK|W_OK) == -1)
        rc = _is_error();
    else {
        struct stat st;
        if (stat(path, &st) == -1)
            rc = _is_error();
        else if (!S_ISDIR(st.st_mode))
            rc = _is_error();
    }

    return rc;
}

static char *_normalize_path(char *str, int len)
{
    char *orig = str, s[len+1];
    int i = 0;
    char sl = 0;
    while(*orig && i < len) {
        if (*orig == '/' && sl == '/') {
            ++orig;
        } else
            s[i++] = sl = *orig++;
    }
    if (s[i-1] == '/')
        s[i-1] = 0;
    s[i] = 0;

    return strdup(s);
}
static status_e _mkdir_p(char *dir, const mode_t mode, bool dryrun)
{
    int rc = S_OK;
    char *s = _normalize_path(dir, strlen(dir));
    if (s) {
        int len = strlen(s);
        int i = 0;
        for (char *ptr = s; *ptr; ptr++, i++) {
            if (!i && *ptr == '/') {
                ++i;
                ++ptr;
                continue;
            }
            if (*ptr == '/') {
                char dname[i+2];
                snprintf(dname, i+1, "%s", s);
                dname[i+2] = '\0';
                rc = _check_access(dname);
                if (!dryrun)
                    if (mkdir(dname, mode) == -1)
                        rc = _is_error();
            }
            if (len == i+1) {
                rc = _check_access(s);
                if (!dryrun)
                    if (mkdir(s, mode) == -1)
                        rc = _is_error();
            }
        }
        FREE(s);
    }

    return rc;
}

status_e fs_mkdir(char *dir, bool dryrun)
{
    if (!dir)
        return S_ARG;

    return _mkdir_p(dir, 0755, dryrun);
}

status_e fs_get_file_size(FILE *file, long *count)
{
    if (!file || !count)
        return S_ARG;

    fseek(file, 0, SEEK_END);
    *count = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (!*count)
       return S_EMPTY;

    return S_OK;
}

status_e fs_write(char *filename, unsigned char *buf, long len, long *rv)
{
    if (!buf || !filename || len <= 0L)
        return S_ARG;

    FILE *fp;
    status_e status = S_OK;
    if (!(fp = fopen(filename, "w+b")))
        status = S_ERRNO;
    else {
        fwrite(buf, len, 1, fp);
        status = fs_get_file_size(fp, rv);
        FCLOSE(fp);
    }

    return status;
}

