// The generated audit copies preserve read-only access and allow an open client
// to hold write handles. They do not change the production CascLib sources.
#include <windows.h>
#include "CascLib.h"
#include "CascCommon.h"
#include <cstdio>
#include <vector>
#include "CascOpenStorageAudit.inc"
#include "CascReadFileAudit.inc"

int main(int argc, char** argv)
{
    if (argc < 3)
        return 2;
    HANDLE storage = nullptr;
    if (!CascOpenStorage(argv[1], CASC_LOCALE_ENUS, &storage))
    {
        std::printf("Storage error %lu\n", GetLastError());
        return 1;
    }
    bool failed = false;
    for (int i = 2; i < argc; ++i)
    {
        HANDLE file = nullptr;
        if (!CascOpenFile(storage, argv[i], CASC_LOCALE_ENUS, 0, &file))
        {
            std::printf("%s: OPEN ERROR %lu\n", argv[i], GetLastError());
            failed = true;
            continue;
        }
        DWORD high = 0, size = CascGetFileSize(file, &high), read = 0;
        // This probe is for visual assets, not unbounded allocation of archives.
        if (high || size == CASC_INVALID_SIZE || size > 512 * 1024 * 1024)
        {
            std::printf("%s: INVALID OR OVERSIZED FILE\n", argv[i]);
            failed = true;
        }
        else
        {
            std::vector<char> data(size ? size : 1);
            bool ok = CascReadFile(file, data.data(), size, &read);
            std::printf("%s: size=%lu read=%lu ok=%d error=%lu\n", argv[i], size, read, ok, ok ? 0 : GetLastError());
            failed |= !ok || read != size;
        }
        CascCloseFile(file);
    }
    CascCloseStorage(storage);
    return failed ? 1 : 0;
}
