/* Input: the raw .sbn bytes. Exercises SBNOpenDiskTree and SBNSearchDiskTree[Integer]. */
#include <stdint.h>
#include <stdio.h>

#include "memio.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    MemFiles mf = {{{".sbn", data, size}}, 1};
    SAHooks hooks;
    mem_hooks(&hooks, &mf);

    SBNSearchHandle h = SBNOpenDiskTree("m.sbn", &hooks);
    if (!h)
        return -1;

    double wMin[2] = {-1e300, -1e300}, wMax[2] = {1e300, 1e300};
    int n = 0;
    int *ids = SBNSearchDiskTree(h, wMin, wMax, &n);
    SBNSearchFreeIds(ids);

    for (int ix = 0; ix < 3; ix++)
        for (int iy = 0; iy < 3; iy++)
        {
            double bMin[2] = {-180 + 120 * ix, -90 + 60 * iy};
            double bMax[2] = {bMin[0] + 120, bMin[1] + 60};
            n = 0;
            ids = SBNSearchDiskTree(h, bMin, bMax, &n);
            SBNSearchFreeIds(ids);
        }

    n = 0;
    ids = SBNSearchDiskTreeInteger(h, 0, 0, 255, 255, &n);
    SBNSearchFreeIds(ids);
    n = 0;
    ids = SBNSearchDiskTreeInteger(h, 64, 64, 192, 192, &n);
    SBNSearchFreeIds(ids);

    SBNCloseDiskTree(h);
    return 0;
}
