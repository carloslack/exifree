#ifndef FS_H
#define FS_H
#include <sys/stat.h>
#include "buffer.h"

#define FCLOSE(x) if(x!=NULL) {\
    fclose(x);\
    x=NULL;\
}
status_e fs_write(char *destdir, unsigned char *buf, long len, long *rv);
status_e fs_get_file_size(FILE *file, long *count);
status_e fs_mkdir(char *dir, bool dryrun);

#endif
