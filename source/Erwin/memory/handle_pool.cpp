#include "memory/handle_pool.h"

namespace erwin
{

HandlePool::HandlePool(uint16_t max_handles):
count_(0),
max_handles_(max_handles)
{
	reset();
}

uint16_t HandlePool::acquire()
{
	if(count_ < max_handles_)
	{
		uint16_t index = count_;
		++count_;

		uint16_t* dense  = get_dense_ptr();
		uint16_t  handle = dense[index];
		uint16_t* sparse = get_sparse_ptr();
		sparse[handle] = index;
		return handle;
	}

	return INVALID_HANDLE;
}

void HandlePool::release(uint16_t handle)
{
	uint16_t* dense  = get_dense_ptr();
	uint16_t* sparse = get_sparse_ptr();
	uint16_t index = sparse[handle];
	--count_;
	uint16_t temp = dense[count_];
	dense[count_] = handle;
	sparse[temp] = index;
	dense[index] = temp;
}

bool HandlePool::is_valid(uint16_t handle) const
{
	if(handle == INVALID_HANDLE)
		return false;

	uint16_t* dense  = get_dense_ptr();
	uint16_t* sparse = get_sparse_ptr();
	uint16_t  index  = sparse[handle];

	return (index < count_) && (dense[index] == handle);
}

void HandlePool::reset()
{
	count_ = 0;
	uint16_t* dense = get_dense_ptr();
	for(uint16_t ii=0, num=max_handles_; ii<num; ++ii)
		dense[ii] = ii;
}

} // namespace erwin