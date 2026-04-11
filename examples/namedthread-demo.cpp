#include <chrono> //NOLINT
#include <ntd/namedthread.hpp>
#include <ntd/utility.hpp>
#include <stop_token>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

void basic_task()
{
    std::println("[Basic Task] Running immediately. No stop token needed");
}

void stoppable_task(std::stop_token st, int max_iterations)
{
    std::println("[Stoppable Task] Booted up. Waiting for work...");
    int count = 0;
    while (!st.stop_requested() && count < max_iterations)
    {
        std::println("[Stoppable Task] Working... phase {}", ++count);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (st.stop_requested())
    {
        std::println("[Stoppable Task] Interrupted by stop request! Cleaning up.");
    }
    else
    {
        std::println("[Stoppable Task] Finished naturally.");
    }
}

void deferred_task(std::stop_token st, std::string payload)
{
    // Even deferred tasks can check the stop token to be safe!
    if (st.stop_requested())
        return;

    std::println("[Deferred Task] Un-parked! Processing payload: {}", payload);
}

int main()
{
    std::println("--- MAIN: Starting NamedThread Demo ---");

    // 1. Immediate Launch (Standard jthread behavior)
    ntd::NamedThread t1("BasicWorker", basic_task);

    // 2. Stop Token & Parameters (Standard jthread behavior)
    ntd::NamedThread t2("StoppableWorker", stoppable_task, 100);

    // Let t2 run for a fraction of a second...
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Manually trigger the cancellation signal (Standard jthread feature)
    std::println("--- MAIN: Requesting t2 to stop early ---");
    t2.request_stop();

    // 3. Deferred Launch (Your Custom Feature!)
    std::println("--- MAIN: Creating t3 as DEFERRED (Parked) ---");
    ntd::NamedThread t3(ntd::launch::deferred, "DeferredWorker", deferred_task,
                        "Alpha Protocol");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)); // Prove it isn't running yet

    std::println("--- MAIN: Calling t3.run() to un-park it ---");
    t3.run();

    // 4. Movability (Proving your SharedState architecture works)
    std::println("--- MAIN: Moving threads into a vector ---");
    std::vector<ntd::NamedThread> thread_pool;
    thread_pool.push_back(std::move(t1));
    thread_pool.push_back(std::move(t2));
    thread_pool.push_back(std::move(t3));

    std::println("--- MAIN: End of scope reached. Destructors will auto-join. ---");
    return 0;
}
