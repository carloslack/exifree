/*
MIT License

Carlos Carvalho

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/limits.h>
#include <libgen.h>

#include "buffer.h"
#include "fs.h"

#define OUTPREFIX   "wiped_"
#define OUTPREFIXLEN    strlen(OUTPREFIX)

enum {
    SET_VERBOSE = 1,
    SET_DRYRUN = 2,
    SET_BINDUMP = 4
};

static void hlp(char arg)
{
    if (arg)
        printf("exifree: invalid option --  '%c'\n", arg);
    printf("Usage: exifree <filename> [OPTION]...\n\n");
    printf("Optional rguments\n");
    printf(" -d <dirname>\t  output directory\n");
    printf(" -v\t\t  verbose dump sections\n");
    printf(" -r\t\t  dry-run\n");
    printf(" -s\t\t  save exif sections in dirname\n");
    exit(0);
}

static int _IFD0(uint32_t offset, bool bin)
{
    uint32_t pos = offset;
    int count = 0;
    struct exif *e = get_chunk(pos, 2);
    pos+=2;

    uint16_t total = data16(e->chunk);
    for (int i = 0; i < total; ++i, pos+=DIR_ENTRY_LEN) {
        e = get_chunk(pos, DIR_ENTRY_LEN);
        count += exif_scanmeta(e, data16(e->chunk), data16(&e->chunk[2]),
                data32(&e->chunk[4]), data32(&e->chunk[8]), pos, bin);
    }

    return count;
}

static status_e _save_output_file(char *path, unsigned char *buf, long isize, long *osize)
{
    status_e status = fs_write(path, buf, isize, osize);
    if (status == S_OK && isize != *osize)
            status = S_MISMATCH;
    return status;
}

static status_e _scthandler(struct exif *exif, char *path, bool isdt)
{
    struct exif *e = exif;
    char *ext = isdt ? "data" : "sct";
    char *file = str_concat(path, "/", get_sct_type(e->group), ".", e->desc, ".", ext);

    long count = 0;
    status_e status = fs_write(file, e->chunk, e->len, &count);
    FREE(file);

    return status;
}

static void _free_node(struct sct *section) {
    struct sct *s = section;
    if (s) {
        if (s->dt_chunk.freedesc) {
            char *desc = (char*)s->dt_chunk.desc;
            FREE(desc);
        }
        mem_free(s->dt_chunk.chunk, s->sct_chunk.chunk, s);
    }
}

int main(int argc, char **argv)
{
    int count = 0, flags = 0;
    long osize = 0;
    char *pathname = NULL, *ifile = NULL,
         *ofile = NULL, *spath = NULL, *path = ".";
    status_e status;
    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];
        for (++arg; *arg; ++arg) {
            switch(*arg) {
                case 'v':
                    flags |= SET_VERBOSE;
                    break;
                case 'r':
                    flags |= SET_DRYRUN;
                    break;
                case 's':
                    flags |= SET_BINDUMP;
                    break;
                case 'd':
                    path = argv[++i];
                    break;
                default:
                    pathname = argv[i];
                    break;
            }
            if (pathname)
                break;
        }
    }

    if (!pathname)
        hlp(0);

    ifile = strrchr(pathname, '/');
    if (!ifile)
        ifile = pathname;

    FILE *fp = fopen(pathname, "r+b");
    if (!fp) {
        fprintf(stderr, "fopen: %s\n", strerror(errno));
        exit(1);
    }

    long isize;
    if ((status = fs_get_file_size(fp, &isize)) != S_OK) {
        fprintf(stderr, "%s\n", get_status(status));
        exit(1);
    }

    if (!(isize = exif_setfilesize(isize))) {
        fprintf(stderr, "Error: invalid file size %ld\n", isize);
        exit(1);
    }

    unsigned char *buf = exif_setbufferptr(isize);
    if (!buf) {
        printf("Error: cannot set file buffer\n");
        exit(1);
    }

    if (fread(buf, isize, 1, fp) != 1) {
        printf("%s\n", strerror(errno));
        exit(1);
    }

    bool dry = flags & SET_DRYRUN;
    bool bin = flags & SET_BINDUMP;

    // skip the header
    long hoff = -1;
    struct exif *e = get_chunk(0, 40);
    for (size_t i = 0; i+8 < e->len && hoff == -1; ++i) {
        if (!strncmp((const char *)&e->chunk[i], "Exif", 4))
            hoff = i+6;
    }

    hoff = exif_setheaderoffset(hoff);
    if (hoff) {
        // Skip byte align and header - see below
        int chunk_pos = hoff + 2;

        // TAG mark defines the byte order
        e = get_chunk(chunk_pos, 2);
        chunk_pos += 2;

        uint16_t order = (e->chunk[0]) << 8| (e->chunk[1]);
        if (!exif_setbyteorder(order)) {
            FCLOSE(fp);
            mem_free(buf, e->chunk);
            exit(1);
        }

        e = get_chunk(chunk_pos, 4);
        uint32_t off = data32(e->chunk);
        if (off < 8 ||off > 16) {
            fprintf(stderr, "Error: invalid offset of first IFD: %u (%08x)\n", off, off);
            FCLOSE(fp);
            mem_free(buf, e->chunk);
            exit(1);
        }
        FREE(e->chunk);

        // Load data and etc...
        count = _IFD0(off+hoff,bin);
        if (count) {
            bool v = flags & SET_VERBOSE;
            exif_ifddata(v,bin);
            exif_subifdata(v,bin);
        }
    }

    if (count) {
        if ((status = fs_mkdir(path, dry)) != S_OK)
            fprintf(stderr, "Error: can't create '%s': %s\n", path, get_status(status));
    }

    // Write the output file
    if (count && !dry && status == S_OK) {
        ofile = str_concat(path, "/", OUTPREFIX, basename(ifile));
        status =_save_output_file(ofile, buf, isize, &osize);

        if (status != S_OK)
            fprintf(stderr, "Can't write %s: %s\n", ofile, get_status(status));
    }

    // Write section & data files
    if (count && (flags & SET_BINDUMP) && status == S_OK) {
        struct sct *node = head;


        // Create destination directory for sections data
        if (node && !dry) {
            spath = str_concat(path, "/", OUTPREFIX, basename(ifile), "-bin");
            if (fs_mkdir(spath, dry) != 0)
                fprintf(stderr, "Error: can't create '%s'\n", spath);
        }
        while (node) {
            if (!dry && status == S_OK && node->round) {
                // Write sections data binary files
                bool isdt = (node->round == DATA) ? true : false;
                struct exif *tmp = isdt ? &node->dt_chunk : &node->sct_chunk;
                status = _scthandler(tmp, spath, isdt);
                if (status != S_OK)
                    fprintf(stderr, "Error writing %s: %s\n", spath, get_status(status));
            }

            // Cleanup section from memory
            struct sct *tmp = node;
            node = node->next;
            FREE(tmp->sct_chunk.chunk);
            _free_node(tmp);
        }
    }
    if (!dry && status == S_OK) {
        printf("\n-= Exifree v0.1 =-\n");
        printf("\ncleared: %lu bytes from %d sectors\n",
                exif_changed(0,0)->bytes, exif_changed(0,0)->changed);
        printf("input file: %s (%ld)\n", pathname, isize);
        printf("output file: %s (%ld)\n", ofile, osize);
        if (spath) {
            printf("binary dumps: %s\n", spath);
            FREE(spath);
        }
    }
    FCLOSE(fp);
    mem_free(ofile, e->chunk, buf);

    return 0;
}

