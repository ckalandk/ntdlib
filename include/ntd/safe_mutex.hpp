#pragma once

#include <atomic>
#include <format>
#include <mutex>
#include <ntd/config.hpp>
#include <stacktrace>
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
            auto msg = std::format(
                "[FATAL] Double lock detected on thread {}!\nStacktrace:\n{}", curr_id,
                std::stacktrace::current());
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
            auto msg = std::format(
                "[FATAL] Double lock detected on thread {}!\nStacktrace:\n{}", curr_id,
                std::stacktrace::current());
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
