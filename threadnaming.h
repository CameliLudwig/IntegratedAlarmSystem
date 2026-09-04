#ifndef THREADNAMING_H
#define THREADNAMING_H

#include <windows.h>

namespace ThreadNaming {

inline void setCurrentThreadName(const wchar_t *name)
{
    typedef HRESULT (WINAPI *SetThreadDescriptionFunction)(HANDLE, PCWSTR);
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32)
        return;

    SetThreadDescriptionFunction setThreadDescription =
        reinterpret_cast<SetThreadDescriptionFunction>(
            GetProcAddress(kernel32, "SetThreadDescription"));
    if (setThreadDescription)
        setThreadDescription(GetCurrentThread(), name);
}

}

#endif