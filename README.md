# bounded-concurrent-queue-cpp20
A lock-based bounded concurrent queue implemented in C++20. Uses semaphores and fine-grained locking for concurrent access.

**Overview**
This queue supports multiple producers and consumers operating concurrently and supports a maximum bounded capacity of nodes
The class provides:
- Blocking: enqueue/dequeue/peek (Threads wait when the queue is full/empty)
- Non-Blocking: try_enqueue/try_dequeue/try_peek (returns immediately if operation is unavailable)
- Fine-Grained locking: separate head and tail mutexes reducing blocking between producers and consumers
- C++20 synchronisation primitives: std::counting_semaphore and std::mutex
- Optional Thread-Safe logging: toggleable mode to enable logging of all operations

**Design Summary**
The queue has:
  Two Semaphores:
    addable - keeps track of available capacity
    removable - keeps track of available items
  Two mutexes:
    headlock - protects dequeue operations
    taillock - protects enqueue and dequeue operations

A thread attempting to:
  Enqueue - acquires addable, locks the tail, inserts the node, unlocks the tail, then releases removable
  Dequeue - acquires removable, locks the head, removes the node, unlocks the head, then releases addable

**Building and running**
Requirements
  C++20 compatible compiler 
  POSIX threads support (Linux / macOS / WSL / MinGW)

Compile
Normal:
'''bash
g++ -std=c++20 -pthread -Iinclude src/main.cpp -o concurrent_queue
With Logging:
'''bash
g++ -std=c++20 -pthread -Iinclude -DENABLE_LOGGING src/main.cpp -o concurrent_queue

When logging is disabled, all logging calls are compiled out with zero runtime overhead.

