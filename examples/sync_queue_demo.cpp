#include <chrono>
#include <ntd/sync_queue.hpp>
#include <print>
#include <thread>

using namespace std::chrono_literals;

ntd::sync_queue<int> queue(50);

void producer()
{
    for (int i = 1; i < 5; ++i)
    {
        queue.push(i);
    }
}

void consumer()
{
    std::this_thread::sleep_for(2s);
    int x;
    auto const r = queue.try_pop(x);
    if (r)
    {
        std::println("[CONSUMER] -> got the value: {}", x);
    };
}
int main()
{
    std::jthread worker(producer);
    std::jthread consumer1(consumer);
    std::jthread consumer2(consumer);
    std::jthread consumer3(consumer);
    std::jthread consumer4(consumer);
    std::jthread consumer5(consumer);
}
