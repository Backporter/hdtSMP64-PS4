#pragma once

#ifndef _CRT_ALIGN
#ifdef __midl
#define _CRT_ALIGN(x)
#else
#define _CRT_ALIGN(x) __declspec(align(x))
#endif
#endif

#include "../../../CSEL/source/REL/Relocation.h"
#include "../../../CSEL/source/RE/Skyrim.h"

#include <tuple>
#include <x86intrin.h>
#include <smmintrin.h>
#include <immintrin.h>