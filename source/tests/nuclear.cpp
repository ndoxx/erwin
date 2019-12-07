#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>

// using namespace erwin;

static inline bool has_mutated(uint64_t state, uint64_t old_state, uint64_t mask)
{
	return ((state^old_state)&mask) > 0;
}

int main(int argc, char** argv)
{
	uint64_t A  = 0b01101011;
	uint64_t B  = 0b01011011;
	uint64_t C  = 0b01101011;
	uint64_t M1 = 0b11110000;
	uint64_t M2 = 0b00001111;

	std::cout << has_mutated(A,C,M1) << std::endl;
	std::cout << has_mutated(A,C,M2) << std::endl;
	std::cout << has_mutated(A,B,M1) << std::endl;
	std::cout << has_mutated(A,B,M2) << std::endl;

	return 0;
}
