#pragma once
#include <print>
#include <algorithm>

enum class LStatus {
    INFO,
    WARNING,
    FATAL
};


inline static void log_msg_type(LStatus status) {
#ifndef NDEBUG

    switch (status) {
        case LStatus::INFO:
            std::print("[INFO] ");
            break;
        case LStatus::WARNING:
            std::print("[WARNING] ");
            break;
        case LStatus::FATAL:
            std::print("[FATAL] ");
            break;
    }

#endif
}


namespace logger {

template<typename ...Args>
inline void log(LStatus status, const std::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
    log_msg_type(status);
    std::println(fmt, std::forward<Args>(args)...);
#endif
}


//old version compatibility
inline void log(const std::string& s) {
#ifndef NDEBUG
    
    std::println("[DEBUG] {}", s);

#endif
}

inline void log(LStatus status, const std::string_view str) {
#ifndef NDEBUG
    
    switch (status) {
        case LStatus::INFO:
            std::println("[INFO] {}", str);
            break;
        case LStatus::WARNING:
            std::println("[WARNING] {}", str);
            break;
        case LStatus::FATAL:
            std::println("[FATAL] {}", str);
            break;
    }

#endif
}



template<std::ranges::input_range R>
inline void log_enum(LStatus status, const std::string& msg, R&& r) {
#ifndef NDEBUG
    log_msg_type(status);


    std::print("{}", msg);

    if (r.empty()) {
        return;
    }
    
    auto it = r.begin();
    std::print("{}", *it);
    ++it;
    for (; it != r.end(); ++it) {
        std::print(", {}", *it);
    }
    std::println();

#endif
}

}

