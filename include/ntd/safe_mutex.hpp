#pragma once

#include <atomic>
#include <format>
#include <mutex>
#include <ntd/config.hpp>
#if NTD_USE_STACKTRACE
#include <stacktrace>
#endif
#include <thread>

namespace ntd
{
class SafeMutex
{
    std::mutex _mutex;
    std::atomic<std::thread::id> _owner;

public:
    void lock()
    {
        auto curr_id = std::this_thread::get_id();
        if (_owner.load(std::memory_order_relaxed) == curr_id)
        {
#if NTD_USE_STACKTRACE
            auto stack_trace = std::stacktrace::current();
#else
            auto stack_trace = "Stacktrace is not supported on this compiler";
#endif
            auto msg = std::format(
                "[FATAL] Double lock detected on thread {}!\nStacktrace:\n{}", curr_id,
                stack_trace);
            NTD_THROW(std::logic_error, msg);
        }
        _mutex.lock();
        _owner.store(curr_id, std::memory_order_relaxed);
    }

    void unlock()
    {
        _owner.store(std::thread::id(), std::memory_order_relaxed);
        _mutex.unlock();
    }

    bool try_lock()
    {
        auto curr_id = std::this_thread::get_id();
        if (_owner.load(std::memory_order_relaxed) == curr_id)
        {
#if NTD_USE_STACKTRACE
            auto stack_trace = std::stacktrace::current();
#else
            auto stack_trace = "stacktrace not supported by this compiler";
#endif
            auto msg = std::format(
                "[FATAL] Double lock detected on thread {}!\nStacktrace:\n{}", curr_id,
                stack_trace);
            NTD_THROW(std::logic_error, msg);
        }
        if (_mutex.try_lock())
        {
            _owner.store(curr_id, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
};
} // namespace ntd
