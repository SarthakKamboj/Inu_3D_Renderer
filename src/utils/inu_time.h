#pragma once

#include <chrono>

typedef double time_count_t;
typedef std::chrono::duration<time_count_t, std::ratio<1>> inu_time_t;

/// <summary>
/// A timer to track how much time has elapsed and whether it is running or not
/// </summary>
struct inu_timer_t {
    inu_time_t start_time;
    inu_time_t end_time;
    time_count_t elapsed_time_sec;
    bool running = false;
    char msg[256]{};

    ~inu_timer_t();
};

void create_debug_timer(const char* msg, inu_timer_t& debug_timer);

/// <summary>
/// Start the timer
/// </summary>
/// <param name="timer">Timer to start</param>
void start_timer(inu_timer_t& timer);

/// <summary>
/// End a timer and record its time elapsed
/// </summary>
/// <param name="timer">Timer to end</param>
void end_timer(inu_timer_t& timer);

