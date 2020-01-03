#pragma once

#include <cstdlib>
#include <memory>

// Export JSON instrumentation profiles compatible with chrome://tracing
#define W_PROFILE

#ifdef _WIN32
	#ifdef W_BUILD_LIB
		#define W_API __declspec(dllexport)
	#else
		#define W_API __declspec(dllimport)
	#endif
#else
	#define W_API
#endif

// Macro to break into debugger
#ifdef W_DEBUG
	#ifdef __linux__
		#include <csignal>
		#define W_DEBUG_BREAK() raise(SIGTRAP)
	#endif
	#ifdef _WIN32
		#define W_DEBUG_BREAK() __debugbreak()
	#endif
#else
	#define W_DEBUG_BREAK()
#endif

#ifdef W_ENABLE_ASSERT
	#include <cstdio>
	#define ASSERT_FMT_BUFFER_SIZE__ 128
	static char ASSERT_FMT_BUFFER__[ASSERT_FMT_BUFFER_SIZE__];
	#define W_STATIC_ERROR(FSTR, ...) printf( FSTR , __VA_ARGS__ )
	#define W_ASSERT_FMT(CND, FORMAT_STR, ...) { if(!( CND )) { snprintf(ASSERT_FMT_BUFFER__, ASSERT_FMT_BUFFER_SIZE__, FORMAT_STR, __VA_ARGS__); printf("\033[1;38;2;255;0;0mAssertion failed:\033[0m '%s' -- %s\n%s:%s:%d\n", #CND , ASSERT_FMT_BUFFER__ , __FILE__ , __func__ , __LINE__ ); W_DEBUG_BREAK(); }}
	#define W_ASSERT(CND, STR) { if(!( CND )) { printf("\033[1;38;2;255;0;0mAssertion failed:\033[0m '%s' -- %s\n%s:%s:%d\n", #CND , STR , __FILE__ , __func__ , __LINE__ ); W_DEBUG_BREAK(); }}
#else
	#define W_STATIC_ERROR(FSTR, ...)
	#define W_ASSERT_FMT(CND, FORMAT_STR, ...)
	#define W_ASSERT(CND, STR)
#endif

inline void fatal() { exit(-1); }

// Instrumentation timer
#ifdef W_PROFILE
	#include "debug/instrumentor.h"
	#define W_PROFILE_BEGIN_SESSION(name, filepath) erwin::Instrumentor::begin_session( name , filepath )
	#define W_PROFILE_END_SESSION(name) erwin::Instrumentor::end_session()
	#define W_PROFILE_ENABLE_SESSION(value) erwin::Instrumentor::set_session_enabled( value )
	#define W_PROFILE_SCOPE(name) erwin::InstrumentationTimer timer##__LINE__( name );
	#define W_PROFILE_FUNCTION() W_PROFILE_SCOPE( __PRETTY_FUNCTION__ )
#else
	#define W_PROFILE_BEGIN_SESSION(name, filepath)
	#define W_PROFILE_END_SESSION(name)
	#define W_PROFILE_ENABLE_SESSION(value)
	#define W_PROFILE_SCOPE(name)
	#define W_PROFILE_FUNCTION()
#endif

// Macro to disallow copy and assign
#define NON_COPYABLE( TYPE_NAME ) \
	TYPE_NAME(const TYPE_NAME&) = delete; \
	TYPE_NAME& operator=(const TYPE_NAME&) = delete

// Macro to make a class non-movable
#define NON_MOVABLE( TYPE_NAME ) \
	TYPE_NAME(TYPE_NAME&&) = delete; \
	TYPE_NAME& operator=(TYPE_NAME&&) = delete

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