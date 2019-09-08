#pragma once

#include <cstdlib>

#ifdef W_PLATFORM_WINDOWS
	#ifdef W_BUILD_LIB
		#define W_API __declspec(dllexport)
	#else
		#define W_API __declspec(dllimport)
	#endif
#else
	#define W_API
#endif

inline void fatal() { exit(-1); }