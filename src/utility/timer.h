#pragma once

#include <ostream>
#include <string>
#include <chrono>

class Timer {
public:
    void Start() {
        m_start = std::chrono::system_clock::now();
    }
    void Stop() {
        m_stop = std::chrono::system_clock::now();
    }

    std::string ToString() const {
        int h = std::chrono::duration_cast<std::chrono::hours>(m_stop - m_start).count();
        int m = std::chrono::duration_cast<std::chrono::minutes>(m_stop - m_start).count();
        int s = std::chrono::duration_cast<std::chrono::seconds>(m_stop - m_start).count();
        int ms = std::chrono::duration_cast<std::chrono::microseconds>(m_stop - m_start).count();
        return std::to_string(h) + "h" + std::to_string(m % 60) + "m" +
            std::to_string(s % 60) + "s" + std::to_string(ms % 1000) + "ms";
    }

    friend std::ostream& operator << (std::ostream& os, const Timer& timer) {

        os << timer.ToString();
    }
private:
    std::chrono::system_clock::time_point m_start, m_stop;
};