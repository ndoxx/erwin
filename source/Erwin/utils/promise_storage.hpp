#pragma once

#include <future>
#include <mutex>
#include <map>

namespace erwin
{

// Helper class to store and address multiple promises
// for use in a deferred / async context
template <typename T> class PromiseStorage
{
private:
    std::map<size_t, std::promise<T>> promises_;
    size_t current_token_ = 0;
    std::mutex mutex_;

public:
    // Get a token and a new future. A promise will be stored internally
    // that will be referenced by the token.
    inline auto future_operation()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        std::promise<T> prom;
        auto fut = prom.get_future();
        promises_.emplace(std::pair(current_token_, std::move(prom)));
        return std::pair(current_token_++, std::move(fut));
    }

    // Set a value inside a promise referenced by a token. Corresponding
    // future will be notified.
    inline void fulfill(size_t token, const T& value)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        promises_.at(token).set_value(value);
        promises_.erase(token);
    }

    inline void fulfill(size_t token, T&& value)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        promises_.at(token).set_value(std::forward<T>(value));
        promises_.erase(token);
    }
};

// Specialization for void (can't SFINAE out the fulfill(1) method so here I go)
template <> class PromiseStorage<void>
{
private:
    std::map<size_t, std::promise<void>> promises_;
    size_t current_token_ = 0;
    std::mutex mutex_;

public:
    inline auto future_operation()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        std::promise<void> prom;
        auto fut = prom.get_future();
        promises_.emplace(std::pair(current_token_, std::move(prom)));
        return std::pair(current_token_++, std::move(fut));
    }

    inline void fulfill(size_t token)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        promises_.at(token).set_value();
        promises_.erase(token);
    }
};

} // namespace erwin