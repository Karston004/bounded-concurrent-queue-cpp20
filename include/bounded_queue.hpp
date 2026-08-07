// bounded_queue.hpp
// C++20 required
// Bounded concurrent queue. 
// Optional logging; Blocking & nonBlocking API
// Enable logging: add -DENABLE_LOGGING at compile time for thread-safe logging
// Header-only library component.
// Intended to be included by client code; see src/main.cpp for a usage demonstration.
// =============================
#pragma once


// standard utilities & containers
#include <string>
#include <climits>
#include <stdexcept>

// concurrency primitives
#include <mutex> 
#include <semaphore>

// I/O and logging
#include <iostream> 
#include <fstream>


namespace bcq {
    // =============================
    // Toggleable Thread-safe logger
    // preprocessor
    // =============================
    namespace logging {
    #ifdef ENABLE_LOGGING
        inline std::mutex g_logMutex;
        inline std::ofstream g_out;
        
        inline void init(const std::string& path = "log.txt") {
            std::scoped_lock lock(g_logMutex);
            if (!g_out.is_open()){
                g_out.open(path, std::ios::out | std::ios::trunc); //open file at path, open it and truncate it
            }
        }

        inline void log(const std::string& msg) {
            std::scoped_lock lock(g_logMutex);
            if (g_out.is_open()) {
                g_out << msg << '\n';
                g_out.flush();
            } else {
                std::cerr << "Logging Error: log() called without successful init(). Log: " << msg << "\n";
            }
        }

        inline void shutdown() {
            std::scoped_lock lock(g_logMutex);
            if (g_out.is_open()) g_out.close();
        }
    #else
        inline void init(const std::string& = "log.txt") {}
        inline void log(const std::string&) {}
        inline void shutdown() {}
    #endif
    }


    // ======================================
    // Bounded multi-producer/multi-consumer queue
    // ======================================

    template <typename T>
    class BoundedQueue {
    private: 
        struct Node {
            T value;
            Node* next;
        };


        Node* head = nullptr;
        Node* tail = nullptr;
        std::counting_semaphore<> addable_;
        std::counting_semaphore<> removable_;
        std::mutex headLock_;
        std::mutex tailLock_;

    public:
        explicit BoundedQueue(int capacity) : addable_(capacity > 0 ? capacity: throw std::invalid_argument("capacity must be > 0")), removable_(0) {
            head = nullptr;
            tail = nullptr;
        }

        ~BoundedQueue() {
            std::scoped_lock lock(headLock_,tailLock_);
            while (head) {
                Node* n = head;
                head = head->next;
                delete n;
            }
            tail = nullptr;
        }

        void enqueue (T value){
            Node* newNode = new Node();
            newNode->value = value;
            newNode->next = nullptr;
            addable_.acquire();
            {
                std::scoped_lock lock(tailLock_);
                if (tail==nullptr) head = newNode;
                else tail->next = newNode;
                tail = newNode;
                logging::log("Enqueue");
            }
            removable_.release();
        }

        T dequeue () {
            removable_.acquire();
            T returnValue{}; 
            Node* temp = nullptr;
            {
                std::scoped_lock lock(headLock_, tailLock_);
                returnValue = head->value;
                temp = head;
                if (head->next == nullptr) tail = nullptr;
                head = head->next;
                logging::log("Dequeue");
            }
            delete temp;
            addable_.release();
            return returnValue;
        }

        T peek () {
            T returnValue{};
            removable_.acquire();
            {
                std::scoped_lock lock (headLock_);
                returnValue = head->value;
                logging::log("Peek");
            }
            removable_.release();
            return returnValue;
        }

        bool try_dequeue (T& result) {
            bool success = false;

            if (!removable_.try_acquire()) logging::log ("TryDequeue-Fail");
            else {
                Node*temp = nullptr;
                {
                    std::scoped_lock lock(headLock_, tailLock_);
                    result = head->value;
                    temp = head;
                    if (head->next == nullptr) tail = nullptr;
                    head = head->next;
                    
                    success = true;
                    logging::log("TryDequeue-Success");
                }
                addable_.release();
                delete temp;
            } 
            return success;
        }

        bool try_enqueue (T value) {
            bool success = false;
            if (!addable_.try_acquire()) logging::log ("TryEnqueue-Fail");
            else {
                Node* newNode = new Node();
                newNode->value = value;
                newNode->next = nullptr;

                std::scoped_lock lock(tailLock_);
                if (tail==nullptr) head = newNode;
                else tail->next = newNode;
                tail = newNode;            

                success = true;
                logging::log ("TryEnqueue-Success");
                removable_.release();
            }
            return  success;
        }

        bool try_peek (T& result) {
            bool success = false;
            if (!removable_.try_acquire()) logging::log ("TryPeek-Fail");
            else {
                std::scoped_lock lock(headLock_);
                result = head->value;

                success = true;
                logging::log ("TryPeek-Success");
                removable_.release();
            }
            return success;
        }
    };
} //namespace bcq