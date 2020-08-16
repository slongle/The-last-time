#pragma once

#include <ostream>
#include <string>
#include <chrono>

class Timer {
public:
    void Start() {
        m_start = std::chrono::system_clock::now();
        m_running = true;
    }
    void Stop() {
        m_stop = std::chrono::system_clock::now();
        m_running = false;
    }

    std::string ToString() const {
        auto delta = m_running ? std::chrono::system_clock::now() - m_start :  m_stop - m_start;
        int h = std::chrono::duration_cast<std::chrono::hours>(delta).count();
        int m = std::chrono::duration_cast<std::chrono::minutes>(delta).count();
        int s = std::chrono::duration_cast<std::chrono::seconds>(delta).count();
        int ms = std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
        return std::to_string(h) + "h" + std::to_string(m % 60) + "m" +
            std::to_string(s % 60) + "s" + std::to_string(ms % 1000) + "ms";
    }

    friend std::ostream& operator << (std::ostream& os, const Timer& timer) {

        os << timer.ToString();
    }
private:
    bool m_running = false;
    std::chrono::system_clock::time_point m_start, m_stop;
};