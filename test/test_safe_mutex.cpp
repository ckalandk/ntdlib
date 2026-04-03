// test/test_safe_mutex.cpp
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <mutex>
#include <ntd/safe_mutex.hpp>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("SafeMutex standard lock and unlock", "[safe_mutex]")
{
    ntd::SafeMutex mtx;

    // Should lock and unlock normally
    mtx.lock();
    SUCCEED("Mutex locked successfully");
    mtx.unlock();
    SUCCEED("Mutex unlocked successfully");

    // Should work perfectly with standard lock managers
    {
        std::lock_guard<ntd::SafeMutex> lock(mtx);
        SUCCEED("std::lock_guard successfully acquired SafeMutex");
    }
}

TEST_CASE("SafeMutex double lock throws logic_error", "[safe_mutex]")
{
    ntd::SafeMutex mtx;
    mtx.lock();

    // Catch2 macro that expects an exact exception type to be thrown
    REQUIRE_THROWS_AS(mtx.lock(), std::logic_error);

    // Clean up
    mtx.unlock();
}

TEST_CASE("SafeMutex double try_lock throws logic_error", "[safe_mutex]")
{
    ntd::SafeMutex mtx;
    mtx.lock();

    // try_lock should ALSO throw if it's a double lock from the same thread
    REQUIRE_THROWS_AS(mtx.try_lock(), std::logic_error);

    mtx.unlock();
}

TEST_CASE("SafeMutex prevents concurrent access", "[safe_mutex]")
{
    ntd::SafeMutex mtx;
    int shared_counter = 0;

    // Launch multiple threads to increment a counter
    auto worker = [&]()
    {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<ntd::SafeMutex> lock(mtx);
            shared_counter++;
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);

    t1.join();
    t2.join();
    t3.join();

    // If mutual exclusion works, no data races should occur
    REQUIRE(shared_counter == 3000);
}

TEST_CASE("SafeMutex try_lock behavior with multiple threads", "[safe_mutex]")
{
    ntd::SafeMutex mtx;
    std::atomic<bool> lock_acquired{false};
    std::atomic<bool> thread_started{false};

    // Thread 1 holds the lock for a little bit
    std::thread holder(
        [&]()
        {
            mtx.lock();
            thread_started = true;
            std::this_thread::sleep_for(100ms);
            mtx.unlock();
        });

    // Wait for the background thread to actually grab the lock
    while (!thread_started)
    {
        std::this_thread::yield();
    }

    // Main thread tries to lock it. It shouldn't throw, but it should return false
    // because it's held by a DIFFERENT thread.
    REQUIRE(mtx.try_lock() == false);

    holder.join();

    // Now it should be free
    REQUIRE(mtx.try_lock() == true);
    mtx.unlock();
}
