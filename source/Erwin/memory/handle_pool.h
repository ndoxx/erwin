#pragma once

#include <cstdint>
#include <cstring>

/*
	HandlePool is based on Branimir Karadzic's HandleAlloc (bx library), I
	basically just changed names but the code is pretty much the same.
	RobustHandlePool is an extension I coded that allows for better tracking
	of handle validity at the cost of more memory usage.
*/

namespace erwin
{

/*
	This class allows for the acquisition and release of handle indices.
	Released indices will be reused by later acquisitions. A handle
	using this system is considered valid as long as the pool hasn't
	released its index. So a copy of a released handle of index [i] 
	will still be considered valid if the pool has allocated [i] again
	in the meantime, which can be dangerous.
*/
class HandlePool
{
public:
	static constexpr uint16_t INVALID_HANDLE = 0xffff;

	explicit HandlePool(uint16_t max_handles);
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
		uint8_t* ptr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this));
		return reinterpret_cast<uint16_t*>(&ptr[sizeof(HandlePool)]);
	}

	inline uint16_t* get_sparse_ptr() const
	{
		return &get_dense_ptr()[max_handles_];
	}

	uint16_t count_;
	uint16_t max_handles_;
};

/*
	Actual fixed-size implementation of a HandlePool.
*/
template <uint16_t MaxHandlesT>
class HandlePoolT: public HandlePool
{
public:
	static constexpr std::size_t k_size_bytes = 2*MaxHandlesT*sizeof(uint16_t);

	HandlePoolT(): HandlePool(MaxHandlesT) {}
	~HandlePoolT() = default;

private:
	uint16_t padding_[2*MaxHandlesT];
};



/*
	This pool stores composite handles made of an index alongside a counter.
	Each time a handle is released, its index can be reused later on. However,
	the counter is incremented internally, so the next handle that reuses this
	index will have a different counter value. We can still check for handle 
	validity in O(1) time, but this time, the concept of validity will not be 
	flawed.

	TODO:
		[ ] HandleInternal can pack a 16 bit index and a 48 bit counter together.
*/
class RobustHandlePool
{
public:
	struct HandleInternal
	{
		uint64_t index;
		uint64_t counter;
	};

	static constexpr uint64_t INVALID_HANDLE = 0xffffffffffffffff;

	RobustHandlePool(uint64_t max_handles);
	~RobustHandlePool() = default;

	inline const HandleInternal* get_handles() const     	   { return get_dense_ptr(); }
	inline HandleInternal get_handle_at(uint32_t offset) const { return get_dense_ptr()[offset]; }
	inline uint64_t get_count() const 			               { return count_; }
	inline uint64_t get_max_handles() const                    { return max_handles_; }

	HandleInternal acquire();
	void release(HandleInternal handle);
	bool is_valid(HandleInternal handle) const;
	void reset();

private:
	RobustHandlePool();

	inline HandleInternal* get_dense_ptr() const
	{
		uint8_t* ptr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this));
		return reinterpret_cast<HandleInternal*>(&ptr[sizeof(RobustHandlePool)]);
	}

	inline HandleInternal* get_sparse_ptr() const
	{
		return &get_dense_ptr()[max_handles_];
	}

	uint64_t count_;
	uint64_t max_handles_;
};

/*
	Actual fixed-size implementation of a RobustHandlePool.
*/
template <uint64_t MaxHandlesT>
class RobustHandlePoolT: public RobustHandlePool
{
public:
	static constexpr std::size_t k_size_bytes = 2*MaxHandlesT*sizeof(HandleInternal);

	RobustHandlePoolT(): RobustHandlePool(MaxHandlesT) {}
	~RobustHandlePoolT() = default;

private:
	HandleInternal padding_[2*MaxHandlesT];
};


} // namespace erwin