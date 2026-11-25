#include <dream/core/job_system.hpp>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    using namespace dream::core;
    JobSystem jobs(2);

    auto sum = jobs.submit([](int left, int right) { return left + right; }, 19, 23);
    require(sum.get() == 42, "submitted result is returned through future");

    auto failure = jobs.submit([]() -> int {
        throw std::runtime_error("expected task failure");
    });
    bool exception_seen = false;
    try {
        (void)failure.get();
    } catch (const std::runtime_error&) {
        exception_seen = true;
    }
    require(exception_seen, "task exception is delivered through future");

    std::atomic<int> completed{0};
    for (int i = 0; i < 32; ++i)
        (void)jobs.submit([&completed] { completed.fetch_add(1); });

    jobs.wait_idle();
    require(completed.load() == 32, "wait_idle waits for queued and active tasks");

    jobs.shutdown();
    require(jobs.is_stopping(), "shutdown enters stopping state");

    bool rejected = false;
    try {
        (void)jobs.submit([] {});
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "new work is rejected after shutdown");

    std::cout << "Dream Engine job system tests passed.\n";
    return EXIT_SUCCESS;
}
