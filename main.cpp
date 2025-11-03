// main.cpp
// C++20 required
// Bounded concurrent queue. 
// Optional logging; Blocking & nonBlocking API
// Build: g++ -std=c++20 -pthread -O2 main.cpp -o concurrent_queue
// Enable logging: add -DENABLE_LOGGING (logs to log.txt file)

// standard utilities & containers
#include <cstdlib> 
#include <vector>
#include <string>

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

