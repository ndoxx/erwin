#pragma once

#include "core/core.h"
#include "memory/memory_utils.h"
#include <kibble/logger/logger.h>
#include <vector>



namespace erwin
{
namespace memory
{

class HeapArea
{
public:
    HeapArea() = default;
    explicit HeapArea(size_t size) { init(size); }

    ~HeapArea() { delete[] begin_; }

    inline bool init(size_t size)
    {
        KLOG("memory", 1) << kb::WCC('i') << "[HeapArea]" << kb::WCC(0) << " Initializing:" << std::endl;
        size_ = size;
        begin_ = new uint8_t[size_];
        head_ = begin_;
#ifdef HEAP_AREA_MEMSET_ENABLED
        memset(begin_, AREA_MEMSET_VALUE, size_);
#endif
        KLOGI << "Size:  " << kb::WCC('v') << size_ << kb::WCC(0) << "B" << std::endl;
        KLOGI << "Begin: 0x" << std::hex << uint64_t(begin_) << std::dec << std::endl;
        return true;
    }

    inline void* begin() { return begin_; }
    inline void* end() { return begin_ + size_ + 1; }
    inline std::pair<void*, void*> range() { return {begin(), end()}; }

    // Get a range of pointers to a memory block within area, and advance head
    inline std::pair<void*, void*> require_block(size_t size, const char* debug_name = nullptr)
    {
        // Page align returned block to avoid false sharing if multiple threads access this area
        size_t padding = utils::alignment_padding(head_, 64);
        K_ASSERT(head_ + size + padding < end(), "[HeapArea] Out of memory!");

        // Mark padding area
#ifdef HEAP_AREA_PADDING_MAGIC
        std::fill(head_, head_ + padding, AREA_PADDING_MARK);
#endif

        std::pair<void*, void*> ptr_range = {head_ + padding, head_ + padding + size + 1};

        KLOG("memory", 1) << kb::WCC('i') << "[HeapArea]" << kb::WCC(0) << " allocated aligned block:" << std::endl;
        if(debug_name)
        {
            KLOGI << "Name:      " << kb::WCC('n') << debug_name << std::endl;
        }
        KLOGI << "Size:      " << kb::WCC('v') << size << kb::WCC(0) << "B" << std::endl;
        KLOGI << "Padding:   " << kb::WCC('v') << padding << kb::WCC(0) << "B" << std::endl;
        KLOGI << "Remaining: " << kb::WCC('v')
              << static_cast<uint64_t>(static_cast<uint8_t*>(end()) - (head_ + size + padding)) << kb::WCC(0) << "B"
              << std::endl;
        KLOGI << "Address:   0x" << std::hex << reinterpret_cast<uint64_t>(head_ + padding) << std::dec << std::endl;

        head_ += size + padding;

#ifdef W_DEBUG
        items_.push_back({debug_name ? debug_name : "block", ptr_range.first, ptr_range.second, size + padding});
#endif
        return ptr_range;
    }

    inline void* require_pool_block(size_t element_size, size_t max_count, const char* debug_name = nullptr)
    {
        size_t pool_size = max_count * element_size;
        auto block = require_block(pool_size, debug_name);
        return block.first;
    }

#ifdef W_DEBUG
    void debug_show_content();

    inline const std::vector<debug::AreaItem>& get_block_descriptions() const { return items_; }

    inline void debug_hex_dump(std::ostream& stream, size_t size = 0)
    {
        if(size == 0)
            size = size_t(head_ - begin_);
        memory::hex_dump(stream, begin_, size, "HEX DUMP");
    }

    inline void fill(uint8_t filler) { std::fill(begin_, begin_ + size_, filler); }
#endif

private:
    size_t size_;
    uint8_t* begin_;
    uint8_t* head_;

#ifdef W_DEBUG
    std::vector<debug::AreaItem> items_;
#endif
};

} // namespace memory
} // namespace erwin