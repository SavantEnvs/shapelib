/* In-memory SAHooks: serves fuzzer-provided buffers to shapelib so harnesses never touch the filesystem. */
#ifndef MAYHEM_MEMIO_H
#define MAYHEM_MEMIO_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "shapefil.h"

#define MEMIO_MAX_SLOTS 4

typedef struct
{
    const char *ext;       /* e.g. ".shp" (matched case-insensitively) */
    const uint8_t *data;
    size_t len;
} MemSlot;

typedef struct
{
    MemSlot slots[MEMIO_MAX_SLOTS];
    int nslots;
} MemFiles;

typedef struct
{
    const uint8_t *data;
    size_t len;
    size_t pos;
} MemFile;

static SAFile mem_open(const char *filename, const char *access, void *ud)
{
    MemFiles *mf = (MemFiles *)ud;
    size_t n = strlen(filename);
    if (strcmp(access, "rb") != 0 && strcmp(access, "r") != 0)
        return NULL;
    for (int i = 0; i < mf->nslots; i++)
    {
        size_t e = strlen(mf->slots[i].ext);
        if (n >= e && strcasecmp(filename + n - e, mf->slots[i].ext) == 0)
        {
            MemFile *f = (MemFile *)calloc(1, sizeof(MemFile));
            if (!f)
                return NULL;
            f->data = mf->slots[i].data;
            f->len = mf->slots[i].len;
            return (SAFile)f;
        }
    }
    return NULL;
}

static SAOffset mem_read(void *p, SAOffset size, SAOffset nmemb, SAFile file)
{
    MemFile *f = (MemFile *)file;
    if (size == 0 || nmemb == 0 || f->pos >= f->len)
        return 0;
    size_t avail = f->len - f->pos;
    size_t items = avail / size;
    if (items > nmemb)
        items = nmemb;
    memcpy(p, f->data + f->pos, items * size);
    f->pos += items * size;
    return items;
}

static SAOffset mem_write(const void *p, SAOffset size, SAOffset nmemb, SAFile file)
{
    (void)p; (void)size; (void)nmemb; (void)file;
    return 0;
}

static SAOffset mem_seek(SAFile file, SAOffset offset, int whence)
{
    MemFile *f = (MemFile *)file;
    int64_t base;
    switch (whence)
    {
    case SEEK_SET: base = 0; break;
    case SEEK_CUR: base = (int64_t)f->pos; break;
    case SEEK_END: base = (int64_t)f->len; break;
    default: return (SAOffset)-1;
    }
    int64_t np = base + (int64_t)offset;
    if (np < 0)
        return (SAOffset)-1;
    f->pos = (size_t)np;
    return 0;
}

static SAOffset mem_tell(SAFile file) { return (SAOffset)((MemFile *)file)->pos; }
static int mem_flush(SAFile file) { (void)file; return 0; }
static int mem_close(SAFile file) { free(file); return 0; }
static int mem_remove(const char *filename, void *ud) { (void)filename; (void)ud; return -1; }
static void mem_error(const char *message) { (void)message; }

static void mem_hooks(SAHooks *h, MemFiles *mf)
{
    h->FOpen = mem_open;
    h->FRead = mem_read;
    h->FWrite = mem_write;
    h->FSeek = mem_seek;
    h->FTell = mem_tell;
    h->FFlush = mem_flush;
    h->FClose = mem_close;
    h->Remove = mem_remove;
    h->Error = mem_error;
    h->Atof = atof;
    h->pvUserData = mf;
}

#endif
