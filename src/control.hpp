#pragma once
#include "com.hpp"
#include "context.hpp"

namespace control {
	void init();
	void term() noexcept;
	void frame();
	
	NativeSurfaceCreateInfo vk_surface_create_info();
	
	extern std::atomic_bool run_sem;
}
