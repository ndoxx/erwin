#pragma once

#include <vector>
#include <functional>
#include <cstdint>

namespace erwin
{

struct RenderCommand
{
    int id;
};

typedef uint64_t RenderKey;

class RenderQueue
{
public:
    // Push a single command together with a key for later command sorting
    void push(RenderKey key, RenderCommand&& command);
    // Push a group of commands with associated keys
    void push(const std::vector<RenderKey>& keys, std::vector<RenderCommand>&& commands);
    // Sort queue, visit each command in order, then clear
    void flush(std::function<void(const RenderCommand&)> visit);
    // Get the current queue length
    inline std::size_t size() const { return order_.size(); }

protected:
    // Sort order pairs in ascending order w.r.t keys
    void sort();

private:
    typedef std::pair<RenderKey, std::size_t> OrderPair;
    std::vector<RenderCommand> commands_;
    std::vector<OrderPair> order_;
};

} // namespace erwin
