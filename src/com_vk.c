#include "com_internal.h"

#include <dlfcn.h>

#define VK_FN_IDECL
#include "com_vk_func.inl"

#define VKR(call) res = call; if (res != VK_SUCCESS) {com_printf_error(#call" unsuccessful (%i)", res); goto finalize;}

static char const * const instance_extensions [] = { "VK_KHR_surface", "VK_KHR_xcb_surface",
#ifdef PROGENY_DEBUG
	"VK_EXT_debug_report"
#endif
};
static size_t const instance_extensions_count = sizeof(instance_extensions) / sizeof(instance_extensions[0]);

static char const * const instance_layers [] = {
#ifdef PROGENY_DEBUG
	"VK_LAYER_LUNARG_standard_validation"
#endif
};
static size_t const instance_layers_count = sizeof(instance_layers) / sizeof(instance_layers[0]);

#ifdef PROGENY_DEBUG
VkDebugReportCallbackEXT vk_debug_cb_handle = VK_NULL_HANDLE;
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_cb(VkDebugReportFlagsEXT flags unu, VkDebugReportObjectTypeEXT objectType unu, uint64_t object unu, size_t location unu, int32_t messageCode unu, char const * pLayerPrefix unu, char const * pMessage, void * pUserData unu) {
    com_printf_level("%s", VKVALID, pMessage);
    return VK_FALSE;
}
#endif

static void * restrict vk_handle = NULL;
static VkInstance vk_instance = VK_NULL_HANDLE;
static VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

static uint32_t vk_physical_devices_count = 0;
static com_vk_physical_device_t * vk_physical_devices = NULL;

bool com_vk_init() {
	
	bool stat = false;
	VkResult res;
	
	VkExtensionProperties * instance_supported_extensions = NULL;
	VkLayerProperties * instance_supported_layers = NULL;
	VkPhysicalDevice *  physical_devices = NULL;
	
	vk_handle = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_NOW);
	if (!vk_handle) {
		com_print_error("could not find libvulkan.so or could not read from it");
		goto finalize;
	}
	
	#define VK_FN_SYM_GLOBAL
	#include "com_vk_func.inl"
	
	uint32_t extension_count;
	VKR(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL))
	instance_supported_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
	VKR(vkEnumerateInstanceExtensionProperties(NULL, &extension_count, instance_supported_extensions))
	
	for (size_t e = 0; e < instance_extensions_count; e++) {
		bool sup = false;
		for (uint32_t i = 0; i < extension_count; i++) {
			if (!strcmp(instance_supported_extensions[i].extensionName, instance_extensions[e])) sup = true;
		}
		if (!sup) {
			com_printf_error("required extension \"%s\" not supported", instance_extensions[e]);
			goto finalize;
		}
	}
	
	uint32_t layer_count;
	VKR(vkEnumerateInstanceLayerProperties(&layer_count, NULL))
	instance_supported_layers = malloc(layer_count * sizeof(VkLayerProperties));
	VKR(vkEnumerateInstanceLayerProperties(&layer_count, instance_supported_layers))
	
	for (size_t e = 0; e < instance_layers_count; e++) {
		bool sup = false;
		for (uint32_t i = 0; i < layer_count; i++) {
			if (!strcmp(instance_supported_layers[i].layerName, instance_layers[e])) sup = true;
		}
		if (!sup) {
			com_printf_error("required layer \"%s\" not supported", instance_layers[e]);
			goto finalize;
		}
	}
	
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Progeny",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.pEngineName = "Progeny",
		.engineVersion = VK_MAKE_VERSION(0, 0, 1),
		.apiVersion = VK_MAKE_VERSION(1, 0, 21),
	};
	
	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = instance_layers_count,
		.ppEnabledLayerNames = instance_layers,
		.enabledExtensionCount = instance_extensions_count,
		.ppEnabledExtensionNames = instance_extensions,
	};
	
	VKR(vkCreateInstance(&instance_create_info, NULL, &vk_instance))
	
	#define VK_FN_SYM_INSTANCE
	#include "com_vk_func.inl"
	
	#ifdef PROGENY_DEBUG
	VkDebugReportCallbackCreateInfoEXT callback_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
		.pfnCallback = vk_debug_cb,
		.pUserData = NULL,
	};
	VKR(vkCreateDebugReportCallbackEXT(vk_instance, &callback_create_info, NULL, &vk_debug_cb_handle))
	#endif
	
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = com_xcb_connection,
		.window = com_xcb_window,
    };
	
	VKR(vkCreateXcbSurfaceKHR(vk_instance, &surface_create_info, NULL, &vk_surface))
	
	VKR(vkEnumeratePhysicalDevices(vk_instance, &vk_physical_devices_count, NULL))
	physical_devices = malloc(vk_physical_devices_count * sizeof(VkPhysicalDevice));
	vk_physical_devices = calloc(1, vk_physical_devices_count * sizeof(com_vk_physical_device_t));
	VKR(vkEnumeratePhysicalDevices(vk_instance, &vk_physical_devices_count, physical_devices))
	
	com_vk_physical_device_t * cpdev = NULL;
	for (uint32_t i = 0; i < vk_physical_devices_count; i++) {
		cpdev = vk_physical_devices + i;
		cpdev->pdev = physical_devices[i];
		vkGetPhysicalDeviceProperties(cpdev->pdev, &cpdev->pdev_props);
	}
	
	stat = true;
	finalize:
	if (physical_devices) free(physical_devices);
	if (instance_supported_extensions) free(instance_supported_extensions);
	if (instance_supported_layers) free(instance_supported_layers);
	return stat;
}

static void com_vk_physical_device_cleanup(com_vk_physical_device_t * cpdev) {
	if (cpdev->queuefam_props) free(cpdev->queuefam_props);
	if (cpdev->sup_ext) free(cpdev->sup_ext);
}

void com_vk_term() {
	if (vk_physical_devices) {
		for (uint32_t i = 0; i < vk_physical_devices_count; i++) {
			com_vk_physical_device_cleanup(vk_physical_devices + i);
		}
		free(vk_physical_devices);
		vk_physical_devices = NULL;
	}
	vk_physical_devices_count = 0;
	if (vk_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
		vk_surface = VK_NULL_HANDLE;
	}
	#ifdef PROGENY_DEBUG
	if (vk_debug_cb_handle) {
		vkDestroyDebugReportCallbackEXT(vk_instance, vk_debug_cb_handle, NULL);
		vk_debug_cb_handle = VK_NULL_HANDLE;
	}
	#endif
	if (vk_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(vk_instance, NULL);
		vk_instance = VK_NULL_HANDLE;
	}
	if (vk_handle) {
		dlclose(vk_handle);
		vk_handle = NULL;
	}
}
