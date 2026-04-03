#pragma once

#include <ntd/config.hpp>
#include <string>
#include <string_view>

#if defined(NTD_OS_WINDOWS)
#include <windows.h>
#elif defined(NTD_OS_LINUX)
#include <sys/prctl.h>
#else
#error "Platform is not supported"
#endif

namespace ntd
{
inline void set_current_thread_name(std::string_view name)
{
#if defined(NTD_OS_WINDOWS)
    std::wstring wname(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), wname.c_str());
#elif defined(NTD_OS_LINUX)
    prctl(PR_SET_NAME, name.substr(0, 15).data(), 0, 0, 0);
#endif
}

inline std::string get_current_thread_name()
{
#if defined(NTD_OS_WINDOWS)
    PWSTR wname_ptr = nullptr;
    HRESULT hr = GetThreadDescription(GetCurrentThread(), &wname_ptr);
    if (SUCCEEDED(hr) && wname_ptr != nullptr)
    {
        std::wstring wname(wname_ptr);
        LocalFree(wname_ptr);
        return std::string(wname.begin(), wname.end());
    }
    return "Unkown_thread";
#elif defined(NTD_OS_LINUX)
    char name_buffer[16]{};
    if (prctl(PR_GET_NAME, name_buffer, 0, 0, 0) == 0)
    {
        return std::string(name_buffer);
    }
    return "Unknown_thread";
#endif
}
} // namespace ntd
