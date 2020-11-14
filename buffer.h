#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#define FREE(x) if(x!=NULL) {\
    free(x);\
    x=NULL;\
}

#define IFD0LIST_MAX 0xffff
#define DIR_ENTRY_LEN 12
#define INTEL 0x2a00
#define MOTOROLA 0x002a

// PP_NARG from
// https://groups.google.com/forum/#!topic/comp.std.c/d-6Mj5Lko_s
#define PP_NARG(...) \
    PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
    PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

struct sct;
struct exif;
void _wipe_chunk(struct exif *e, bool verbose, bool bin, int rpt);
typedef void (*OP)(struct exif*, bool, bool, int);

enum {
    SECTION_SUBIFD = 1,
    SECTION_EXIF,
    SECTION_GPS,
    SECTION_IFD0,
    SECTION_PRIV
};
struct sectiontype_t {
    int type;
    const char *sct_type_desc;
};
static struct sectiontype_t sct_type_handler[] = {
    {SECTION_SUBIFD, "subifd"},
    {SECTION_EXIF, "exif"},
    {SECTION_GPS, "gps"},
    {SECTION_IFD0, "ifd0"},
    {SECTION_PRIV, "priv"},
    {-1}
};
static inline const char *get_sct_type(int s) {
    for (int i = 0; sct_type_handler[i].type != -1; ++i) {
        if (s == sct_type_handler[i].type)
            return sct_type_handler[i].sct_type_desc;
    }
    return "unknown";
}

typedef enum {
    S_OK,
    S_ARG,
    S_RW,
    S_EMPTY,
    S_MISMATCH,
    S_ERRNO,
    S_MEMORY,
    S_INPUT_FILE,
    S_FILE_SIZE,
    S_FILE_BUFFER,
    S_FILE_READ
} status_e;

struct status_t{
    int status;
    const char *status_desc;
};

static struct status_t status_handler[] = {
    {S_OK, "Success"},
    {S_ARG, "Invalid or missing argument"},
    {S_RW, "Filesystem R/W error"},
    {S_EMPTY, "Invalid input file"},
    {S_MISMATCH, "Data or string mismatch error"},
    {S_MEMORY, "Memory error"},
    {S_INPUT_FILE, "Input file error"},
    {S_FILE_SIZE, "Invalid or zero file size"},
    {S_FILE_BUFFER, "Cannot set file buffer ptr"},
    {S_FILE_READ, "Cannot read from file"},
    {-1}
};

static inline const char *get_status(status_e status) {
    const char *rval = NULL;

    if (status == S_ERRNO)
        rval = strerror(errno);
    else {
        for (int i = 0; status_handler[i].status != -1 && !rval; ++i) {
            if (status == status_handler[i].status)
                rval = status_handler[i].status_desc;
        }
    }
    if (!rval)
        rval = "Unknwon error";
    return rval;
}

static inline void *CALLOC(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        fprintf(stderr, "%s\n", get_status(S_MEMORY));
        abort();
    }
    return ptr;
}


static inline void _mem_free(int argc, ...) {
    va_list ap;
    va_start(ap, argc);
    for (int i = 0; i < argc; ++i) {
        void *p = va_arg(ap, void*);
        FREE(p);
    }
    va_end(ap);
}
#define mem_free(...) _mem_free(PP_NARG(__VA_ARGS__), __VA_ARGS__)

static inline char *_strconcat(int argc, ...)
{
    va_list ap;
    char *result = (char*)CALLOC(1, PATH_MAX+1);

    va_start(ap, argc);
    int pos = 0, max = PATH_MAX;
    for (int i = 0; i < argc && max > 0; ++i) {
        char *arg = va_arg(ap, char *);
        pos += snprintf(&result[pos], max, "%s", arg);
        max -= pos;
    }
    va_end(ap);

    return result;
}
#define str_concat(...) _strconcat(PP_NARG(__VA_ARGS__), __VA_ARGS__)

// Statistics displayed
// at the end of execution
typedef struct {
    int changed;
    size_t bytes;
} stats_t;

/*
 * Find and store the relevant
 * data and offsets
 * Returns 0 success, 1 error
 */
int exif_scanmeta(struct exif *entry, uint16_t tag, uint16_t format, uint32_t nentries,
        uint32_t chunk, uint32_t pos, bool bin);

/*
 * Wipeout baseline data
 * Returns 0 success, 1 error
 */
status_e exif_ifddata(bool verbose, bool bin);

/*
 * Wipeout sibifd data
 * Return number of bytes written
 */
status_e exif_subifdata(bool verbose, bool bin);

/*
 * End-user statistics
 */
stats_t *get_written_stats(void);

/*
 * Returns a buffer chunk from given position and size
 */
struct exif *get_chunk(uint32_t pos, size_t len);

/**
 * Useful setters and getters
 */
uint16_t exif_setbyteorder(uint16_t val);
uint16_t exif_getbyteorder(void);
long exif_setfilesize(long val);
long exif_getfilesize(void);
long exif_setheaderoffset(long val);
long exif_getheaderoffset(void);
unsigned char *exif_setbufferptr(long size);
unsigned char *exif_getbufferptr(void);

uint16_t data16(unsigned char *chunk);
uint32_t data32(unsigned char *chunk);

/**
 * format:
 *   1 unsigned byte, 2 ascii string, 3 unsigned short,
 *   4 unsigned long, 5 unsigned rational, 6 signed byte,
 *   7 undefined, 8 signed short, 9 signed long,
 *   10 signed rational, 11 signed float, 12 double float
 */
typedef struct {
    uint16_t format;
    size_t bytes; /* number of bytes per unit */
} format_t;

static const format_t data_format[] = {
    {1, 1},{2, 1},{3, 2},{4, 4},{5, 8},{6, 1},{7, 1},
    {8, 2},{9, 4},{10, 8},{11, 4},{12, 8},{0, -1}
};

// Exif tags
typedef struct {
    const char *desc;
    uint16_t tag;
} tags_t;

// https://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
static const tags_t tags[] = {
    // Baseline TIFF tags
    {"SubfileType",                 0x00fe},
    {"OldSubfileType",              0x00ff},
    {"ImageWidth",                  0x0100},
    {"ImageLength",                 0x0101},
    {"BitsPerSample",               0x0102},
    {"Compression",                 0x0103},
    {"PhotometricInterpretation",   0x0106},
    {"Thresholding",                0x0107},
    {"CellWidth",                   0x0108},
    {"CellLength",                  0x0109},
    {"FillOrder",                   0x010a},
    {"ImageDescription",            0x010e},
    {"Make",                        0x010f},
    {"Model",                       0x0110},
    {"SamplesPerPixel",             0x0115},
    {"RowsPerStrip",                0x0116},
    {"MinSampleValue",              0x0118},
    {"MaxSampleValue",              0x0119},
    {"XResolution",                 0x011a},
    {"YResolution",                 0x011b},
    {"PlannarConfiguration",        0x011c},
    {"GrayResponseUnit",            0x0122},
    {"ResolutionUnit",              0x0128}, /* 1=none, 2=inch, 3=cm */
    {"Software",                    0x0131},
    {"ModifyDate",                  0x0132},
    {"Artist",                      0x013b},
    {"HostComputer",                0x013c},
    {"Copyright",                   0x8298},
    // Extension tags
    {"YCbCrCoefficients",           0x0211},
    {"YCbCrSubSampling",            0x0212},
    {"YCbCrPositioning",            0x0213},
    {"ReferenceBlackWhite",         0x0214},
    // Private IFD tags [GPS]
    {"GPSIFD",                      0x8825},
    // Private IFD tags
    {"ExifIFD",                     0x8769},
    {"GPSVersionID",                0x0000},
    {"GPSLatitudeRef",              0x0001},
    {"GPSLatitude",                 0x0002},
    {"GPSLongitudeRef",             0x0003},
    {"GPSLongitude",                0x0004},
    {"GPSAltitudeRef",              0x0005},
    {"GPSAltitude",                 0x0006},
    {"GPSGPSTimeStamp",             0x0007},
    {"GPSSatellites",               0x0008},
    {"GPSStatus",                   0x0009},
    {"GPSMeasureMode",              0x000a},
    {"GPSSDOP",                     0x000b},
    {"GPSSpeedRef",                 0x000c},
    {"GPSSpeed",                    0x000d},
    {"GPSTrackRef",                 0x000e},
    {"GPSTrack",                    0x000f},
    {"GPSImgDirectionRef",          0x0010},
    {"GPSImgDirection",             0x0011},
    {"GPSMapDatum",                 0x0012},
    {"GPSDestLatitudeRef",          0x0013},
    {"GPSDestLatitude",             0x0014},
    {"GPSDestLongitudeRef",         0x0015},
    {"GPSDestLongitude",            0x0016},
    {"GPSDestBearingRef",           0x0017},
    {"GPSDestBearing",              0x0018},
    {"GPSDestDistanceRef",          0x0019},
    {"GPSDestDistance",             0x001a},
    {"GPSProcessingMethod",         0x001b},
    {"GPSAreaInformation",          0x001c},
    {"GPSDateStamp",                0x001d},
    {"GPSDifferential",             0x001e},
    // Private IFD tags [EXIF]
    {"ExposureTime",                0x829a},
    {"FNumber",                     0x829d},
    {"ExposureProgram",             0x8822},
    {"SpectralSensivity",           0x8824},
    {"ISOSpeedRatings",             0x8827},
    {"ExifVersion",                 0x9000},
    {"DateTimeOriginal",            0x9003},
    {"DateTimeDigitized",           0x9004},
    {"ComponentsConfiguration",     0x9101},
    {"CompressedBitsPerPixel",      0x9102},
    {"ShutterSpeedValue",           0x9201},
    {"ApertureValue",               0x9202},
    {"BrightnessValue",             0x9203},
    {"ExposureBiasValue",           0x9204},
    {"MaxApertureValue",            0x9205},
    {"SubjectDistance",             0x9206},
    {"MeteringMode",                0x9207},
    {"LightSource",                 0x9208},
    {"Flash",                       0x9209},
    {"FocalLength",                 0x920a},
    {"SubjectArea",                 0x9214},
    {"MakerNote",                   0x927c},
    {"UserComment",                 0x9286},
    {"SubsecTime",                  0x9290},
    {"SubsecTimeOriginal",          0x9291},
    {"SubSecTimeDigitized",         0x9292},
    {"FlashpixVersoin",             0xa000},
    {"ColourSpace",                 0xa001},
    {"PixelXDimension",             0xa002},
    {"PixelYDimension",             0xa003},
    {"RelatedSoundFile",            0xa004},
    {"FlashEnergy",                 0xa20b},
    {"FocalPlaneXResolution",       0xa20e},
    {"FocalPlaneYResolution",       0xa20f},
    {"FocalPlaneResolutionUnit",    0xa210},
    {"SubjectLocation",             0xa214},
    {"ExposureIndex",               0xa215},
    {"SensingMethod",               0xa217},
    {"FileSource",                  0xa300},
    {"SceneType",                   0xa301},
    {"CFAPattern",                  0xa302},
    {"ExposureMode",                0xa402},
    {"WhiteBalance",                0xa403},
    {"DigitalZoomRatio",            0xa404},
    {"FocalLengthIn35mmFilm",       0xa405},
    {"SceneCaptureType",            0xa406},
    {"GainControl",                 0xa407},
    {"Contrast",                    0xa408},
    {"Saturation",                  0xa409},
    {"Sharpness",                   0xa40a},
    {"SubjectDistanceRange",        0xa40c},
    {"ImageUniqueID",               0xa420},
    {"CustomRendered",              0xa401},
    {NULL}
};

// Main structure for data chunks
#define DATA 1
#define SECTION 2
struct exif{
    bool freedesc;
    uint16_t tag; //
    uint32_t off;
    int format;
    int group; //
    size_t len; //

    // for _wipe
    size_t wlen; //
    size_t wpos; //

    // used to structure the bins in filesystem
    const char *desc; //
    unsigned char *chunk;
    const tags_t *tags;
    const format_t *fmt;
};
struct exif epriv[IFD0LIST_MAX];


struct sct{
    int round;
    struct exif sct_chunk;
    struct exif dt_chunk;
    struct sct *next;
};
struct sct *head, *shead;

#endif //BUFFER_H
