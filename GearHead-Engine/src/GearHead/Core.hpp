#pragma once


#ifdef GEARHEAD_PLATFORM_WINDOWS
    #ifdef GEARHEAD_BUILD_DLL
        #define GEARHEAD_API __declspec(dllexport)
    #else
        #define GEARHEAD_API __declspec(dllimport)
    #endif
#elif defined(GEARHEAD_PLATFORM_GCC)
    #ifdef GEARHEAD_BUILD_DLL
        #define GEARHEAD_API __attribute__ ((visibility("default")))
    #else
        #define GEARHEAD_API __attribute__ ((visibility("hidden")))
    #endif

#else
    #error "Compiler not Supported"
#endif 

#ifdef GEARHEAD_ENABLE_ASSERTS
    #define GEARHEAD_ASSERT(x, ...) {if(!x){ GEARHEAD_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
    #define GEARHEAD_CORE_ASSERT(x, ...) {if(!x){ GEARHEAD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

#else
    #define GEARHEAD_ASSERT(x, ...)
    #define GEARHEAD_CORE_ASSERT(x, ...)

#endif


#define BIT(x) (1 << x)