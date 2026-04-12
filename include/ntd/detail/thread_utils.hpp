#pragma once

#include <ntd/config.hpp>
#include <string>
#include <string_view>

#if defined(NTD_OS_WINDOWS)
#include <windows.h>
#elif defined(NTD_OS_LINUX)
#include <sys/prctl.h>
constexpr size_t LINUX_NAME_MAX_LENGTH = 15;
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
    std::string short_name(name.substr(0, LINUX_NAME_MAX_LENGTH));
    prctl(PR_SET_NAME, short_name.c_str(), 0, 0, 0);
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
    char name_buffer[LINUX_NAME_MAX_LENGTH + 1]{}; // NOLINT
    if (prctl(PR_GET_NAME, name_buffer, 0, 0, 0) == 0)
    {
        return name_buffer;
    }
    return "Unknown_thread";
#endif
}
} // namespace ntd
