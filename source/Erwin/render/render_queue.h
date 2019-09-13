#pragma once

#include <vector>
#include <functional>
#include <cstdint>

namespace erwin
{

enum class QueueItemType: uint8_t
{
    STATE_MUTATION,
    DRAW_COMMAND
};

struct QueueItem
{
    QueueItemType type;         // Is this queue item to be understood as a state mutation or instance draw command?
    const void* render_state;   // Relevant when queue item is a per-batch state mutation
    const void* instance_data;  // Relevant when queue item is a per-instance draw command

    uint32_t id; // TMP for tests
};

typedef uint64_t RenderKey;

class RenderQueue
{
public:
    // Push a single item together with a key for later draw command/state mutation sorting
    void push(RenderKey key, QueueItem&& item);
    // Push a group of commands with associated keys
    void push(const std::vector<RenderKey>& keys, std::vector<QueueItem>&& item);
    // Sort queue, visit each item in order, then clear
    void flush(std::function<void(const QueueItem&)> visit);
    // Get the current queue length
    inline std::size_t size() const { return order_.size(); }

protected:
    // Sort order pairs in ascending order w.r.t keys
    void sort();

private:
    typedef std::pair<RenderKey, std::size_t> OrderPair;
    std::vector<QueueItem> items_;
    std::vector<OrderPair> order_;
};

} // namespace erwin
