#include "core/db_pool.h"
#include "core/core.h"
#include "debug/logger.h"

#include <utility>
#include <cstring>

namespace erwin
{


DoubleBufferedPool::DoubleBufferedPool(uint32_t size_bytes):
size_(size_bytes),
offset_(0),
FRONT_(0),
BACK_(1)
{
	DLOG("core",1) << "Allocating double-buffered pool:" << std::endl;
	data_[FRONT_] = new uint8_t[size_];
	data_[BACK_]  = new uint8_t[size_];
	memset(&data_[FRONT_][0], 0, size_);
	memset(&data_[BACK_][0], 0, size_);
	DLOGI << "size: " << size_ << "B" << std::endl;
}

DoubleBufferedPool::~DoubleBufferedPool()
{
	DLOG("core",1) << "Destroying double-buffered pool." << std::endl;
	delete[] data_[FRONT_];
	delete[] data_[BACK_];
}

uint32_t DoubleBufferedPool::push(void* data, uint32_t size)
{
	W_ASSERT(offset_+size < size_, "DoubleBufferedPool overflow on push.");

	uint32_t ret = offset_;
	memcpy(&data_[FRONT_][offset_], data, size);
	offset_ += size;
	return ret;
}

void DoubleBufferedPool::swap_buffers()
{
	std::swap(FRONT_, BACK_);
	offset_ = 0;
}


} // namespace erwin