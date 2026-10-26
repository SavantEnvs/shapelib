/* Known-answer probe: print what the SBN/DBF/SHP readers parse out of upstream's fixtures. */
#include <stdio.h>

#include "shapefil.h"

int main(int argc, char **argv)
{
    if (argc < 4)
        return 2;

    SBNSearchHandle sbn = SBNOpenDiskTree(argv[1], NULL);
    if (!sbn)
        return 1;
    double bMin[2] = {-1e300, -1e300}, bMax[2] = {1e300, 1e300};
    int n = 0;
    int *ids = SBNSearchDiskTree(sbn, bMin, bMax, &n);
    printf("sbn shapes=%d", n);
    for (int i = 0; i < n; i++)
        printf(" %d", ids[i]);
    printf("\n");
    SBNSearchFreeIds(ids);
    SBNCloseDiskTree(sbn);

    DBFHandle dbf = DBFOpen(argv[2], "rb");
    if (!dbf)
        return 1;
    printf("dbf records=%d fields=%d\n", DBFGetRecordCount(dbf), DBFGetFieldCount(dbf));
    for (int r = 0; r < 2 && r < DBFGetRecordCount(dbf); r++)
        for (int f = 0; f < DBFGetFieldCount(dbf); f++)
            printf("r%d f%d=%s\n", r, f, DBFReadStringAttribute(dbf, r, f));
    DBFClose(dbf);

    SHPHandle shp = SHPOpen(argv[3], "rb");
    if (!shp)
        return 1;
    int ne, nt;
    double mn[4], mx[4];
    SHPGetInfo(shp, &ne, &nt, mn, mx);
    printf("shp entities=%d type=%d x=[%.6f,%.6f] y=[%.6f,%.6f]\n", ne, nt, mn[0], mx[0], mn[1], mx[1]);
    SHPObject *o = SHPReadObject(shp, 0);
    if (o)
    {
        printf("obj0 parts=%d vertices=%d\n", o->nParts, o->nVertices);
        SHPDestroyObject(o);
    }
    SHPClose(shp);
    return 0;
}
