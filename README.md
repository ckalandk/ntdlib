# ntd Library 🛠️

A modern, header-only C++23 utility library designed specifically for
**teaching** and **learning** the internals of thread safety,
synchronization primitives, and modern C++ library architecture.
This obviously a work in progres, many materials will be added in
the near future, depending of the pace of the course.

> [!IMPORTANT]
> **Educational Purpose:** This library is built to demonstrate clean
> API design, C++23 features (like `std::stacktrace` and `std::print`),
> and robust CMake integration. While fully functional, it is intended
> as a reference for students learning concurrent systems.
> Right now classes and functions are no commented, we'll add
> documentations in the future the explain every aspects of the library
---

## ✨ Features

### 🛡️ `ntd::SafeMutex`

A debugging-enhanced mutex wrapper that detects **recursive locking**
(deadlocks) on the same thread.

* **Self-Lock Protection:** Throws a `std::logic_error` if a thread attempts to
lock a mutex it already owns.
* **Integrated Diagnostics:** Automatically captures and prints a full
`std::stacktrace` upon failure, showing exactly where the illegal lock occurred.
* **Compatibility:** Works seamlessly with `std::lock_guard` and `std::unique_lock`.

> NOTE
> The current version of SafeMutex detects only self-deadlocks (recursive locking).
> Support for transitive deadlock detection is planned for a future release.

### 🧵 `ntd::NamedThread`

A high-level wrapper around `std::jthread` that brings human-readable Identity
to your OS threads.

* **Thread Identity:** Assign names to threads
(e.g., "NetworkWorker", "DatabasePool") which are visible in system
debuggers and profilers.
* **Lifecycle Management:** Inherits the RAII "join-on-destruction" behavior of
`std::jthread`.
* **Stop Token Support:** Seamlessly handles cooperative cancellation via
`std::stop_token`.
* **Deferred execution:** Defer execution of the `NamedThread` and run it manually.

### 📥 `ntd::sync_queue<T>`

A thread-safe, producer-consumer blocking queue.

* **Bounded Capacity:** Prevents memory exhaustion by blocking
producers when the queue is full.
* **Multiple Modes:** Supports both blocking (`async_pop`)
and non-blocking (`try_pop`) operations.
* **Graceful Shutdown:** Provides a `close()` mechanism to
safely wake up and terminate waiting consumers.

---

## 🚀 Getting Started

### Prerequisites

* **Compiler:** GCC 13+ or Clang 16+ (Required for C++23 features).
* **Build System:** CMake 3.23+ and Ninja.
* **Dependencies:** [Catch2 v3](https://github.com/catchorg/Catch2)
(automatically managed via CMake `FetchContent`).

### Building and Testing

This project uses **CMake Presets** for a standardized developer workflow.

```bash
# 1. Configure the project (using GCC)
cmake --preset gcc-debug

# 2. Build the library and tests
cmake --build --preset gcc-debug

# 3. Run the test suite
ctest --preset gcc-debug
