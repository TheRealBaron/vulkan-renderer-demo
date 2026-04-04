#pragma once
#include <chrono>

class Time {
public:
    void setup_start();
    float get();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

