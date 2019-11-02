#pragma once

#include <cstdlib>
#include <memory>

#ifdef _WIN32
	#ifdef W_BUILD_LIB
		#define W_API __declspec(dllexport)
	#else
		#define W_API __declspec(dllimport)
	#endif
#else
	#define W_API
#endif

#ifdef W_ENABLE_ASSERT
	#include <cstdio>
	#define W_STATIC_ERROR(FSTR, ...) printf( FSTR , __VA_ARGS__ )
	#ifdef __linux__ 
		#include <csignal>
		#define W_ASSERT(CND, ...) { if(!( CND )) { printf("Assertion Failed: %s\n", __VA_ARGS__ ); raise(SIGTRAP); } }
	#endif
	#ifdef _WIN32 
		#define W_ASSERT(CND, ...) { if(!( CND )) { printf("Assertion Failed: %s\n", __VA_ARGS__ ); __debugbreak(); } }
	#endif
#else
	#define W_STATIC_ERROR(FSTR, ...)
	#define W_ASSERT(CND, ...)
#endif

inline void fatal() { exit(-1); }

namespace erwin
{
	// Ref counting pointer and unique pointer aliases (for now)
	template <class T>
	using WRef = std::shared_ptr<T>;

	template <class T>
	using WScope = std::unique_ptr<T>;

	// Factory methods for ref and scope as function alias, using perfect forwarding (for now)
	template <class T, class... Args>
	auto make_ref(Args&&... args) -> decltype(std::make_shared<T>(std::forward<Args>(args)...))
	{
	  return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <class T, class... Args>
	auto make_scope(Args&&... args) -> decltype(std::make_unique<T>(std::forward<Args>(args)...))
	{
	  return std::make_unique<T>(std::forward<Args>(args)...);
	}
} // namespace erwin