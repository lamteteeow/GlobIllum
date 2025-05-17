#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>

class LcTimer {
  public:
    void begin() { t_begin = std::chrono::steady_clock::now(); }

    void end() { t_end = std::chrono::steady_clock::now(); }

    void report() const {
        using std::chrono::duration_cast;
        const uint64_t ms_raw = duration_cast<std::chrono::milliseconds>(t_end - t_begin).count();
        const uint64_t m      = ms_raw / (1000 * 60);
        const uint64_t s      = (ms_raw / 1000) % 60;
        const uint64_t ms     = ms_raw % 1000;

        std::cout << "  duration:\t";
        if (m > 0) std::cout << m << "m, ";
        if (m > 0 || s > 0) std::cout << s << "s, ";
        std::cout << ms << "ms" << std::endl;
    }

  private:
    std::chrono::steady_clock::time_point t_begin;
    std::chrono::steady_clock::time_point t_end;
};

class LcProgressBar {
  public:
    void init(uint32_t total_steps);
    void advance();
    void end();

  private:
    void draw() const;

  private:
    static constexpr char START = '[';
    static constexpr char FILL  = '=';
    static constexpr char LEAD  = '>';
    static constexpr char SPACE = ' ';
    static constexpr char END   = ']';

    static constexpr uint32_t SEGMENTS = 64;

    uint32_t total_steps     = 0;
    uint32_t current_steps   = 0;
    uint32_t current_segment = 0;
};

inline void LcProgressBar::init(const uint32_t total_steps) {
    this->total_steps = total_steps;
    draw();
}

inline void LcProgressBar::advance() {
    if (current_segment >= SEGMENTS) {
        return;
    }

    current_steps += 1;

    if (current_steps < total_steps / SEGMENTS) {
        return;
    }

    current_steps   -= total_steps / SEGMENTS;
    current_segment += 1;

    draw();
}

inline void LcProgressBar::end() {
    current_segment = SEGMENTS;

    std::cout << "\r";

    for (uint32_t i = 0; i < SEGMENTS + 8; i++) {
        std::cout << ' ';
    }

    std::cout << "\r";
    std::cout.flush();
}

inline void LcProgressBar::draw() const {
    std::cout << '\r' << ' ' << START;

    for (uint32_t i = 0; i < current_segment; i++) {
        std::cout << FILL;
    }

    if (current_segment < SEGMENTS) {
        std::cout << LEAD;
    }

    for (uint32_t i = current_segment + 1; i < SEGMENTS; i++) {
        std::cout << SPACE;
    }

    std::cout << END;

    const auto progress = static_cast<float>(current_segment) / SEGMENTS;
    std::cout << ' ' << static_cast<int>(100.f * (progress)) << '%';

    std::cout.flush();
}
