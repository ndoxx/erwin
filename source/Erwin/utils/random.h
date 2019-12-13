#pragma once

#include <cstdint>
#include <ostream>
#include <limits>

namespace erwin
{

/*
	Implementation of a XorShift128+ random number generator.
	Most of the code is taken from: https://en.wikipedia.org/wiki/Xorshift
*/
class XorShiftEngine
{
public:
	typedef uint64_t result_type;

	struct Seed
	{
		uint64_t state_[2];

		Seed() = default;
		explicit  Seed(const char* str);
		explicit  Seed(uint64_t seed);
		constexpr Seed(uint64_t upper, uint64_t lower): state_{upper, lower} { }
		constexpr Seed(const Seed& rhs): state_{rhs.state_[0], rhs.state_[1]} { }
		constexpr Seed& operator=(const Seed& rhs) { state_[0]=rhs.state_[0]; state_[1]=rhs.state_[1]; return *this; }

		friend std::ostream& operator <<(std::ostream& stream, Seed rhs);
	};

	XorShiftEngine() = default;

	inline Seed get_seed() const                 { return seed_; }
	inline void set_seed(Seed seed)              { seed_ = seed; }
	inline void set_seed(uint64_t seed)          { seed_ = Seed(seed); }
	inline void set_seed_string(const char* str) { seed_ = Seed(str); }
	void set_seed_time();

	uint64_t rand64();
	inline uint64_t operator()() { return rand64(); }
	inline uint32_t rand() { return static_cast<uint32_t>(rand64()); }

	inline uint64_t min() const { return std::numeric_limits<result_type>::min(); }
	inline uint64_t max() const { return std::numeric_limits<result_type>::max(); }

private:
	Seed seed_;
};


} // namespace erwin