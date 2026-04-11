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
    std::atomic<bool> started{false};
    std::atomic<bool> done{false};

    ntd::NamedThread t1("MoveMe",
                        [&](std::stop_token st)
                        {
                            started = true;
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

    while (!started.load())
    {
        std::this_thread::yield();
    }
    // Clean up
    t4.request_stop();
    t4.join();

    REQUIRE(done == true);
}

#include <atomic>
#include <stdexcept>

TEST_CASE("Deferred thread cleans up safely during exception unwinding",
          "[named_thread]")
{
    std::atomic<bool> lambda_ran{false};

    try
    {
        // 1. Create the deferred thread (it parks itself at the condition variable)
        ntd::NamedThread t(ntd::launch::defered, "DangerThread",
                           [&](std::stop_token st)
                           {
                               lambda_ran = true; // We should NEVER reach this line

                               while (!st.stop_requested())
                               {
                                   std::this_thread::yield();
                               }
                           });

        // 2. Simulate a catastrophic failure BEFORE calling t.run()
        throw std::runtime_error("Simulated crash in main logic!");

        t.run(); // This line is skipped due to the exception
    }
    catch (const std::runtime_error &e)
    {
        // 3. We catch the exception.
        // Because of stack unwinding, 't' went out of scope and its destructor
        // has completely finished running by the time we enter this catch block.
        REQUIRE(std::string(e.what()) == "Simulated crash in main logic!");
    }

    // 4. The Ultimate Verification:
    // If the test reaches this line without hanging, it proves your destructor
    // successfully woke up the parked thread and joined it.
    // If lambda_ran is false, it proves your early-exit optimization worked!
    REQUIRE(lambda_ran == false);
}
