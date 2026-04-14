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
    explicit sync_queue(size_t maxsize)
      : _max_size(maxsize)
    {
    }

    sync_queue(const sync_queue &other)
    {
        std::scoped_lock lock(other._mutex);
        _datas = other._datas;
    }

    sync_queue &operator=(const sync_queue &other)
    {
        if (this == &other)
        {
            return *this;
        }
        size_t new_size = 0;
        {
            std::scoped_lock lock(_mutex, other._mutex);
            _datas = other._datas;
            new_size = _datas.size();
        }
        this->_notify(new_size);
        return *this;
    }

    sync_queue(sync_queue &&other) noexcept
    {
        std::scoped_lock lock(other._mutex);
        _datas = std::move(other._datas);
    }

    sync_queue &operator=(sync_queue &&other) noexcept
    {
        std::scoped_lock lock(_mutex, other._mutex);
        _datas = std::move(other._datas);
        this->_notify(_datas.size());
        return *this;
    }

    void push(Tp val)
    {
        std::unique_lock lock(_mutex);
        _producer_cv.wait(lock,
                          [this] { return _datas.size() < _max_size || _closed; });
        if (_closed)
        {
            return;
        }
        _datas.push(std::move(val));
        _consumer_cv.notify_one();
    }

    template <typename... Args>
        requires std::constructible_from<Tp, Args...>
    void emplace(Args &&...args)
    {
        std::unique_lock lock(_mutex);
        _producer_cv.wait(lock,
                          [this] { return _datas.size() < _max_size || _closed; });
        if (_closed)
        {
            return;
        }
        _datas.emplace(std::forward<Args>(args)...);
        _consumer_cv.notify_one();
    }

    bool try_pop(Tp &out)
    {
        std::scoped_lock lock(_mutex);
        if (_datas.empty())
        {
            return false;
        }
        out = std::move(_datas.front());
        _datas.pop();
        return true;
    }

    std::shared_ptr<Tp> try_pop()
    {
        std::scoped_lock lock(_mutex);
        if (_datas.empty())
        {
            return nullptr;
        }
        auto ret = std::make_shared(std::move(_datas.front()));
        _datas.pop();
        return ret;
    }

    bool async_pop(Tp &out)
    {
        std::unique_lock lock(_mutex);
        _consumer_cv.wait(lock, [this] { return !_datas.empty() || _closed; });
        if (_datas.empty() && _closed)
        {
            return false;
        }
        out = std::move(_datas.front());
        _datas.pop();
        return true;
    }

    std::shared_ptr<Tp> async_pop()
    {
        std::unique_lock lock(_mutex);
        _consumer_cv.wait(lock, [this] { return !_datas.empty() || _closed; });
        if (_datas.empty() && _closed)
        {
            return nullptr;
        }
        auto ret = std::make_shared(std::move(_datas.front()));
        _datas.pop();
        return ret;
    }

    bool empty() const
    {
        std::scoped_lock lock(_mutex);
        return _datas.empty();
    }

    size_t size() const
    {
        std::scoped_lock lock(_mutex);
        return _datas.size();
    }

    void close()
    {
        {
            std::scoped_lock lock(_mutex);
            _closed = true;
        }
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
    bool _closed{false};
    size_t _max_size;

    void _notify(size_t count) const
    {
        if (count == 1)
        {
            _consumer_cv.notify_one();
        }
        if (count > 1)
        {
            _consumer_cv.notify_all();
        }
    }
};
} // namespace ntd
