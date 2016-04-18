#include "com_internal.h"

#include <dlfcn.h>

#define VK_FN_DECL
#include "com_vulkan_fn.inl"

/*
    if(!CreateDevice()) {
      return false;
    }
    if(!LoadDeviceLevelEntryPoints()) {
      return false;
    }
    if(!GetDeviceQueue()) {
      return false;
    }
    if(!CreateSemaphores()) {
      return false;
    }
    return true;
*/

//================================================================
//----------------------------------------------------------------
//================================================================

static void * restrict vk_handle;

static VkInstance vk_instance = NULL;
static VkSurfaceKHR vk_surface = NULL;
static VkDevice vk_device = NULL;

static uint32_t vk_queue_index_graphics = 0;
static uint32_t vk_queue_index_presentation = 0;

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
		com_printf_error("vkEnumerateInstanceExtensionProperties unsuccessful (%u)", res);
		goto ret;
	};
	supported_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
	res = vk_EnumerateInstanceExtensionProperties(NULL, &extension_count, supported_extensions);
	if (res != VK_SUCCESS) {
		com_printf_error("vkEnumerateInstanceExtensionProperties unsuccessful (%u)", res);
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
		com_printf_error("vkCreateInstance unsuccessful (%u)", res);
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
		com_printf_error("vkCreateXcbSurfaceKHR unsuccessful (%u)", res);
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
		com_printf_error("vkEnumeratePhysicalDevices unsuccessful (%u)", res);
		goto ret;
	}
	physical_devices = malloc(physical_device_count * sizeof(VkPhysicalDevice));
	res = vk_EnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices);
	if (res != VK_SUCCESS) {
		com_printf_error("vkEnumeratePhysicalDevices unsuccessful (%u)", res);
		goto ret;
	}
	
	VkPhysicalDevice selected_physical_device = NULL;
	
	for (uint32_t d = 0; d < physical_device_count; d++) {
		
		VkPhysicalDeviceProperties pdp;
		uint32_t device_extensions_count;
		bool swapchain_supported = false;
		
		if (queue_families) erase(queue_families);
		if (device_extensions) erase(device_extensions);
		
		vk_GetPhysicalDeviceProperties(physical_devices[d], &pdp);
		
		res = vk_EnumerateDeviceExtensionProperties(physical_devices[d], NULL, &device_extensions_count, NULL);
		if (res != VK_SUCCESS) {
			com_printf_debug("skipping %s: vk_EnumerateDeviceExtensionProperties unsuccessful (%u)", pdp.deviceName, res);
			continue;
		}
		device_extensions = malloc(device_extensions_count * sizeof(VkExtensionProperties));
		res = vk_EnumerateDeviceExtensionProperties(physical_devices[d], NULL, &device_extensions_count, device_extensions);
		if (res != VK_SUCCESS) {
			com_printf_debug("skipping %s: vk_EnumerateDeviceExtensionProperties unsuccessful (%u)", pdp.deviceName, res);
			continue;
		}
		
		for (uint32_t e = 0; e < device_extensions_count; e++) {
			if (!strcmp("VK_KHR_swapchain", device_extensions[e].extensionName)) swapchain_supported = true;
		}
		
		erase(device_extensions);
		
		if (!swapchain_supported) {
			com_printf_debug("skipping %s: VK_KHR_swapchain unsupported", pdp.deviceName);
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
		vk_queue_index_presentation = UINT32_MAX;
		
		vk_GetPhysicalDeviceQueueFamilyProperties(physical_devices[d], &queue_families_count, queue_families);
		for (uint32_t q = 0; q < queue_families_count; q++) {
			if (queue_families[q].queueFlags | VK_QUEUE_GRAPHICS_BIT) vk_queue_index_graphics = q;
			VkBool32 present_supported;
			vk_GetPhysicalDeviceSurfaceSupportKHR(physical_devices[d], q, vk_surface, &present_supported);
			if (present_supported) vk_queue_index_presentation = q;
			if (vk_queue_index_graphics != UINT32_MAX || vk_queue_index_presentation != UINT32_MAX) break;
		}
		
		if (vk_queue_index_graphics == UINT32_MAX) {
			com_printf_debug("skipping %s: no queue families support graphics", pdp.deviceName);
			continue;
		}
		
		if (vk_queue_index_presentation == UINT32_MAX) {
			com_printf_debug("skipping %s: no queue families support presentation", pdp.deviceName);
			continue;
		}
		
		erase(queue_families);
		
		com_printf_info("selecting device: %s (G: %u, P: %u)", pdp.deviceName, vk_queue_index_graphics, vk_queue_index_presentation);
		selected_physical_device = physical_devices[d];
		break;
	}
	
	if (!selected_physical_device) {
		com_print_error("no compatible vulkan devices found");
		goto ret;
	}
	
	uint32_t device_queue_create_info_count = vk_queue_index_graphics == vk_queue_index_presentation ? 1 : 2;
	device_queue_create_infos = malloc(device_queue_create_info_count * sizeof(VkDeviceQueueCreateInfo));
	
	float const queue_priorities [] = { 1.0f };
	
	device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_infos[0].pNext = NULL;
	device_queue_create_infos[0].flags = 0;
	device_queue_create_infos[0].queueFamilyIndex = vk_queue_index_graphics;
	device_queue_create_infos[0].queueCount = 1;
	device_queue_create_infos[0].pQueuePriorities = queue_priorities;
	
	if (vk_queue_index_graphics != vk_queue_index_presentation) {
		device_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_infos[1].pNext = NULL;
		device_queue_create_infos[1].flags = 0;
		device_queue_create_infos[1].queueFamilyIndex = vk_queue_index_presentation;
		device_queue_create_infos[1].queueCount = 1;
		device_queue_create_infos[1].pQueuePriorities = queue_priorities;
	}
	
	static char const * const extensions [] = { "VK_KHR_swapchain" };
	
	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = device_queue_create_info_count,
		.pQueueCreateInfos = device_queue_create_infos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = extensions,
		.pEnabledFeatures = NULL,
	};
	
	res = vk_CreateDevice(selected_physical_device, &device_create_info, NULL, &vk_device);
	if (res != VK_SUCCESS) {
		com_printf_error("vkCreateDevice unsuccessful (%u)", res);
		goto ret;
	}
	
	erase(device_queue_create_infos);
	
	#define VK_FN_SYM_DEVICE
	#include "com_vulkan_fn.inl"
	
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
	return true;
}

//================================================================
//----------------------------------------------------------------
//================================================================

void com_vk_term (void) {
	if (vk_device) vk_DestroyDevice(vk_device, NULL);
	if (vk_surface) vk_DestroySurfaceKHR(vk_instance, vk_surface, NULL);
	if (vk_instance) vk_DestroyInstance(vk_instance, NULL);
	dlclose(vk_handle);
}

//================================================================
//----------------------------------------------------------------
//================================================================

void com_vk_swap (void) {
	
}
