#include "render_queue.h"

namespace erwin
{

void RenderQueue::push(RenderKey key, QueueItem&& item)
{
    items_.push_back(std::forward<QueueItem>(item));
    order_.push_back(std::make_pair(key, items_.size()-1));
}

void RenderQueue::push(const std::vector<RenderKey>& keys, std::vector<QueueItem>&& items)
{
    std::size_t offset = items_.size();
    items_.insert
    (
        items_.end(),
        std::make_move_iterator(items.begin()),
        std::make_move_iterator(items.end())
    );

    std::size_t ii = 0;
    for(auto&& key: keys)
        order_.push_back(std::make_pair(key, offset+ii++));
}

void RenderQueue::sort()
{
    std::sort(order_.begin(), order_.end(), [](OrderPair p1, OrderPair p2)
    {
        return p1.first <= p2.first; // Weak ordering to allow multiple same key
    });
}

void RenderQueue::flush(std::function<void(const QueueItem&)> visit)
{
    sort();

    for(auto&& [key,index]: order_)
        visit(items_[index]);
    items_.clear();
    order_.clear();
}


} // namespace erwin
