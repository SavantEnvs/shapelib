/* Input: uint32 LE shx_len | .shx bytes | .shp bytes. Exercises SHPOpenLL, SHPReadObject and the in-memory spatial tree. */
#include <stdint.h>
#include <stdio.h>

#include "memio.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4)
        return -1;
    uint32_t shx_len = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) |
                       ((uint32_t)data[3] << 24);
    if (shx_len > size - 4)
        return -1;

    MemFiles mf = {{{".shx", data + 4, shx_len}, {".shp", data + 4 + shx_len, size - 4 - shx_len}}, 2};
    SAHooks hooks;
    mem_hooks(&hooks, &mf);

    SHPHandle h = SHPOpenLL("m.shp", "rb", &hooks);
    if (!h)
        return -1;

    int nEntities = 0, nType = 0;
    double dMin[4], dMax[4];
    SHPGetInfo(h, &nEntities, &nType, dMin, dMax);

    for (int i = -1; i <= nEntities; i++)
    {
        if (i == nEntities / 2)
            SHPSetFastModeReadObject(h, 1);
        SHPObject *o = SHPReadObject(h, i);
        if (o)
        {
            SHPRewindObject(h, o);
            SHPDestroyObject(o);
        }
    }

    SHPTree *t = SHPCreateTree(h, 2, 0, NULL, NULL);
    if (t)
    {
        int nFound = 0;
        double bMin[4] = {-1e9, -1e9, 0, 0}, bMax[4] = {1e9, 1e9, 0, 0};
        int *ids = SHPTreeFindLikelyShapes(t, bMin, bMax, &nFound);
        free(ids);
        SHPDestroyTree(t);
    }

    SHPClose(h);
    return 0;
}
