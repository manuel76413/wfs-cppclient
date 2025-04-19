#pragma once

// Define export macros
#if defined(_MSC_VER) // Microsoft Visual C++
#if defined(WFS_CLIENT_EXPORTS)
#define WFS_CLIENT_API extern "C" __declspec(dllexport)
#elif defined(WFS_CLIENT_DLL)
#define WFS_CLIENT_API extern "C" __declspec(dllimport)
#else
#define WFS_CLIENT_API
#endif
#else // Other compilers (GCC/Clang)
#if defined(WFS_CLIENT_EXPORTS)
#define WFS_CLIENT_API extern "C" __attribute__((visibility("default")))
#else
#define WFS_CLIENT_API
#endif
#endif