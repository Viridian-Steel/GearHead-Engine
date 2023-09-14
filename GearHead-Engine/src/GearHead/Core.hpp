#pragma once


#ifdef GEARHEAD_PLATFORM_WINDOWS

	#ifdef GEARHEAD_DEBUG
		#define GEARHEAD_DEBUGBREAK() __debugbreak()
	#else
		#define GEARHEAD_DEBUGBREAK()
	#endif // DEBUG


    #ifdef GEARHEAD_BUILD_DLL
        #define GEARHEAD_API __declspec(dllexport)
    #else
        #define GEARHEAD_API __declspec(dllimport)
    #endif
#elif defined(GEARHEAD_PLATFORM_UNIX)
	#ifdef GEARHEAD_DEBUG
		#include <signal.h>
		#define GEARHEAD_DEBUGBREAK() raise(SIGTRAP)

	#endif // DEBUG

    #ifdef GEARHEAD_BUILD_DLL
        #define GEARHEAD_API __attribute__ ((visibility("default")))
    #else
        #define GEARHEAD_API __attribute__ ((visibility("hidden")))
    #endif

#else
    #error "OS not Supported"
#endif 

#ifdef GEARHEAD_ENABLE_ASSERTS
    #define GEARHEAD_ASSERT(x, ...) {if(!x){ GEARHEAD_ERROR("Assertion Failed: {0}", __VA_ARGS__); GEARHEAD_DEBUGBREAK(); } } 
    #define GEARHEAD_CORE_ASSERT(x, ...) {if(!x){ GEARHEAD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); GEARHEAD_DEBUGBREAK(); } }
	#define GEARHEAD_ASSERT_FUNC(x, ...) {if(!x){ GEARHEAD_ERROR("Assertion Failed: {0}", __VA_ARGS__); GEARHEAD_DEBUGBREAK(); } } 
    #define GEARHEAD_CORE_ASSERT_FUNC(x, ...) {if(!x){ GEARHEAD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); GEARHEAD_DEBUGBREAK(); } }

#else
    #define GEARHEAD_ASSERT(x, ...) 
	#define GEARHEAD_ASSERT_FUNC(x, ...) x
    #define GEARHEAD_CORE_ASSERT(x, ...)
	#define GEARHEAD_CORE_ASSERT_FUNC(x, ...) x

#endif


#define BIT(x) (1 << x)
