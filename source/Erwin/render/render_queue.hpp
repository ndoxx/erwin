#pragma once

#include <vector>
#include <functional>

#include "render/render_state.h"

namespace erwin
{

// Sort key calculation policy
template <typename QueueDataT>
struct SortKeyCreator
{
    inline uint64_t operator()(const QueueDataT& data);
};

class AbstractRenderQueue
{
public:
    virtual ~AbstractRenderQueue() = default;
    virtual void flush() = 0;
};

template <typename QueueDataT>
class RenderQueue: public AbstractRenderQueue
{
protected:
    struct QueueItem;

public:
    typedef QueueDataT DataT;
    typedef uint64_t SortKey;

    inline uint32_t get_count() { return items_.size(); }

    void resize_pool(uint32_t num_data, uint32_t num_rs)
    {
        data_pool_.resize(num_data);
        pass_state_pool_.resize(num_rs);
    }

    inline QueueDataT* data_ptr()      { return &data_pool_[data_pool_cur_index_++]; }
    inline PassState* pass_state_ptr() { return &pass_state_pool_[ps_pool_cur_index_++]; }

    void push(QueueDataT* data)
    {
        order_.push_back(items_.size());
        items_.push_back(QueueItem(data));
        set_key(items_.back());
    }

    void begin_pass(PassState* state)
    {
        ++current_pass_;
        order_.push_back(items_.size());
        items_.push_back(QueueItem(state));
        set_key(items_.back());
    }

    virtual void flush() override
    {
        sort();

        for(uint32_t index: order_)
        {
            QueueItem& item = items_[index];
            if(item.is_state)
                on_state_(*item.content.render_state);
            else
                on_data_(*item.content.data);
            item.release();
        }

        items_.clear();
        order_.clear();
        current_pass_ = 0;
        data_pool_cur_index_ = 0;
        ps_pool_cur_index_ = 0;
    }

    void set_command_handlers(std::function<void(const QueueDataT&)> on_data,
                              std::function<void(const PassState&)> on_state)
    {
        on_data_ = on_data;
        on_state_ = on_state;
    }

protected:
    void sort()
    {
        std::sort(order_.begin(), order_.end(), [this](uint32_t ind1, uint32_t ind2) -> bool
        {
            return items_[ind1].key > items_[ind2].key;
        });
    }

    inline void set_key(QueueItem& item)
    {
        item.key = item.is_state ? (1<<24) : make_key_(*item.content.data);
        item.key |= (~current_pass_ << 25);
    }

    struct QueueItem
    {
        SortKey key = 0;
        bool is_state = false;
        union
        {
            QueueDataT* data;
            PassState* render_state;
        } content;

        QueueItem(QueueDataT* data)
        {
            is_state = false;
            content.data = data;
        }
        QueueItem(PassState* state)
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

private:
    SortKeyCreator<QueueDataT> make_key_;
    std::vector<QueueItem> items_;
    std::vector<uint32_t> order_;
    std::vector<QueueDataT> data_pool_;
    std::vector<PassState> pass_state_pool_;
    std::function<void(const QueueDataT&)> on_data_;
    std::function<void(const PassState&)> on_state_;
    uint32_t data_pool_cur_index_ = 0;
    uint32_t ps_pool_cur_index_ = 0;
    uint8_t current_pass_ = 0;
};

} // namespace erwin
