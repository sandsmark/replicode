#ifndef DEBUG_H
#define DEBUG_H

static std::mutex s_debugSection;

/// Thread safe debug output
class DebugStream
{
public:
    inline DebugStream(std::string area) {
        s_debugSection.lock();
        std::cout << "\033[1;34m" << area << "\033[1;37m>\033[1;32m";
    }

    ~DebugStream() {
        std::cout << "\033[0m" << std::endl;
        s_debugSection.unlock();
    }

    inline DebugStream &operator<<(const std::string output) { std::cout << " " << output ; return *this; }
    inline DebugStream &operator<<(const uint64_t output) { std::cout << " " << output; return *this; }
    inline DebugStream &operator<<(const int64_t output) { std::cout << " " << output; return *this; }
    inline DebugStream &operator<<(const char *output) { std::cout << " " << output; return *this; }
    inline DebugStream &operator<<(const void *output) { std::cout << std::hex <<  " 0x" <<  (uint64_t)output << std::dec; return *this; }
    inline DebugStream &operator<<(const bool output) { std::cout << (output ? " true" : " false"); return *this; }
};

static inline DebugStream debug(std::string area) { return DebugStream(area); }

#endif // DEBUG_H
