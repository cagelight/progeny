#pragma once
#include "com.hpp"

namespace pmath {
	template <typename T> inline T clamp(T val, T min, T max) noexcept { return val < min ? min : val > max ? max : val; }
}
