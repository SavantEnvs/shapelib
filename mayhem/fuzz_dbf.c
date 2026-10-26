/* Input: the raw .dbf bytes. Exercises DBFOpenLL and every attribute reader. */
#include <stdint.h>
#include <stdio.h>

#include "memio.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    MemFiles mf = {{{".dbf", data, size}}, 1};
    SAHooks hooks;
    mem_hooks(&hooks, &mf);

    DBFHandle h = DBFOpenLL("m.dbf", "rb", &hooks);
    if (!h)
        return -1;

    int nFields = DBFGetFieldCount(h);
    int nRecords = DBFGetRecordCount(h);
    if (nRecords > 256)
        nRecords = 256;

    for (int f = 0; f < nFields; f++)
    {
        char name[32];
        int w, d;
        DBFGetFieldInfo(h, f, name, &w, &d);
        DBFGetNativeFieldType(h, f);
        DBFGetFieldIndex(h, name);
    }
    for (int r = 0; r < nRecords; r++)
    {
        DBFIsRecordDeleted(h, r);
        for (int f = 0; f < nFields; f++)
        {
            DBFReadIntegerAttribute(h, r, f);
            DBFReadDoubleAttribute(h, r, f);
            DBFReadStringAttribute(h, r, f);
            DBFReadLogicalAttribute(h, r, f);
            DBFReadDateAttribute(h, r, f);
            DBFIsAttributeNULL(h, r, f);
        }
    }
    DBFReadStringAttribute(h, -1, 0);
    DBFReadStringAttribute(h, 0, -1);
    DBFReadStringAttribute(h, nRecords, nFields);

    DBFClose(h);
    return 0;
}
