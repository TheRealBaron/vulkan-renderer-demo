#include "utils/Time.hpp"

void Time::setup_start() {
    start = std::chrono::high_resolution_clock::now();
}

float Time::get() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> sec = now - start;
    return sec.count();
}

