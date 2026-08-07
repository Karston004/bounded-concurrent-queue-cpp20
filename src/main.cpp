// main.cpp
// Demonstration program for BoundedQueue<T>
// This file illustrates correct usage under concurrent producer/consumer workloads.
// It is not part of the queue library itself.
// Compile with -DENABLE_LOGGING to enable file-based logging (log.txt).
// =============================

#include "../include/bounded_queue.hpp"
#include <thread>
#include <iostream>

int main() {
    bcq::logging::init("log.txt");

    bcq::BoundedQueue<int> q(5);
    int r;

    std::thread producer([&] {
        for (int i = 0; i < 100; ++i) {
            q.enqueue(i);
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < 100; ++i) {
            int v = q.dequeue();
            std::cout << "Consumed: " << v << "\n";
        }
    });

    
    producer.join();
    if (q.try_peek(r)) std::cout << "Peeked: " << r << "\n";
    else std::cout << "Peek failed (queue empty)\n";
    consumer.join();

    std::cout << "Try Dequeue (Expected: false(0)) | " << q.try_dequeue(r) << "\n";

    bcq::BoundedQueue<int> q2(2);
    std::cout << "Expected: true  (1) | " << q2.try_enqueue(1) << "\n";
    std::cout << "Expected: true  (1) | " << q2.try_enqueue(2) << "\n";
    std::cout << "Expected: false (0) | " << q2.try_enqueue(3) << "\n"; //full


    bcq::logging::shutdown();
}
