#include "Observers.hpp"
#include <cstdio>
#include <cstring>

void LogObserver::onEvent(const DebugEvent& event) {
    switch (event.type) {
        case DebugEventType::Breakpoint:
            std::printf("[event] breakpoint hit\n");
            break;
        case DebugEventType::SingleStep:
            std::printf("[event] single step\n");
            break;
        case DebugEventType::ProcessExited:
            std::printf("[event] process exited with code %d\n", event.data);
            break;
        case DebugEventType::Signal:
            std::printf("[event] received signal %d (%s)\n", event.data, strsignal(event.data));
            break;
    }
}

void HistoryObserver::onEvent(const DebugEvent& event) {
    history_.push(event);
}
