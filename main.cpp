// main.cpp
// C++20 required
// Bounded concurrent queue. 
// Optional logging; Blocking & nonBlocking API
// Build: g++ -std=c++20 -pthread -O2 main.cpp -o concurrent_queue
// Enable logging: add -DENABLE_LOGGING (logs to log.txt file)
// =============================


// standard utilities & containers
#include <cstdlib> 
#include <vector>
#include <string>
#include <climits>

// concurrency primitives
#include <mutex> 
#include <chrono>
#include <thread>
#include <semaphore>

// I/O and logging
#include <iostream> 
#include <fstream>



// =============================
// Toggleable Thread-safe logger
// preprocessor
// =============================
#ifdef ENABLE_LOGGING
namespace logging {
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
}

#define LOG_INIT(path)    logging::init(path)
#define LOG(MSG)          logging::log(MSG)
#define LOG_SHUTDOWN()    logging::shutdown()

#else
#define LOG_INIT(path)    ((void)0)
#define LOG(MSG)          ((void)0)
#define LOG_SHUTDOWN()    ((void)0)
#endif

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
            LOG("Enqueue");
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
            LOG("Dequeue");
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
            LOG("Peek");
        }
        removable_.release();
        return returnValue;
    }

    bool try_dequeue (T& result) {
        bool success = false;

        if (!removable_.try_acquire()) LOG ("TryDequeue-Fail");
        else {
            Node*temp = nullptr;
            {
                std::scoped_lock lock(headLock_, tailLock_);
                result = head->value;
                temp = head;
                if (head->next == nullptr) tail = nullptr;
                head = head->next;
                
                success = true;
                LOG("TryDequeue-Success");
            }
            addable_.release();
            delete temp;
        } 
        return success;
    }

    bool try_enqueue (T value) {
        bool success = false;
        if (!addable_.try_acquire()) LOG ("TryEnqueue-Fail");
        else {
            Node* newNode = new Node();
            newNode->value = value;
            newNode->next = nullptr;

            std::scoped_lock lock(tailLock_);
            if (tail==nullptr) head = newNode;
            else tail->next = newNode;
            tail = newNode;            

            success = true;
            LOG ("TryEnqueue-Success");
            removable_.release();
        }
        return  success;
    }

    bool try_peek (T& result) {
        bool success = false;
        if (!removable_.try_acquire()) LOG ("TryPeek-Fail");
        else {
            std::scoped_lock lock(headLock_);
            result = head->value;

            success = true;
            LOG ("TryPeek-Success");
            removable_.release();
        }
        return success;
    }


};

