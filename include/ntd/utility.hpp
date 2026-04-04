#pragma once

#include <mutex>
#include <print>

namespace ntd
{
namespace
{
std::mutex _stdout_mutex;
}
template <typename... Args>
inline void sync_print(std::format_string<Args...> fmt, Args &&...args)
{
    std::scoped_lock lk(_stdout_mutex);
    std::println(fmt, std::forward<Args>(args)...);
}
} // namespace ntd
