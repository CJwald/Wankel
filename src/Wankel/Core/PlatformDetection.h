#pragma once

// Platform detection using predefined macros

#ifdef _WIN32
    #define WK_PLATFORM_WINDOWS

    #ifndef _WIN64
        #error "x86 Builds are not supported!"
    #endif

#elif defined(__APPLE__) || defined(__MACH__)
    #error "MacOS is not supported!"

#elif defined(__ANDROID__)
    #define WK_PLATFORM_ANDROID

#elif defined(__linux__)
    #define WK_PLATFORM_LINUX

#else
    #error "Unknown platform!"
#endif
