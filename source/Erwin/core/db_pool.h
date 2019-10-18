#pragma once

#include <cstdint>

namespace erwin
{

// Memory pool to exchange data with the master renderer
class DoubleBufferedPool
{
public:
	DoubleBufferedPool(uint32_t size_bytes);
	~DoubleBufferedPool();

	inline uint8_t* get_front_pointer() { return &data_[FRONT_][offset_]; }
	inline uint8_t* get_back_pointer()  { return data_[BACK_]; }

	// Copy data to front buffer, specifying size, beginning offset is returned
	uint32_t push(void* data, uint32_t size);

	void swap_buffers();

private:
	uint32_t size_;
	uint32_t offset_;
	uint32_t FRONT_;
	uint32_t BACK_;
	uint8_t* data_[2];
};


} // namespace erwin