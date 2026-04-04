#pragma once

#include <concepts>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace ntd
{
template <typename Tp>
class sync_queue
{
public:
    explicit sync_queue(size_t capacity = 30) : _max_capacity(capacity)
    {
    }
    sync_queue(const sync_queue &other)
    {
        std::scoped_lock lk(other._mutex);
        _datas = other._datas;
    }

    sync_queue &operator=(const sync_queue &other)
    {
        if (this == &other)
            return *this;
        std::scoped_lock lk(_mutex, other._mutex);
        _datas = other._datas;
        this->_notify();
        return *this;
    }

    sync_queue(sync_queue &&other) noexcept
    {
        std::scoped_lock lk(other._mutex);
        _datas = std::move(other._datas);
    }

    sync_queue &operator=(sync_queue &&other) noexcept
    {
        std::scoped_lock lk(_mutex, other._mutex);
        _datas = std::move(other._datas);
        this->_notify();
        return *this;
    }

    void push(Tp val)
    {
        std::unique_lock lk(_mutex);
        _producer_cv.wait(lk,
                          [this] { return _datas.size() < _max_capacity || _closed; });
        if (_closed)
            return;
        _datas.push(std::move(val));
        _consumer_cv.notify_one();
    }

    template <typename... Args>
        requires std::constructible_from<Tp, Args...>
    void emplace(Args &&...args)
    {
        std::unique_lock lk(_mutex);
        _producer_cv.wait(lk,
                          [this] { return _datas.size() < _max_capacity || _closed; });
        if (_closed)
            return;
        _datas.emplace(std::forward<Args>(args)...);
        _consumer_cv.notify_one();
    }

    bool try_pop(Tp &out)
    {
        std::scoped_lock lk(_mutex);
        if (_datas.empty())
            return false;
        out = std::move(_datas.front());
        _datas.pop();
        return true;
    }

    std::shared_ptr<Tp> try_pop()
    {
        std::scoped_lock lk(_mutex);
        if (_datas.empty())
            return nullptr;
        auto ret = std::make_shared(std::move(_datas.front()));
        _datas.pop();
        return ret;
    }

    bool async_pop(Tp &out)
    {
        std::unique_lock lk(_mutex);
        _consumer_cv.wait(lk, [this] { return !_datas.empty() || _closed.load(); });
        if (_datas.empty() && _closed.load())
            return false;
        out = std::move(_datas.front());
        _datas.pop();
        return true;
    }

    std::shared_ptr<Tp> async_pop()
    {
        std::unique_lock lk(_mutex);
        _consumer_cv.wait(lk, [this] { return !_datas.empty() || _closed.load(); });
        if (_datas.empty() && _closed.load())
            return nullptr;
        auto ret = std::make_shared(std::move(_datas.front()));
        _datas.pop();
        return ret;
    }

    bool empty() const
    {
        std::scoped_lock lk(_mutex);
        return _datas.empty();
    }

    size_t size() const
    {
        std::scoped_lock lk(_mutex);
        return _datas.size();
    }

    void close()
    {
        _closed.store(true);
        _consumer_cv.notify_all();
    }

    bool is_closed() const
    {
        return _closed;
    }

private:
    std::queue<Tp> _datas;
    mutable std::mutex _mutex;
    mutable std::condition_variable _consumer_cv;
    mutable std::condition_variable _producer_cv;
    std::atomic<bool> _closed{false};
    size_t _max_capacity;

private:
    void _notify() const
    {
        auto const new_size = _datas.size();
        if (new_size == 1)
            _consumer_cv.notify_one();
        if (new_size > 1)
            _consumer_cv.notify_all();
    }
};
} // namespace ntd
