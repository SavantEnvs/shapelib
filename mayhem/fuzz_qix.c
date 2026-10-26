/* Input: the raw .qix bytes. Exercises SHPOpenDiskTree and SHPSearchDiskTreeEx. */
#include <stdint.h>
#include <stdio.h>

#include "memio.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    MemFiles mf = {{{".qix", data, size}}, 1};
    SAHooks hooks;
    mem_hooks(&hooks, &mf);

    SHPTreeDiskHandle h = SHPOpenDiskTree("m.qix", &hooks);
    if (!h)
        return -1;

    double wMin[4] = {-1e300, -1e300, 0, 0}, wMax[4] = {1e300, 1e300, 0, 0};
    int n = 0;
    int *ids = SHPSearchDiskTreeEx(h, wMin, wMax, &n);
    free(ids);

    for (int ix = 0; ix < 3; ix++)
        for (int iy = 0; iy < 3; iy++)
        {
            double bMin[4] = {-180 + 120 * ix, -90 + 60 * iy, 0, 0};
            double bMax[4] = {bMin[0] + 120, bMin[1] + 60, 0, 0};
            n = 0;
            ids = SHPSearchDiskTreeEx(h, bMin, bMax, &n);
            free(ids);
        }

    SHPCloseDiskTree(h);
    return 0;
}
