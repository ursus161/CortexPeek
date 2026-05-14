#pragma once
#include <memory>
#include <vector>

enum class DebugEventType {
    Breakpoint,
    SingleStep,
    ProcessExited,
    Signal,
};

struct DebugEvent {
    DebugEventType type;
    int            data = 0; // signal number or exit code depending on type
};

class IDebugObserver {
public:
    virtual ~IDebugObserver() = default;
    virtual void onEvent(const DebugEvent& event) = 0;
};

class DebugEventSource {
public:
    void subscribe(std::shared_ptr<IDebugObserver> observer);
    void unsubscribe(const std::shared_ptr<IDebugObserver>& observer);

protected:
    void notify(const DebugEvent& event);

private:
    // weak_ptr so the source doesn't keep dead observers alive
    std::vector<std::weak_ptr<IDebugObserver>> observers_;
};
