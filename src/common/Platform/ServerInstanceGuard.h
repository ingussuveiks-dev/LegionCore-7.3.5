/*
 * Copyright (C) 2008-2018 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef SERVER_INSTANCE_GUARD_H
#define SERVER_INSTANCE_GUARD_H

#ifdef _WIN32
#include <Windows.h>

// Keeping the handle open makes creation of the same named mutex in another
// process report ERROR_ALREADY_EXISTS. Windows closes it after a crash, too.
class ServerInstanceGuard
{
public:
    explicit ServerInstanceGuard(wchar_t const* name)
        : _handle(CreateMutexW(nullptr, FALSE, name)), _lastError(GetLastError()) { }

    ~ServerInstanceGuard()
    {
        if (_handle)
            CloseHandle(_handle);
    }

    ServerInstanceGuard(ServerInstanceGuard const&) = delete;
    ServerInstanceGuard& operator=(ServerInstanceGuard const&) = delete;

    bool IsFirstInstance() const { return _handle && _lastError != ERROR_ALREADY_EXISTS; }
    DWORD GetError() const { return _lastError; }

private:
    HANDLE _handle;
    DWORD _lastError;
};
#endif

#endif
