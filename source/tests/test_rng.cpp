#include <array>
#include <random>

#include "catch2/catch.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "utils/random.h"

using namespace erwin;

class XorShiftFixture
{
public:
	XorShiftFixture()
	{
		rng.seed();
		DLOG("rng",1) << "Seed: " << rng.get_seed() << std::endl;
	}

protected:
	XorShiftEngine rng;
};

TEST_CASE_METHOD(XorShiftFixture, "Rand64", "[rng]")
{
	for(int ii=0; ii<10; ++ii)
		DLOG("rng",1) << rng() << " ";
	DLOG("rng",1) << std::endl;

	SUCCEED();
}

TEST_CASE_METHOD(XorShiftFixture, "Uniform int distribution", "[rng]")
{
	std::uniform_int_distribution<int> dis(1,6);
	for(int ii=0; ii<100; ++ii)
		DLOG("rng",1) << dis(rng) << " ";
	DLOG("rng",1) << std::endl;

	SUCCEED();
}

TEST_CASE_METHOD(XorShiftFixture, "Uniform float distribution", "[rng]")
{
	std::uniform_real_distribution<float> dis(-1.f,1.f);
	for(int ii=0; ii<10; ++ii)
		DLOG("rng",1) << dis(rng) << " ";
	DLOG("rng",1) << std::endl;

	SUCCEED();
}

TEST_CASE_METHOD(XorShiftFixture, "Bit statistics", "[rng]")
{
	int Nexp = 1000000;
	std::array<uint32_t,64> bit_stats;
	for(int ii=0; ii<64; ++ii)
		bit_stats[ii] = 0;
	for(int kk=0;kk<Nexp; ++kk)
	{
		uint64_t rnd = rng.rand64();
		for(int ii=0; ii<64; ++ii)
			bit_stats[ii] += uint32_t((rnd & (1ul<<ii))!=0);
	}

	for(int ii=0; ii<64; ++ii)
		DLOG("rng",1) << ii << ": " << bit_stats[ii]/float(Nexp) << std::endl;

	SUCCEED();
}