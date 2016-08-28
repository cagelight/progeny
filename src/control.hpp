#pragma once
#include "com.hpp"
#include "vk.hpp"

namespace control {
	
	void init();
	void term() noexcept;
	void frame();
	VkExtent2D extent() noexcept;
	
	extern std::atomic_bool run_sem;
}
