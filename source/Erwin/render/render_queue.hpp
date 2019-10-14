#pragma once

#include <vector>
#include <functional>

#include "render/render_state.h"

namespace erwin
{

template <typename QueueDataT>
class RenderQueue
{
public:
    struct QueueItem;
    typedef QueueDataT DataT;
    typedef QueueItem QueueItemT;
    typedef typename QueueDataT::RenderKey RenderKey;

    void resize_pool(uint32_t num_data, uint32_t num_rs)
    {
        data_pool_.resize(num_data);
        render_state_pool_.resize(num_rs);
    }

    inline QueueDataT*  data_ptr()         { return &data_pool_[data_pool_cur_index_++]; }
    inline RenderState* render_state_ptr() { return &render_state_pool_[rs_pool_cur_index_++]; }

    void push(QueueDataT* data)
    {
        order_.push_back(items_.size());
        items_.push_back(QueueItem(data));
        make_key(items_.back());
    }

    void push(RenderState* state)
    {
        ++current_pass_;
        order_.push_back(items_.size());
        items_.push_back(QueueItem(state));
        make_key(items_.back());
    }

    void flush(std::function<void(const QueueDataT&)> on_data,
               std::function<void(const RenderState&)> on_state)
    {
        sort();

        for(uint32_t index: order_)
        {
            QueueItem& item = items_[index];
            if(item.is_state)
                on_state(*item.content.render_state);
            else
                on_data(*item.content.data);
            item.release();
        }

        items_.clear();
        order_.clear();
        current_pass_ = 0;
        data_pool_cur_index_ = 0;
        rs_pool_cur_index_ = 0;
    }

    struct QueueItem
    {
        RenderKey key = 0;
        bool is_state = false;
        union
        {
            QueueDataT* data;
            RenderState* render_state;
        } content;

        QueueItem(QueueDataT* data)
        {
            is_state = false;
            content.data = data;
        }
        QueueItem(RenderState* state)
        {
            is_state = true;
            content.render_state = state;
        }

        inline void release()
        {
            if(is_state)
                content.render_state->reset();
            else
                content.data->reset();
        }
    };

protected:
    void sort()
    {
        std::sort(order_.begin(), order_.end(), [this](uint32_t ind1, uint32_t ind2) -> bool
        {
            return items_[ind1].key > items_[ind2].key;
        });
    }

    void make_key(QueueItem& item);

private:
    std::vector<QueueItem> items_;
    std::vector<uint32_t> order_;
    std::vector<QueueDataT> data_pool_;
    std::vector<RenderState> render_state_pool_;
    uint32_t data_pool_cur_index_ = 0;
    uint32_t rs_pool_cur_index_ = 0;
    uint8_t current_pass_ = 0;
};

} // namespace erwin
