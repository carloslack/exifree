#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <stdarg.h>
#include <linux/limits.h>
#include "buffer.h"

#define IFDNUM 2
enum { GPSIFD = 0, ExifIFD };
static int privIFD[IFDNUM];
void hexdump(unsigned char *, size_t, const char *);

// Track repeated tags
typedef struct {
    int num;
} tag_tick_t;
static tag_tick_t tag_tick[IFD0LIST_MAX];

typedef struct
{
    uint16_t byte_order;
    long file_size;
    long header_offset;
    unsigned char *buf;
} buffer_utils_t;
static buffer_utils_t utils;

typedef struct
{
    uint16_t tag;
    int group;
    size_t pos;
    size_t len;
    char *desc;
} wipe_t;

static const tags_t *_get_data_tag(uint16_t tag)
{
    for (int i = 0; tags[i].desc != NULL; ++i) {
        if (tags[i].tag == tag)
            return &tags[i];
    }
    return NULL;
}

static const format_t *_get_data_format(uint16_t format)
{
    for (int i = 0; data_format[i].format != 0; ++i) {
        if (data_format[i].format == format)
            return &data_format[i];
    }
    return NULL;
}

//_store_section(e, e->tags->desc, e->group, e->tags->tag);
static void _store_section(struct exif *e)
{
    if (!e || e->len <= 0 || !e->tags->desc) {
        fprintf(stderr, "%s", get_status(S_ARG));
        return;
    }

    struct sct *node = CALLOC(1, sizeof(struct sct));
    node->round = SECTION;
    node->sct_chunk.desc = e->tags->desc;
    node->sct_chunk.group = e->group;
    node->sct_chunk.tag = e->tags->tag;
    node->sct_chunk.len = e->len;
    node->sct_chunk.chunk = CALLOC(1, e->len);
    memcpy(node->sct_chunk.chunk, e->chunk, e->len);
    node->next = head;
    head = node;
}

static void _store_data(struct exif *e, unsigned char *buf, int rpt)
{
    if (!e || !buf || e->wlen <= 0 || !e->tags->desc) {
        fprintf(stderr, "%s", get_status(S_ARG));
        return;
    }

    static int offlen = sizeof(size_t);

    struct sct *node = head;
    node = CALLOC(1, sizeof(struct sct));

    const char *d = e->tags->desc;
    if (rpt) {
        char num[16] = {0};
        snprintf(num, 15, "-%d", rpt);
        d = str_concat(d,num);
        node->dt_chunk.freedesc = true;
    }

    node->round = DATA;
    node->dt_chunk.desc = d;
    node->dt_chunk.group = e->group;
    node->dt_chunk.tag = e->tags->tag;
    node->dt_chunk.len = e->wlen+offlen;

    // first size_t bytes are offset to this data
    // Not really sure yet what I'm gonna do with it though...
    node->dt_chunk.chunk = CALLOC(1, e->wlen+offlen);
    memcpy(node->dt_chunk.chunk, &e->wpos, offlen);
    memcpy(&node->dt_chunk.chunk[offlen], buf, e->wlen);

    node->next = head;
    head = node;
}

void _wipe_chunk(struct exif *e, bool verbose, bool bin, int rpt)
{
    long file_size = exif_getfilesize();
    if (!e || e->wlen <= 0 || e->wpos < 0 ||
            e->wpos > file_size || e->wpos+e->wlen > file_size) {
        fprintf(stderr, "%s", get_status(S_ARG));
        return;
    }

    unsigned char *buf = exif_getbufferptr();
    if (buf) {
        if (verbose)
            hexdump(buf+e->wpos, e->wlen, e->tags->desc);
        if (bin)
            _store_data(e, buf+e->wpos, rpt);
        memset(buf+e->wpos, 0, e->wlen);
        exif_changed(1, e->wlen);
    }
}

static int _warn_if_tag_repeats(const tags_t *tag, size_t len)
{
    int ret = 0;
    if (!tag_tick[tag->tag].num)
        tag_tick[tag->tag].num = 1;
    else {
        printf("*** Tag %04x/%s: repeats\n", tag->tag, tag->desc);
        ret = tag_tick[tag->tag].num++;

    }
    return ret;
}

static inline uint16_t _data16(unsigned char *chunk)
{
    uint16_t rv = 0;
    if (chunk)
        rv = (chunk[0] & 0xff) << 8| (chunk[1] & 0xff);
    return rv;
}

static inline uint32_t _data32(unsigned char *chunk)
{
    uint32_t rv = 0;
    if (chunk)
        rv = (chunk[0] & 0xff) << 24| (chunk[1] & 0xff) << 16|
                    (chunk[2] & 0xff) << 8| (chunk[3] & 0xff);
    return rv;
}

uint16_t data16(unsigned char *chunk)
{
    uint16_t order = exif_getbyteorder(), retval = 0;
    if (order == INTEL)
        retval = htons(_data16(chunk));
    else if (order == MOTOROLA)
        retval = _data16(chunk);
    return retval;
}

uint32_t data32(unsigned char *chunk)
{
    uint32_t order = exif_getbyteorder(), retval = 0;
    if (order == INTEL)
        retval = htonl(_data32(chunk));
    else if (order == MOTOROLA)
        retval = _data32(chunk);
    return retval;
}

stats_t *exif_changed(int inc, size_t bytes)
{
    static stats_t stats;
    stats.changed += inc;
    stats.bytes += bytes;
    return &stats;
}

uint16_t exif_setbyteorder(uint16_t val)
{
    if (val != INTEL && val != MOTOROLA) {
        fprintf(stderr, "Error: unknwon byte order %04x\n", val);
        return 0;
    }

    return (utils.byte_order = val);
}

uint16_t exif_getbyteorder(void)
{
    return utils.byte_order;
}

long exif_setfilesize(long val)
{
    if (val <= 0)
        return 0;

    return (utils.file_size = val);
}

long exif_getfilesize(void)
{
    return utils.file_size;
}

long exif_setheaderoffset(long val)
{
    if (val <= 0) {
        fprintf(stderr, "Error: Exif header not found\n");
        return 0;
    }
    return (utils.header_offset = val);
}

long exif_getheaderoffset(void)
{
    return utils.header_offset;
}

unsigned char *exif_setbufferptr(long size)
{
    if (!size)
        return NULL;

    return (utils.buf = CALLOC(1, size));
}
unsigned char *exif_getbufferptr(void)
{
    return utils.buf;
}

struct exif *get_chunk(uint32_t pos, size_t len)
{
    static struct exif e;
    long file_size = exif_getfilesize();
    if (pos > file_size || pos+len > file_size) {
        fprintf(stderr, "Invalid file position %u\n", pos);
        // no reason to proceed
        abort();
    }

    FREE(e.chunk);
    e.chunk = CALLOC(1, len+1);

    memcpy(e.chunk, exif_getbufferptr()+pos, len);
    e.len = len;

    return &e;
}

int exif_scanmeta(struct exif *exif, uint16_t tag, uint16_t format, uint32_t nentries,
        uint32_t chunk, uint32_t pos, bool bin)
{
    struct exif *e = exif;
    e->tags = _get_data_tag(tag);
    if (!e->tags) {
        printf("*** Tag %04x: unhandled\n", tag);
        return 0;
    }

    e->fmt = _get_data_format(format);
    if (!e->fmt) {
        printf("*** Format %02x not found: skipping\n", format);
        return 0;
    }

   // GROUP_EXIF and GROUP_GPS are always IFD_PTR
   // IFD_PTR is NOT always GROUP_EXIF or GROUP_GPS
   e->group = 0;
    if (e->tags->tag == 0x8769)
        e->group = SECTION_EXIF;
    else if (e->tags->tag == 0x8825)
        e->group = SECTION_GPS;

    static int idx;
    epriv[idx].tag = e->tags->tag;
    epriv[idx].len = nentries;
    epriv[idx].format = e->fmt->format;
    epriv[idx].group = e->group;
    epriv[idx].len = (e->fmt->bytes * nentries);

    // is it data yet?
    if (epriv[idx].len <= 4 && !e->group)
        // fits 4 in bytes
        epriv[idx].off = pos;
    else {
        // offset to data
        epriv[idx].off = (chunk + exif_getheaderoffset());

        // whether private IFD is present
        if (!privIFD[ExifIFD] && e->group == SECTION_EXIF)
            privIFD[ExifIFD] = idx;
        if (!privIFD[GPSIFD] && e->group == SECTION_GPS)
            privIFD[GPSIFD] = idx;
    }

   // IFD0 sections are stored here
   // SubIFD and GPS sections are located and stored in exif_subifdata
    if (bin) {
        e->group = e->group ? SECTION_PRIV : SECTION_IFD0;
        _store_section(e);
    }

    return ++idx;
}

// erase baseline data
status_e exif_ifddata(bool verbose, bool bin)
{
    for (int i = 0; i < IFD0LIST_MAX; ++i) {
        if (!epriv[i].tag)
            continue;

        bool subifd = ((epriv[i].group == SECTION_GPS) ||
                (epriv[i].group == SECTION_EXIF));
        if (!subifd) {
            struct exif e;
            e.tags = _get_data_tag(epriv[i].tag);
            e.group = epriv[i].group ? epriv[i].group : SECTION_IFD0;
            e.wpos = epriv[i].off;;
            e.wlen = epriv[i].len;
            int rpt = _warn_if_tag_repeats(e.tags, e.wlen);
            _wipe_chunk(&e, verbose, bin, rpt);
        }
    }
    return S_OK;
}

/*
 * SubIFD:
 *   Directory entries (12 bytes per entry) follows.
 *  'tttt'  2 bytes is the Tag number, this shows a kind of data.
 *  'ffff'  2 bytes is data format.
 *  'nnnnnnnn'  4 bytes is number of components.
 *  'dddddddd'  4 bytes contains a data value or offset to data value.
 */
status_e exif_subifdata(bool verbose, bool bin)
{
    for (int i = 0; i < IFDNUM; ++i) {

        if (privIFD[i]) {
            int idx = privIFD[i];
            struct exif *e = get_chunk(epriv[idx].off, 2);
            uint32_t total = data16(e->chunk);

            for (int i = 0, dir = 0; i < total; ++i, dir+=DIR_ENTRY_LEN) {
                uint32_t off = dir + epriv[idx].off + 2;
                e = get_chunk(off, DIR_ENTRY_LEN);
                e->tags = _get_data_tag(data16(&e->chunk[0]));
                e->fmt = _get_data_format(data16(&e->chunk[2]));

                if (e->tags && e->fmt) {
                    e->group = epriv[idx].group;
                    e->wlen = data32(&e->chunk[4]) * e->fmt->bytes;
                    e->wpos = (e->wlen <= 4) ? off+8 : data32(&e->chunk[8]) + exif_getheaderoffset();

                    // SubIFD & GPS sections
                    if (bin)
                        _store_section(e);

                    // number of components * data type
                    int rpt = _warn_if_tag_repeats(e->tags, e->wlen);
                    _wipe_chunk(e, verbose, bin, rpt);
                }
            }
        }
    }
    return S_OK;
}


