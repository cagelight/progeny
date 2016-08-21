#pragma once
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

typedef VkXcbSurfaceCreateInfoKHR NativeSurfaceCreateInfo;
