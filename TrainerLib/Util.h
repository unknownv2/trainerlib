#pragma once

#include "kernel.h"

#define RANDOM(maxValue) (reinterpret_cast<KUSER_SHARED_DATA*>(0x7FFE0000)->SystemTime.LowPart % maxValue)