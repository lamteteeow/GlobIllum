#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <omp.h>
#include <mutex>

// ----------------------------------------------------------------------
// Timer

typedef std::chrono::time_point<std::chrono::system_clock> timepoint;

struct Timer {
    virtual ~Timer() {}

    virtual void start(const std::string &name);
    virtual void stop(const std::string &name);

    void clear();
    void merge(const Timer& t);

    inline uint64_t get_ns(const std::string& name) const { return times.at(name); }
    inline double get_ms(const std::string &name) const { return get_ns(name) / 1000000.0; }
    inline double get_s(const std::string &name) const { return get_ms(name) / 1000.0; }
    inline double get_m(const std::string &name) const { return get_s(name) / 60.0; };
    inline double get_h(const std::string &name) const { return get_m(name) / 60.0; };

    uint64_t get_ns_total() const;
    inline double get_ms_total() const { return get_ns_total() / 1000000.0; }
    inline double get_s_total() const { return get_ms_total() / 1000.0; }
    inline double get_m_total() const { return get_s_total() / 60.0; }
    inline double get_h_total() const { return get_m_total() / 60.0; }

    inline uint64_t get_count(const std::string& name) const { return counts.at(name); }
    uint64_t get_count_total() const;

    std::string format_time(const std::string& name) const;
    static std::string format_time(double ms_elapsed, double ms_total);
    static std::string format_time(double ms_elapsed, double ms_total, uint64_t count);

    void print(const std::string& timer_name = "");

    // data
    std::map<std::string, timepoint> starts;
    std::map<std::string, uint64_t> times;
    std::map<std::string, uint64_t> counts;
};

// ----------------------------------------------------------------------
// Adapters:

struct OMPTimer {
    OMPTimer() : timers(omp_get_max_threads()) {}
    virtual ~OMPTimer() {}

    virtual void start(const std::string &name) { timers[omp_get_thread_num()].start(name); }

    virtual void stop(const std::string &name) { timers[omp_get_thread_num()].stop(name); }

    void clear() {
        for (auto &t : timers)
            t.clear();
    }

    void print(const std::string &timer_name = "") {
        Timer total;
        for (auto &t : timers)
            total.merge(t);
        total.print(timer_name);
    }

    // data
    std::vector<Timer> timers;
};


struct LockTimer : public Timer {
    virtual ~LockTimer() {}

    void start(const std::string &name) {
        mutex.lock();
        starts[name] = std::chrono::system_clock::now();
    }
    void stop(const std::string &name) {
        times[name] += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - starts[name]).count();
        mutex.unlock();
    }

    void clear() {
        mutex.lock();
        starts.clear();
        times.clear();
        mutex.unlock();
    }

    void merge(const Timer& t) {
        mutex.lock();
        for (auto& curr : t.times)
            times[curr.first] += curr.second;
        mutex.unlock();
    }

    // data
    std::mutex mutex;
};

// ----------------------------------------------------------------------
// Statistics logging

// Makros used for logging the execution times, may be enabled via the STATS flag
#ifdef STATS
    #define STAT(name) StatTimer timer(name);
    #define PRINT_STATS() StatTimer::print();
    #define CLEAR_STATS() StatTimer::clear();
#else
    #define STAT(name) ;
    #define PRINT_STATS() ;
    #define CLEAR_STATS() ;
#endif

/**
 * @brief Log entry using RAII to update an internal timer
 * Is thread-safe when called from multiple OMP threads
 */
class StatTimer {
public:
    /**
     * @brief Start named timing
     *
     * @param name Timing name
     */
    StatTimer(const std::string& name);

    /**
     * @brief Stop named timing
     */
    ~StatTimer();

    /**
     * @brief Print all gathered timings
     */
    static void print();

    /**
     * @brief Clear all gathered timings
     */
    static void clear();

private:
    // data
    std::string name; ///< Timing name
    static OMPTimer timer; ///< Internal timer
};
