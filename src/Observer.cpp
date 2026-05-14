#include "Observer.hpp"
#include <algorithm>

void DebugEventSource::subscribe(std::shared_ptr<IDebugObserver> observer) {
    observers_.emplace_back(observer);
}

void DebugEventSource::unsubscribe(const std::shared_ptr<IDebugObserver>& observer) {
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [&](const std::weak_ptr<IDebugObserver>& weakObserver) {
                auto sharedObserver = weakObserver.lock();
                return !sharedObserver || sharedObserver == observer;
            }),
        observers_.end());
}

void DebugEventSource::notify(const DebugEvent& event) {
    auto current = observers_.begin();
    while (current != observers_.end()) {
        if (auto sharedObserver = current->lock()) {
            sharedObserver->onEvent(event);
            ++current;
        } else {
            // observer was destroyed; clean it up
            current = observers_.erase(current);
        }
    }
}
