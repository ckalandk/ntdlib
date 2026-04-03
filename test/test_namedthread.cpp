// test/test_named_thread.cpp
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <ntd/namedthread.hpp>
#include <string>

using namespace std::chrono_literals;

TEST_CASE("NamedThread executes and sets thread name", "[named_thread]")
{
    std::atomic<bool> executed{false};
    std::string internal_name;

    // Launch a thread with a name, a lambda, and an argument
    ntd::NamedThread t(
        "TestWorker1",
        [&](int expected_val)
        {
            // Fetch the name from INSIDE the thread
            internal_name = ntd::this_thread::name();

            // Ensure arguments are forwarded correctly
            REQUIRE(expected_val == 42);

            executed = true;
        },
        42);

    REQUIRE(t.joinable());
    t.join();

    REQUIRE(executed == true);
    // Note: OS thread names are often truncated to 15 chars on Linux,
    // so "TestWorker1" is safe.
    REQUIRE(internal_name == "TestWorker1");
}

TEST_CASE("NamedThread properly handles std::stop_token", "[named_thread]")
{
    std::atomic<bool> thread_started{false};
    std::atomic<bool> stopped_gracefully{false};

    // Lambda explicitly takes a stop_token as its first argument
    ntd::NamedThread t("Stoppable",
                       [&](std::stop_token st)
                       {
                           thread_started = true;

                           // Wait until the main thread requests a stop
                           while (!st.stop_requested())
                           {
                               std::this_thread::yield();
                           }

                           stopped_gracefully = true;
                       });

    // Wait for thread to actually spin up
    while (!thread_started)
    {
        std::this_thread::yield();
    }

    // Request stop from the outside
    t.request_stop();
    t.join();

    REQUIRE(stopped_gracefully == true);
}

TEST_CASE("NamedThread move semantics and swap", "[named_thread]")
{
    std::atomic<bool> done{false};

    ntd::NamedThread t1("MoveMe",
                        [&](std::stop_token st)
                        {
                            while (!st.stop_requested())
                            {
                                std::this_thread::yield();
                            }
                            done = true;
                        });

    REQUIRE(t1.joinable());

    // Test Move Constructor
    ntd::NamedThread t2(std::move(t1));
    REQUIRE(!t1.joinable());
    REQUIRE(t2.joinable());

    // Test Move Assignment
    ntd::NamedThread t3;
    REQUIRE(!t3.joinable());

    t3 = std::move(t2);
    REQUIRE(!t2.joinable());
    REQUIRE(t3.joinable());

    // Test Swap
    ntd::NamedThread t4;
    t3.swap(t4);
    REQUIRE(!t3.joinable());
    REQUIRE(t4.joinable());

    // Clean up
    t4.request_stop();
    t4.join();

    REQUIRE(done == true);
}
