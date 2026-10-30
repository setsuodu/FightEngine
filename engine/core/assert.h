#pragma once
#include <intrin.h>
#include "core/log.h"
#define ENG_ASSERT(c) do { if (!(c)) { ::eng::LogError("ASSERT failed: %s (%s:%d)", #c, __FILE__, __LINE__); __debugbreak(); } } while (0)
#define ENG_HR(hr)    ENG_ASSERT(SUCCEEDED(hr))
