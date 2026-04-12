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
        auto thread_id = _check_for_double_lock();
        _mutex.lock();
        _owner.store(thread_id, std::memory_order_relaxed);
    }

    void unlock()
    {
        _owner.store(std::thread::id(), std::memory_order_relaxed);
        _mutex.unlock();
    }

    bool try_lock()
    {
        auto thread_id = _check_for_double_lock();
        if (_mutex.try_lock())
        {
            _owner.store(thread_id, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

private:
    auto _check_for_double_lock() -> std::thread::id
    {
        auto curr_id = std::this_thread::get_id();
        if (_owner.load(std::memory_order_relaxed) == curr_id)
        {
            std::ostringstream thread_oss;
            thread_oss << curr_id;
            std::ostringstream trace_oss;
            auto trace = std::stacktrace::current();
            trace_oss << trace;
            auto msg = std::format(
                "[FATAL] Double lock detected on thread {}!\nStacktrace:\n{}",
                thread_oss.str(), trace_oss.str());
            NTD_THROW(std::logic_error, msg);
        }
        return curr_id;
    }
};
} // namespace ntd
