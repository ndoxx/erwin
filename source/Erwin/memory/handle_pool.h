#pragma once

#include <cstdint>

namespace erwin
{

/*
	Based on Branimir Karadzic's HandleAlloc (bx library)
*/
class HandlePool
{
public:
	static constexpr uint16_t INVALID_HANDLE = 0xffff;

	HandlePool(uint16_t max_handles);
	~HandlePool() = default;

	inline const uint16_t* get_handles() const     	     { return get_dense_ptr(); }
	inline uint16_t get_handle_at(uint16_t offset) const { return get_dense_ptr()[offset]; }
	inline uint16_t get_count() const 			         { return count_; }
	inline uint16_t get_max_handles() const              { return max_handles_; }

	uint16_t acquire();
	void release(uint16_t handle);
	bool is_valid(uint16_t handle) const;
	void reset();

private:
	HandlePool();

	inline uint16_t* get_dense_ptr() const
	{
		uint8_t* ptr = (uint8_t*)reinterpret_cast<const uint8_t*>(this);
		return (uint16_t*)&ptr[sizeof(HandlePool)];
	}

	inline uint16_t* get_sparse_ptr() const
	{
		return &get_dense_ptr()[max_handles_];
	}

	uint16_t count_;
	uint16_t max_handles_;
};

template <uint16_t MaxHandlesT>
class HandlePoolT: public HandlePool
{
public:
	HandlePoolT(): HandlePool(MaxHandlesT) { }
	~HandlePoolT() = default;

private:
	uint16_t padding_[2*MaxHandlesT];
};


} // namespace erwin