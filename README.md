# bounded-concurrent-queue-cpp20
A lock-based bounded concurrent queue implemented in C++ 20. Uses semaphores and fine-grained locking for concurrent access.

**Overview**
This queue supports multiple producers and consumers operating conccurently and supports a maximum capacity (bounded) of nodes
The class provides:
Blocking: enqueue/dequeue/peek (Threads wait when the queue is full/empty)
Non-Blocking: try_enqeue/try_dequeue/try_peek (returns imedietly if operation is unavilible)
Fine-Grained locking: seperate head and tail mutexes reducing blocking between producers and consumers
C++20 syncronisation primatives: std::counting_semaphore and std::mutex
Opitional Thread-Safe logging: togglable mode to enable logging of all operations

**Design Summary**
The queue has:
  Two Semaphors:
    addable - keeps track of avaible capacity
    removable - keeps tracks avaible items
  Two mutexs:
    headlock - protects dequeue operations
    taillock - protects enqueue operations

A thread attmepting to:
  Enqueue - acquires addable, lcoks the tail, inserts the node, unlocks the tail, then releases removable
  Dequeue - acquires removable, locks the head, inserts the node, unlocks the head, then releases addable

**Building and running**
Requirements
  C++20 compatible compiler 
  POSIX threads support (Linux / macOS / WSL / MinGW

Compile
bash
g++ -std=c++20 -pthread main.cpp -o concurrent_queue
