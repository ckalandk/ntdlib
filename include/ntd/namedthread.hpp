#pragma once

#include "ntd/detail/thread_utils.hpp"
#include <concepts>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <ntd/safe_mutex.hpp>
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

enum class launch : std::uint8_t
{
    immediate,
    deferred
};

class NamedThreadImpl
{
public:
    using native_handle_type = std::jthread::native_handle_type;

    NamedThreadImpl()
      : _state(std::make_shared<SharedState>())
    {
    }

    explicit NamedThreadImpl(std::string_view name)
      : _m_name(name),
        _state(std::make_shared<SharedState>())
    {
    }

    ~NamedThreadImpl() = default;

    NamedThreadImpl(NamedThreadImpl &&) noexcept = default;
    NamedThreadImpl &operator=(NamedThreadImpl &&) noexcept = default;

    // Delete copy constructors
    NamedThreadImpl(const NamedThreadImpl &) = delete;
    NamedThreadImpl &operator=(const NamedThreadImpl &) = delete;

    template <typename Fn, typename... Args>
    void start(ntd::launch policy, Fn &&func, Args &&...args)
    {
        _state->is_started = (policy == ntd::launch::immediate);
        _thread = std::jthread(
            [state = _state, thread_name = _m_name]<typename CFn, typename... CArgs>(
                std::stop_token stoken, CFn &&captured_fn, CArgs &&...captured_args)
            {
                ntd::this_thread::set_name(thread_name);
                {
                    std::stop_callback stop_cb(stoken,
                                               [state]()
                                               {
                                                   std::scoped_lock lock(state->mtx);
                                                   state->cv.notify_all();
                                               });
                    std::unique_lock lock(state->mtx);
                    state->cv.wait(
                        lock, [&state, &stoken]
                        { return state->is_started || stoken.stop_requested(); });
                }

                if (stoken.stop_requested())
                {
                    return;
                }

                if constexpr (std::is_invocable_v<CFn, std::stop_token, CArgs...>)
                {
                    std::invoke(std::forward<CFn>(captured_fn), stoken,
                                std::forward<CArgs>(captured_args)...);
                }
                else
                {
                    std::invoke(std::forward<CFn>(captured_fn),
                                std::forward<CArgs>(captured_args)...);
                }
            },
            std::forward<Fn>(func), std::forward<Args>(args)...);
    }

    void join()
    {
        _thread.join();
    }

    [[nodiscard]]
    bool joinable() const noexcept
    {
        return _thread.joinable();
    }

    void detach()
    {
        _thread.detach();
    }

    [[nodiscard]]
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

    [[nodiscard]]
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
    std::string _m_name;
    std::jthread _thread;

protected:
    struct SharedState
    {
        std::mutex mtx;
        std::condition_variable cv;
        bool is_started = false;
    };
    std::shared_ptr<SharedState> _state;
};

class NamedThread : private NamedThreadImpl
{
    using Base = NamedThreadImpl;

public:
    using native_handle_type = Base::native_handle_type;

    NamedThread() = default;
    explicit NamedThread(std::string_view name)
      : Base(name)
    {
    }
    template <typename Fn, typename... Args>
        requires(
            std::invocable<std::decay_t<Fn>, std::decay_t<Args>...> ||
            std::invocable<std::decay_t<Fn>, std::stop_token, std::decay_t<Args>...>)
    NamedThread(std::string_view name, Fn &&func, Args &&...args)
      : Base(name)
    {
        this->start(ntd::launch::immediate, std::forward<Fn>(func),
                    std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
        requires(
            std::invocable<std::decay_t<Fn>, std::decay_t<Args>...> ||
            std::invocable<std::decay_t<Fn>, std::stop_token, std::decay_t<Args>...>)
    NamedThread(ntd::launch policy, std::string_view name, Fn &&func, Args &&...args)
      : NamedThread(name)
    {
        this->start(policy, std::forward<Fn>(func), std::forward<Args>(args)...);
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

    void run()
    {
        if (!_state)
        {
            return;
        }
        {
            std::scoped_lock lock(_state->mtx);
            if (_state->is_started)
            {
                return;
            }
            _state->is_started = true;
        }
        _state->cv.notify_all();
    }

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
