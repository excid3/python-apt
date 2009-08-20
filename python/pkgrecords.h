#include <apt-pkg/pkgrecords.h>

struct PkgRecordsStruct
{
   pkgRecords Records;
   pkgRecords::Parser *Last;
   
   PkgRecordsStruct(pkgCache *Cache) : Records(*Cache), Last(0) {};
   PkgRecordsStruct() : Records(*(pkgCache *)0) {abort();};  // G++ Bug..
};
