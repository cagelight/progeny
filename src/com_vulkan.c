#include "com_internal.h"

#include <dlfcn.h>

#define VK_FN_DECL
#include "com_vulkan_fn.inl"

//================================================================
//----------------------------------------------------------------
//================================================================

static void * restrict vk_handle;

static VkInstance vk_instance = NULL;
static VkSurfaceKHR vk_surface = NULL;
static VkPhysicalDevice vk_device_physical = NULL;
static VkDevice vk_device = NULL;

static uint32_t vk_queue_index_graphics = 0;
static VkQueue vk_queue_graphics = NULL;
static uint32_t vk_queue_index_present = 0;
static VkQueue vk_queue_present = NULL;

static VkSemaphore vk_sem_imgavail = NULL;
static VkSemaphore vk_sem_rndcomp = NULL;

static VkSwapchainKHR vk_swapchain = NULL;
static VkFormat vk_swapchain_format;
static uint32_t vk_swapchain_images_count = 0;
static VkImage * vk_swapchain_images = NULL;

static VkCommandPool vk_cmdpool_graphics = NULL;
static uint32_t vk_cmdbufs_graphics_count = 0;
static VkCommandBuffer * vk_cmdbufs_graphics = NULL;

static VkRenderPass vk_renderpass = NULL;
static VkPipeline vk_graphics_pipeline;

static uint32_t vk_imageviews_count = 0;
static VkImageView * vk_imageviews = NULL;
static uint32_t vk_framebuffers_count = 0;
static VkFramebuffer * vk_framebuffers = NULL;

static bool vk_load(void) {
	vk_handle = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_NOW);
	return vk_handle ? true : false;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_init_instance(void) {
	
	VkResult res;
	bool stat = false;
	
	VkExtensionProperties * restrict supported_extensions = NULL;
	
	#define VK_FN_SYM_GLOBAL
	#include "com_vulkan_fn.inl"
	
	uint32_t extension_count;
	res = vk_EnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	if (res != VK_SUCCESS) {
		com_printf_error("vkEnumerateInstanceExtensionProperties unsuccessful (%i)", res);
		goto ret;
	};
	supported_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
	res = vk_EnumerateInstanceExtensionProperties(NULL, &extension_count, supported_extensions);
	if (res != VK_SUCCESS) {
		com_printf_error("vkEnumerateInstanceExtensionProperties unsuccessful (%i)", res);
		goto ret;
	}
	
	bool surface_supported = false;
	bool xcb_supported = false;
	
	for (uint32_t i = 0; i < extension_count; i++) {
		if (!strcmp(supported_extensions[i].extensionName, "VK_KHR_surface")) surface_supported = true;
		if (!strcmp(supported_extensions[i].extensionName, "VK_KHR_xcb_surface")) xcb_supported = true;
	}
	
	erase(supported_extensions);
	
	if (!surface_supported) {
		com_print_error("required extension VK_KHR_surface not supported");
		goto ret;
	}
	
	if (!xcb_supported) {
		com_print_error("required extension VK_KHR_xcb_surface not supported");
		goto ret;
	}
	
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Progeny",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.pEngineName = "Progeny",
		.engineVersion = VK_MAKE_VERSION(0, 0, 1),
		.apiVersion = VK_MAKE_VERSION(1, 0, 8),
	};
	
	static char const * const extensions [] = { "VK_KHR_surface", "VK_KHR_xcb_surface" };

	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = extensions,
	};
	
	res = vk_CreateInstance(&instance_create_info, NULL, &vk_instance);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateInstance unsuccessful (%i)", res);
		goto ret;
	}
	
	#define VK_FN_SYM_INSTANCE
	#include "com_vulkan_fn.inl"
	
	stat = true;
	ret:
	if (supported_extensions) free(supported_extensions);
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_create_surface(void) {
	
	VkResult res;
	bool stat = false;
	
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = com_xcb_connection,
		.window = com_xcb_window,
    };
	
	res = vk_CreateXcbSurfaceKHR(vk_instance, &surface_create_info, NULL, &vk_surface);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateXcbSurfaceKHR unsuccessful (%i)", res);
		goto ret;
	}
	
	stat = true;
	ret:
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_init_device(void) {
	
	VkResult res;
	bool stat = false;
	
	VkPhysicalDevice * restrict physical_devices = NULL;
	VkExtensionProperties * restrict device_extensions = NULL;
	VkQueueFamilyProperties * restrict queue_families = NULL;
	VkDeviceQueueCreateInfo * restrict device_queue_create_infos = NULL;
	
	uint32_t physical_device_count;
	res = vk_EnumeratePhysicalDevices(vk_instance, &physical_device_count, NULL);
	if (res != VK_SUCCESS) {
		com_printf_error("vkEnumeratePhysicalDevices unsuccessful (%i)", res);
		goto ret;
	}
	physical_devices = malloc(physical_device_count * sizeof(VkPhysicalDevice));
	res = vk_EnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices);
	if (res != VK_SUCCESS) {
		com_printf_error("vkEnumeratePhysicalDevices unsuccessful (%i)", res);
		goto ret;
	}
	
	for (uint32_t d = 0; d < physical_device_count; d++) {
		
		VkPhysicalDeviceProperties pdp;
		uint32_t device_extensions_count;
		
		bool swapchain_supported = false;
		bool glsl_supported = false;
		
		if (queue_families) erase(queue_families);
		if (device_extensions) erase(device_extensions);
		
		vk_GetPhysicalDeviceProperties(physical_devices[d], &pdp);
		
		res = vk_EnumerateDeviceExtensionProperties(physical_devices[d], NULL, &device_extensions_count, NULL);
		if (res != VK_SUCCESS) {
			com_printf_debug("skipping %s: vk_EnumerateDeviceExtensionProperties unsuccessful (%i)", pdp.deviceName, res);
			continue;
		}
		device_extensions = malloc(device_extensions_count * sizeof(VkExtensionProperties));
		res = vk_EnumerateDeviceExtensionProperties(physical_devices[d], NULL, &device_extensions_count, device_extensions);
		if (res != VK_SUCCESS) {
			com_printf_debug("skipping %s: vk_EnumerateDeviceExtensionProperties unsuccessful (%i)", pdp.deviceName, res);
			continue;
		}
		
		for (uint32_t e = 0; e < device_extensions_count; e++) {
			if (!strcmp("VK_NV_glsl_shader", device_extensions[e].extensionName)) glsl_supported = true;
			if (!strcmp("VK_KHR_swapchain", device_extensions[e].extensionName)) swapchain_supported = true;
		}
		
		erase(device_extensions);
		
		if (!swapchain_supported) {
			com_printf_debug("skipping %s: VK_KHR_swapchain unsupported", pdp.deviceName);
			continue;
		}
		
		if (!glsl_supported) {
			com_printf_debug("skipping %s: VK_NV_glsl_shader unsupported", pdp.deviceName);
			continue;
		}
		
		uint32_t queue_families_count;
		vk_GetPhysicalDeviceQueueFamilyProperties(physical_devices[d], &queue_families_count, NULL);
		if (!queue_families_count) {
			com_printf_debug("skipping %s: no queue families", pdp.deviceName);
			continue;
		}
		
		queue_families = malloc(queue_families_count * sizeof(VkQueueFamilyProperties));
		
		vk_queue_index_graphics = UINT32_MAX;
		vk_queue_index_present = UINT32_MAX;
		
		vk_GetPhysicalDeviceQueueFamilyProperties(physical_devices[d], &queue_families_count, queue_families);
		for (uint32_t q = 0; q < queue_families_count; q++) {
			if (queue_families[q].queueFlags | VK_QUEUE_GRAPHICS_BIT) vk_queue_index_graphics = q;
			VkBool32 present_supported;
			vk_GetPhysicalDeviceSurfaceSupportKHR(physical_devices[d], q, vk_surface, &present_supported);
			if (present_supported) vk_queue_index_present = q;
			if (vk_queue_index_graphics != UINT32_MAX || vk_queue_index_present != UINT32_MAX) break;
		}
		
		if (vk_queue_index_graphics == UINT32_MAX) {
			com_printf_debug("skipping %s: no queue families support graphics", pdp.deviceName);
			continue;
		}
		
		if ( vk_queue_index_present == UINT32_MAX) {
			com_printf_debug("skipping %s: no queue families support presentation", pdp.deviceName);
			continue;
		}
		
		erase(queue_families);
		
		com_printf_info("selecting device: %s (G: %u, P: %u)", pdp.deviceName, vk_queue_index_graphics, vk_queue_index_present );
		vk_device_physical = physical_devices[d];
		break;
	}
	
	if (!vk_device_physical) {
		com_print_error("no compatible vulkan devices found");
		goto ret;
	}
	
	uint32_t device_queue_create_info_count = vk_queue_index_graphics == vk_queue_index_present ? 1 : 2;
	device_queue_create_infos = malloc(device_queue_create_info_count * sizeof(VkDeviceQueueCreateInfo));
	
	float const queue_priorities [] = { 1.0f };
	
	device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_infos[0].pNext = NULL;
	device_queue_create_infos[0].flags = 0;
	device_queue_create_infos[0].queueFamilyIndex = vk_queue_index_graphics;
	device_queue_create_infos[0].queueCount = 1;
	device_queue_create_infos[0].pQueuePriorities = queue_priorities;
	
	if (vk_queue_index_graphics != vk_queue_index_present ) {
		device_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_infos[1].pNext = NULL;
		device_queue_create_infos[1].flags = 0;
		device_queue_create_infos[1].queueFamilyIndex = vk_queue_index_present;
		device_queue_create_infos[1].queueCount = 1;
		device_queue_create_infos[1].pQueuePriorities = queue_priorities;
	}
	
	static char const * const extensions [] = { "VK_KHR_swapchain", "VK_NV_glsl_shader" };
	
	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = device_queue_create_info_count,
		.pQueueCreateInfos = device_queue_create_infos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = extensions,
		.pEnabledFeatures = NULL,
	};
	
	res = vk_CreateDevice(vk_device_physical, &device_create_info, NULL, &vk_device);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateDevice unsuccessful (%i)", res);
		goto ret;
	}
	
	erase(device_queue_create_infos);
	
	#define VK_FN_SYM_DEVICE
	#include "com_vulkan_fn.inl"
	
	vk_GetDeviceQueue(vk_device, vk_queue_index_graphics, 0, &vk_queue_graphics);
	vk_GetDeviceQueue(vk_device, vk_queue_index_present, 0, &vk_queue_present);
	
	stat = true;
	ret:
	if (device_queue_create_infos) free(device_queue_create_infos);
	if (queue_families) free(queue_families);
	if (device_extensions) free(device_extensions);
	if (physical_devices) free(physical_devices);
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_create_semaphores(void) {
	bool stat = false;
	VkResult res;
	
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
	};
	
	res = vk_CreateSemaphore(vk_device, &semaphore_create_info, NULL, &vk_sem_imgavail);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateSemaphore unsuccessful (%i)", res);
		goto ret;
	}
	
	res = vk_CreateSemaphore(vk_device, &semaphore_create_info, NULL, &vk_sem_rndcomp);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateSemaphore unsuccessful (%i)", res);
		goto ret;
	}
	
	stat = true;
	ret:
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_init_swapchain(void) {
	bool stat = false;
	VkResult res;
	
	VkSurfaceFormatKHR * restrict surface_formats = NULL;
	VkPresentModeKHR * restrict present_modes = NULL;
	
	vk_DeviceWaitIdle(vk_device);
	
	VkSurfaceCapabilitiesKHR surface_capabilities;
	res = vk_GetPhysicalDeviceSurfaceCapabilitiesKHR(vk_device_physical, vk_surface, &surface_capabilities);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR unsuccessful (%i)", res);
		goto ret;
	}
	
	if (!(surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		com_print_error("VK_IMAGE_USAGE_TRANSFER_DST_BIT unsupported, required");
		goto ret;
	}
	
	uint32_t surface_formats_count;
	res = vk_GetPhysicalDeviceSurfaceFormatsKHR(vk_device_physical, vk_surface, &surface_formats_count, NULL);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetPhysicalDeviceSurfaceFormatsKHR unsuccessful (%i)", res);
		goto ret;
	}
	if (surface_formats_count == 0) {
		com_print_error("surface has no formats! wat???");
		goto ret;
	}
	surface_formats = malloc(surface_formats_count *  sizeof(VkSurfaceFormatKHR));
	res = vk_GetPhysicalDeviceSurfaceFormatsKHR(vk_device_physical, vk_surface, &surface_formats_count, surface_formats);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetPhysicalDeviceSurfaceFormatsKHR unsuccessful (%i)", res);
		goto ret;
	}
	
	uint32_t present_modes_count;
	res = vk_GetPhysicalDeviceSurfacePresentModesKHR(vk_device_physical, vk_surface, &present_modes_count, NULL);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetPhysicalDeviceSurfacePresentModesKHR unsuccessful (%i)", res);
		goto ret;
	}
	present_modes = malloc(present_modes_count *  sizeof(present_modes));
	res = vk_GetPhysicalDeviceSurfacePresentModesKHR(vk_device_physical, vk_surface, &present_modes_count, present_modes);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetPhysicalDeviceSurfacePresentModesKHR unsuccessful (%i)", res);
		goto ret;
	}
	
	VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	char const * spmstr = "FIFO";
	
	for (uint32_t i = 0; i < present_modes_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			selected_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			spmstr = "Mailbox";
			break;
		}
	}
	
	erase(present_modes);
	
	uint32_t selected_swapchain_image_count;
	switch(selected_present_mode) {
	case VK_PRESENT_MODE_MAILBOX_KHR:
		selected_swapchain_image_count = clamp(3, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
		break;
	default:
		selected_swapchain_image_count = clamp(2, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
		break;
	}
	
	VkExtent2D selected_extent = surface_capabilities.currentExtent;
	if (selected_extent.width == (uint32_t)-1) {
		selected_extent.width = 800;
		selected_extent.height = 600;
	}
	selected_extent.width = clamp(selected_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
	selected_extent.height = clamp(selected_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
	
	VkSurfaceFormatKHR selected_format = surface_formats[0];
	if (selected_format.format == VK_FORMAT_UNDEFINED) {
		selected_format.format = VK_FORMAT_R8G8B8A8_UNORM;
		selected_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	} else if (surface_formats_count > 1) {
		for (uint32_t i = 0; i < surface_formats_count; i++) {
			if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
				selected_format = surface_formats[i];
				break;
			}
		}
	}
	
	vk_swapchain_format = selected_format.format;
	
	erase(surface_formats);
	
	VkSwapchainKHR vk_swapchain_old = vk_swapchain;
	
	VkSwapchainCreateInfoKHR swap_chain_create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = vk_surface,
		.minImageCount = selected_swapchain_image_count,
		.imageFormat = selected_format.format,
		.imageColorSpace = selected_format.colorSpace,
		.imageExtent = selected_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = selected_present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = vk_swapchain_old,
	};
	
	com_printf_info("creating swapchain: (M: %s, I: %u, F: %u, C: %u)", spmstr, selected_swapchain_image_count, selected_format.format, selected_format.colorSpace);
	
	res = vk_CreateSwapchainKHR(vk_device, &swap_chain_create_info, NULL, &vk_swapchain);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateSwapchainKHR unsuccessful (%i)", res);
		goto ret;
	}
	
	if (vk_swapchain_old) vk_DestroySwapchainKHR(vk_device, vk_swapchain_old, NULL);
	
	res = vk_GetSwapchainImagesKHR(vk_device, vk_swapchain, &vk_swapchain_images_count, NULL);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetSwapchainImagesKHR unsuccessful (%i)", res);
		goto ret;
	}
	vk_swapchain_images = malloc(vk_swapchain_images_count * sizeof(VkImage));
	res = vk_GetSwapchainImagesKHR(vk_device, vk_swapchain, &vk_swapchain_images_count, vk_swapchain_images);
	if (res != VK_SUCCESS) {
		com_printf_error("vkGetSwapchainImagesKHR unsuccessful (%i)", res);
		goto ret;
	}
	
	stat = true;
	ret:
	if (present_modes) free(present_modes);
	if (surface_formats) free(surface_formats);
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_init_command (void) {
	
	VkResult res;
	bool stat = false;
	
	VkCommandPoolCreateInfo command_pool_create = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = vk_queue_index_graphics,
	};
	
	res = vk_CreateCommandPool(vk_device, &command_pool_create, NULL, &vk_cmdpool_graphics);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateCommandPool unsuccessful (%i)", res);
		goto ret;
	}
	
	vk_cmdbufs_graphics_count = vk_swapchain_images_count;
	vk_cmdbufs_graphics = malloc(vk_swapchain_images_count * sizeof(VkCommandBuffer));
	VkCommandBufferAllocateInfo cmdbufs_allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = vk_cmdpool_graphics,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = vk_cmdbufs_graphics_count
	};
	res = vk_AllocateCommandBuffers(vk_device, &cmdbufs_allocate, vk_cmdbufs_graphics);
	if (res != VK_SUCCESS) {
		com_printf_error("vkAllocateCommandBuffers unsuccessful (%i)", res);
		goto ret;
	}
	
	VkCommandBufferBeginInfo cmdbuf_graphics_begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = NULL,
	};
	
	VkImageSubresourceRange subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
	
	VkClearValue clear_color = {
		.color.float32 = { 
			0.0f,
			0.125f,
			0.25f,
			0.0f,
		},
	};
	
	for (uint32_t i = 0; i < vk_cmdbufs_graphics_count; i++) {
		vk_BeginCommandBuffer(vk_cmdbufs_graphics[i], &cmdbuf_graphics_begin);
		if (vk_queue_index_graphics != vk_queue_index_present) {
			VkImageMemoryBarrier barrier_from_present_to_draw = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = NULL,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = vk_queue_index_present,
				.dstQueueFamilyIndex = vk_queue_index_graphics,
				.image = vk_swapchain_images[i],
				.subresourceRange = subresource_range,
			};
			vk_CmdPipelineBarrier(vk_cmdbufs_graphics[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_from_present_to_draw);
		}
		
		VkRenderPassBeginInfo renderpass_begin = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = NULL,
			.renderPass = vk_renderpass,
			.framebuffer = vk_framebuffers[i],
			.renderArea = {
				.offset = {
					.x = 0,
					.y = 0,
				},
				.extent = {
					.width = 800,
					.height = 600,
				},
			},
			.clearValueCount = 1,
			.pClearValues = &clear_color,
		};
		
		vk_CmdBeginRenderPass(vk_cmdbufs_graphics[i], &renderpass_begin, VK_SUBPASS_CONTENTS_INLINE);
		vk_CmdBindPipeline(vk_cmdbufs_graphics[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_graphics_pipeline);
		vk_CmdDraw(vk_cmdbufs_graphics[i], 3, 1, 0, 0);
		vk_CmdEndRenderPass(vk_cmdbufs_graphics[i]);
		
		if (vk_queue_index_graphics != vk_queue_index_present) {
			VkImageMemoryBarrier barrier_from_draw_to_present = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = NULL,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = vk_queue_index_graphics,
				.dstQueueFamilyIndex = vk_queue_index_present,
				.image = vk_swapchain_images[i],
				.subresourceRange = subresource_range,
			};
			vk_CmdPipelineBarrier(vk_cmdbufs_graphics[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_from_draw_to_present);
		}
		res = vk_EndCommandBuffer(vk_cmdbufs_graphics[i]);
		if (res != VK_SUCCESS) {
			com_printf_error("vkEndCommandBuffer unsuccessful (%i)", res);
			goto ret;
		}
	}
	
	stat = true;
	ret:
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

static bool vk_init_render(void) {
	
	bool stat = false;
	VkResult res;
	
	VkAttachmentDescription attachment_desc = {
		.flags = 0,
		.format = vk_swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	
	VkAttachmentReference attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	
	VkSubpassDescription subpass_desc = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachment_ref,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL,
	};
	
	VkRenderPassCreateInfo renderpass_create = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &attachment_desc,
		.subpassCount = 1,
		.pSubpasses = &subpass_desc,
		.dependencyCount = 0,
		.pDependencies = NULL,
	};
	
	res = vk_CreateRenderPass(vk_device, &renderpass_create, NULL, &vk_renderpass);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateRenderPass unsuccessful (%i)", res);
		goto ret;
	}
	
	vk_framebuffers_count = vk_imageviews_count = vk_swapchain_images_count;
	vk_imageviews = malloc(vk_imageviews_count * sizeof(VkImageView));
	vk_framebuffers = malloc(vk_framebuffers_count * sizeof(VkFramebuffer));
	
	for (uint32_t i = 0; i < vk_imageviews_count; i++) {
		VkImageViewCreateInfo imageview_create = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.image = vk_swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vk_swapchain_format,
			.components = {
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		
		res = vk_CreateImageView(vk_device, &imageview_create, NULL, vk_imageviews+i);
		if (res != VK_SUCCESS) {
			com_printf_error("vkCreateImageView unsuccessful (%i)", res);
			goto ret;
		}
			
		VkFramebufferCreateInfo framebuffer_create = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = vk_renderpass,
			.attachmentCount = 1,
			.pAttachments = vk_imageviews+i,
			.width = 800,
			.height = 600,
			.layers = 1,
		};
		
		res = vk_CreateFramebuffer(vk_device, &framebuffer_create, NULL, vk_framebuffers+i);
		if (res != VK_SUCCESS) {
			com_printf_error("vkCreateImageView unsuccessful (%i)", res);
			goto ret;
		}
	}
	
	char const * vert_spv = \
	"#version 400\n\n"\
	"void main() {"\
	"	vec2 pos[3] = vec2[3] (vec2(-0.7, 0.7), vec2(0.7, 0.7), vec2(0.0, -0.7));"\
	"	gl_Position = vec4( pos[gl_VertexIndex], 0.0, 1.0 );"\
	"}";

	char const * frag_spv = \
	"#version 400\n\n"\
	"layout(location = 0) out vec4 out_Color;\n"\
	"void main() {"\
	"	out_Color = vec4( 0.0, 0.4, 1.0, 1.0 );"\
	"}";
	
	VkShaderModule vk_shamod_vert;
	VkShaderModule vk_shamod_frag;
	
	VkPipelineLayout vk_pipeline_layout;
	
    VkPipelineLayoutCreateInfo layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
    };
	
	res = vk_CreatePipelineLayout(vk_device, &layout_create_info, NULL, &vk_pipeline_layout);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreatePipelineLayout unsuccessful (%i)", res);
		goto ret;
	}
	
	VkShaderModuleCreateInfo vert_module_create = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = strlen(vert_spv),
		.pCode = (uint32_t *) vert_spv,
	};
	
	res = vk_CreateShaderModule(vk_device, &vert_module_create, NULL, &vk_shamod_vert);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateShaderModule unsuccessful (%i)", res);
		goto ret;
	}
	
	VkShaderModuleCreateInfo frag_module_create = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = strlen(frag_spv),
		.pCode = (uint32_t *) frag_spv,
	};
	
	res = vk_CreateShaderModule(vk_device, &frag_module_create, NULL, &vk_shamod_frag);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateShaderModule unsuccessful (%i)", res);
		goto ret;
	}
	
	VkPipelineShaderStageCreateInfo shader_stage_create_infos [] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vk_shamod_vert,
			.pName = "main",
			.pSpecializationInfo = NULL,
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = vk_shamod_frag,
			.pName = "main",
			.pSpecializationInfo = NULL,
		},
	};
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL,
	};
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	
	VkViewport viewport = {
		.x = 0,
		.y = 0,
		.width = 800,
		.height = 600,
		.minDepth = 0,
		.maxDepth = 1,
	};
	
	VkRect2D scissor = {
		{0, 0},
		{800, 600},
	};
	
	VkPipelineViewportStateCreateInfo viewport_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};
	
	VkPipelineRasterizationStateCreateInfo rasterization_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
		.lineWidth = 1,
	};
	
	VkPipelineMultisampleStateCreateInfo multisample_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};
	
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, 
		.colorBlendOp = VK_BLEND_OP_ADD, 
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
	
	VkPipelineColorBlendStateCreateInfo color_blend_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment_state,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
	};
	
	VkGraphicsPipelineCreateInfo pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stageCount = 2,
		.pStages = shader_stage_create_infos,
		.pVertexInputState = &vertex_input_state_create,
		.pInputAssemblyState = &input_assembly_state_create,
		.pTessellationState = NULL,
		.pViewportState = &viewport_state_create,
		.pRasterizationState = &rasterization_state_create,
		.pMultisampleState = &multisample_state_create,
		.pDepthStencilState = NULL,
		.pColorBlendState = &color_blend_state_create,
		.pDynamicState = NULL,
		.layout = vk_pipeline_layout,
		.renderPass = vk_renderpass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};
	
	res = vk_CreateGraphicsPipelines(vk_device, NULL, 1, &pipeline_create_info, NULL, &vk_graphics_pipeline);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateGraphicsPipelines unsuccessful (%i)", res);
		goto ret;
	}
	
	stat = true;
	ret:
	return stat;
}

//================================================================
//----------------------------------------------------------------
//================================================================

bool com_vk_init (void) {
	if (!vk_load()) {
		com_print_error("failed to load vulkan library");
		return false;
	}
	if (!vk_init_instance()) {
		com_print_error("failed to initialize vulkan instance");
		return false;
	}
	if (!vk_create_surface()) {
		com_print_error("failed to create vulkan surface");
		return false;
	}
	if (!vk_init_device()) {
		com_print_error("failed to initialize vulkan device");
		return false;
	}
	if (!vk_create_semaphores()) {
		com_print_error("failed to create semaphores");
		return false;
	}
	if (!vk_init_swapchain()) {
		com_print_error("failed to initialize swapchain");
		return false;
	}
	if (!vk_init_render()) {
		com_print_error("failed to initialize render structures");
		return false;
	}
	if (!vk_init_command()) {
		com_print_error("failed to initialize command structures");
		return false;
	}
	
	return true;
}

//================================================================
//----------------------------------------------------------------
//================================================================

void com_vk_term (void) {
	if (vk_framebuffers) {
		for (uint32_t i = 0; i < vk_framebuffers_count; i++) {
			vk_DestroyFramebuffer(vk_device, vk_framebuffers[i], NULL);
		}
		free(vk_framebuffers);
	}
	if (vk_imageviews) {
		for (uint32_t i = 0; i < vk_imageviews_count; i++) {
			vk_DestroyImageView(vk_device, vk_imageviews[i], NULL);
		}
		free(vk_imageviews);
	}
	//if (vk_cmdbufs_present) {
	//	vk_FreeCommandBuffers(vk_device, vk_cmdpool_present, vk_cmdbufs_present_count, vk_cmdbufs_present);
	//	free(vk_cmdbufs_present);
	//}
	//if (vk_cmdpool_present) vk_DestroyCommandPool(vk_device, vk_cmdpool_present, NULL);
	if (vk_swapchain_images) free(vk_swapchain_images);
	if (vk_swapchain) vk_DestroySwapchainKHR(vk_device, vk_swapchain, NULL);
	if (vk_sem_imgavail) vk_DestroySemaphore(vk_device, vk_sem_imgavail, NULL);
	if (vk_sem_rndcomp) vk_DestroySemaphore(vk_device, vk_sem_rndcomp, NULL);
	if (vk_device) vk_DestroyDevice(vk_device, NULL);
	if (vk_surface) vk_DestroySurfaceKHR(vk_instance, vk_surface, NULL);
	if (vk_instance) vk_DestroyInstance(vk_instance, NULL);
	if (vk_handle) dlclose(vk_handle);
}

//================================================================
//----------------------------------------------------------------
//================================================================

void com_vk_swap (void) {
	
	VkResult res;
	
	uint32_t current_image_index;
	res = vk_AcquireNextImageKHR(vk_device, vk_swapchain, UINT64_MAX, vk_sem_imgavail, NULL, &current_image_index);
	if (res != VK_SUCCESS) {
		com_printf_error("vkAcquireNextImageKHR unsuccessful (%i)", res);
		return;
	}
	
	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_sem_imgavail,
		.pWaitDstStageMask = &wait_dst_stage_mask,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_cmdbufs_graphics[current_image_index],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vk_sem_rndcomp
    };
	
	res = vk_QueueSubmit(vk_queue_present, 1, &submit_info, NULL);
	if (res != VK_SUCCESS) {
		com_printf_error("vkQueueSubmit unsuccessful (%i)", res);
		return;
	}
	
    VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_sem_rndcomp,
		.swapchainCount = 1,
		.pSwapchains = &vk_swapchain,
		.pImageIndices = &current_image_index,
		.pResults = NULL,
    };
	
	res = vk_QueuePresentKHR(vk_queue_present, &present_info);
	if (res != VK_SUCCESS) {
		com_printf_error("vkQueuePresentKHR unsuccessful (%i)", res);
		return;
	};

}
