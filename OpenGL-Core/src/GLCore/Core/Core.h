// Mostly from Hazel
#pragma once

#include <memory>

#if defined GLCORE_PLATFORM_WINDOWS
	#define GLCORE_DEBUG_BREAK() __debugbreak()
#elif defined GLCORE_PLATFORM_LINUX
	#include <signal.h>
	#define GLCORE_DEBUG_BREAK() raise(SIGTRAP)
#else
	#error Unsupported platform!
#endif

#ifdef GLCORE_DEBUG
	#define GLCORE_ENABLE_ASSERTS
#endif

#ifdef GLCORE_ENABLE_ASSERTS
	#define GLCORE_ASSERT(x, ...) { if(!(x)) { LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); GLCORE_DEBUG_BREAK(); } }
#else
	#define GLCORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define GLCORE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)