#pragma once

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#ifndef TARGET_UNIX
#define TARGET_UNIX 1
#endif
#ifndef DECLSPEC_NORETURN
#define DECLSPEC_NORETURN __attribute__((noreturn))
#endif
#include <cstddef>
#endif
