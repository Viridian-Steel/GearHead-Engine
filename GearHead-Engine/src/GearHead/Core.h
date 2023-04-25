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


#define BIT(x) (1 << x)