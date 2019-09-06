#include "render_queue.h"

namespace erwin
{

void RenderQueue::push(RenderKey key, RenderCommand&& command)
{
    commands_.push_back(std::forward<RenderCommand>(command));
    order_.push_back(std::make_pair(key, commands_.size()-1));
}

void RenderQueue::push(const std::vector<RenderKey>& keys, std::vector<RenderCommand>&& commands)
{
    std::size_t offset = commands_.size();
    commands_.insert
    (
        commands_.end(),
        std::make_move_iterator(commands.begin()),
        std::make_move_iterator(commands.end())
    );

    std::size_t ii = 0;
    for(auto&& key: keys)
        order_.push_back(std::make_pair(key, offset+ii++));
}

void RenderQueue::sort()
{
    std::sort(order_.begin(), order_.end(), [](OrderPair p1, OrderPair p2)
    {
        return p1.first < p2.first;
    });
}

void RenderQueue::flush(std::function<void(const RenderCommand&)> visit)
{
    sort();

    for(auto&& [key,index]: order_)
        visit(commands_[index]);
    commands_.clear();
    order_.clear();
}


} // namespace erwin
