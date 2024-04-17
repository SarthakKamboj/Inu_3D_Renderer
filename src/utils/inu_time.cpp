#include "inu_time.h"

void start_timer(inu_timer_t& timer) {
    timer.running = true;
    auto start = std::chrono::high_resolution_clock::now();
    timer.start_time = std::chrono::duration_cast<inu_time_t>(start.time_since_epoch());
}

void end_timer(inu_timer_t& timer) {
    timer.running = false;
    auto end = std::chrono::high_resolution_clock::now();
    timer.end_time = std::chrono::duration_cast<inu_time_t>(end.time_since_epoch());
    inu_time_t elapsed = timer.end_time - timer.start_time;
    timer.elapsed_time_sec = elapsed.count();
}

void create_debug_timer(const char* msg, inu_timer_t& debug_timer) {
    memcpy(debug_timer.msg, msg, strlen(msg));
    start_timer(debug_timer);
}

inu_timer_t::~inu_timer_t() {
    end_timer(*this);
    if (msg[0] != 0) {
        printf("elapsed time was %lfs. %s\n", this->elapsed_time_sec, this->msg);
    }
}
