#pragma once
#include <deque>
#include <optional>
#include <cstddef>

template<typename T>
class History {
public:
    explicit History(size_t maxSize = 100) : maxSize_(maxSize) {}

    void push(T item) {
        if (entries_.size() == maxSize_)
            entries_.pop_front();
        entries_.push_back(std::move(item));
    }

    // returns the most recently pushed entry
    std::optional<T> last() const {
        if (entries_.empty()) return std::nullopt;
        return entries_.back();
    }

    const std::deque<T>& entries() const { return entries_; }
    size_t size()                  const { return entries_.size(); }
    bool   empty()                 const { return entries_.empty(); }

private:
    std::deque<T> entries_;
    size_t        maxSize_;
};
