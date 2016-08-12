#pragma once
#include "com.h"

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

typedef struct com_vk_physical_device_s {
	VkPhysicalDevice pdev;
	VkPhysicalDeviceProperties pdev_props;
	VkExtensionProperties * sup_ext;
	VkQueueFamilyProperties * queuefam_props;
} com_vk_physical_device_t;

typedef struct com_vk_device_s {
	VkDevice device;
	#define VK_FN_DDECL
	#include "com_vk_func.inl"
} com_vk_device_t;
