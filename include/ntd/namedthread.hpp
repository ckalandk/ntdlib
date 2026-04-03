#pragma once

#include "ntd/detail/thread_utils.hpp"
#include <concepts>
#include <functional>
#include <ntd/detail/thread_utils.hpp>
#include <string_view>
#include <thread>
#include <utility>

namespace ntd
{
namespace this_thread
{
inline std::string name()
{
    return ntd::get_current_thread_name();
}

inline void set_name(std::string_view new_name)
{
    ntd::set_current_thread_name(new_name);
}
} // namespace this_thread

class NamedThreadImpl
{
public:
    using native_handle_type = std::jthread::native_handle_type;

public:
    NamedThreadImpl() noexcept = default;

    explicit NamedThreadImpl(std::string_view name) : _m_name(name), _thread{}
    {
    }

    ~NamedThreadImpl() = default;

    NamedThreadImpl(NamedThreadImpl &&) noexcept = default;
    NamedThreadImpl &operator=(NamedThreadImpl &&) noexcept = default;

    // Delete copy constructors
    NamedThreadImpl(const NamedThreadImpl &) = delete;
    NamedThreadImpl &operator=(const NamedThreadImpl &) = delete;

    template <typename Fn, typename... Args>
    void start(Fn &&fn, Args &&...args)
    {
        _thread = std::jthread(
            [thread_name = _m_name]<typename CFn, typename... CArgs>(
                std::stop_token st, CFn &&captured_fn, CArgs &&...captured_args)
            {
                this_thread::set_name(thread_name);
                if constexpr (std::is_invocable_v<CFn, std::stop_token, CArgs...>)
                {
                    std::invoke(std::forward<CFn>(captured_fn), st,
                                std::forward<CArgs>(captured_args)...);
                }
                else
                {
                    std::invoke(std::forward<CFn>(captured_fn),
                                std::forward<CArgs>(captured_args)...);
                }
            },
            std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    void join()
    {
        _thread.join();
    }

    bool joinable() const noexcept
    {
        return _thread.joinable();
    }

    void detach()
    {
        _thread.detach();
    }

    auto get_id() const
    {
        return _thread.get_id();
    }

    native_handle_type native_handle()
    {
        return _thread.native_handle();
    }

    void request_stop() noexcept
    {
        _thread.request_stop();
    }

    std::stop_source get_stop_source() noexcept
    {
        return _thread.get_stop_source();
    }

    std::stop_token get_stop_token() const noexcept
    {
        return _thread.get_stop_token();
    }

    void swap(NamedThreadImpl &other) noexcept
    {
        std::swap(_m_name, other._m_name);
        _thread.swap(other._thread);
    }

    friend void swap(NamedThreadImpl &lhs, NamedThreadImpl &rhs) noexcept
    {
        lhs.swap(rhs);
    }

private:
    std::string _m_name{};
    std::jthread _thread;
};

class NamedThread : private NamedThreadImpl
{
    using Base = NamedThreadImpl;

public:
    using native_handle_type = Base::native_handle_type;

public:
    NamedThread() = default;
    explicit NamedThread(std::string_view name) : Base(name)
    {
    }
    template <typename Fn, typename... Args>
        requires(
            std::invocable<std::decay_t<Fn>, std::decay_t<Args>...> ||
            std::invocable<std::decay_t<Fn>, std::stop_token, std::decay_t<Args>...>)
    NamedThread(std::string_view name, Fn &&fn, Args &&...args) : Base(name)
    {
        this->start(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
    ~NamedThread() = default;
    NamedThread(NamedThread &&) noexcept = default;
    NamedThread &operator=(NamedThread &&) noexcept = default;
    NamedThread(const NamedThread &) = delete;
    NamedThread &operator=(const NamedThread &) = delete;

    using Base::detach;
    using Base::get_id;
    using Base::get_stop_source;
    using Base::get_stop_token;
    using Base::join;
    using Base::joinable;
    using Base::native_handle;
    using Base::request_stop;

    void swap(NamedThread &other) noexcept
    {
        Base::swap(other);
    }

    friend void swap(NamedThread &lhs, NamedThread &rhs) noexcept
    {
        lhs.swap(rhs);
    }
};
} // namespace ntd
