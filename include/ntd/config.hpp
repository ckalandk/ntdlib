#pragma once

#include <version>

#if __cplusplus < 202302L && (!defined(_MSVC_LANG) || _MSVC_LANG < 202302L)
#error                                                                                 \
    "This library requires C++23 or higher. Please check your compiler flags (e.g., -std=c++23 or /std:c++23)."
#endif

// --- Compiler Detection ---
#if defined(_MSC_VER)
#define NTD_COMPILER_MSVC _MSC_VER
#elif defined(__clang__)
#define NTD_COMPILER_CLANG                                                             \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#define NTD_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#define NTD_COMPILER_UNKNOWN 1
#endif

// --- Platform Detection ---
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define NTD_OS_WINDOWS 1
#elif defined(__linux__)
#define NTD_OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define NTD_OS_MACOS 1
#elif defined(__unix__)
#define NTD_OS_UNIX 1
#else
#define NTD_OS_UNKNOWN 1
#endif

#if defined(NTD_OS_WINDOWS)
#define NTD_API __declspec(dllexport) // or dllimport
#else
#define NTD_API __attribute__((visibility("default")))
#endif

// 1. Detect if exceptions are enabled
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define NTD_HAS_EXCEPTIONS 1
#else
#define NTD_HAS_EXCEPTIONS 0
#endif

#ifdef __cpp_lib_stacktrace
#define NTD_USE_STACKTRACE 1
#else
#define NTD_USE_STACKTRACE 0
#endif

#if NTD_HAS_EXCEPTIONS
#define NTD_THROW(exception_type, message) throw exception_type(message)
#else
#include <cstdlib>
#include <print>
#define NTD_THROW(exception_type, message)                                             \
    do                                                                                 \
    {                                                                                  \
        std::println(stderr, "NTD Fatal Error: {}", message);                          \
        std::abort();                                                                  \
    } while (0)
#endif
