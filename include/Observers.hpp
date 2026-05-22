#pragma once
#include "Observer.hpp"
#include "History.hpp"

class LogObserver : public IDebugObserver {
public:
    void onEvent(const DebugEvent& event) override;
};

class HistoryObserver : public IDebugObserver {
public:
    void onEvent(const DebugEvent& event) override;
    const History<DebugEvent>& history() const { return history_; }

private:
    History<DebugEvent> history_{100u};
};
