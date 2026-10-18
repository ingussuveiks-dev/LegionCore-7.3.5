// Private audit copies retain read-only access and permit an already open
// client's write-sharing handles. No production CascLib source is modified.
#include <windows.h>
#include "CascLib.h"
#include "CascCommon.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "CascOpenStorageAudit.inc"
#include "CascReadFileAudit.inc"

int main(int argc,char**argv)
{
    if(argc!=4)return 2;
    HANDLE storage=nullptr,file=nullptr;
    if(!CascOpenStorage(argv[1],CASC_LOCALE_ENUS,&storage))return 3;
    bool ok=CascOpenFile(storage,argv[2],CASC_LOCALE_ENUS,0,&file);
    if(ok)
    {
        DWORD high=0,size=CascGetFileSize(file,&high),read=0;
        ok=!high&&size!=CASC_INVALID_SIZE&&size>0&&size<=16*1024*1024;
        if(ok)
        {
            std::vector<char> bytes(size);
            ok=CascReadFile(file,bytes.data(),size,&read)&&read==size;
            if(ok)
            {
                std::ofstream output(argv[3],std::ios::binary);
                ok=bool(output.write(bytes.data(),bytes.size()));
            }
            std::printf("%s bytes=%lu read=%lu exported=%d\n",argv[2],size,read,ok);
        }
        CascCloseFile(file);
    }
    else std::printf("CASC open error=%lu\n",GetLastError());
    CascCloseStorage(storage);
    return ok?0:1;
}
