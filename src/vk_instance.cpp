#include "vk.hpp"
#include "control.hpp"

#include <dlfcn.h>

//================================================================
#define VK_FN_IDECL
#include "vk_functions.inl"
//================================================================
static constexpr char const * const instance_extensions [] = { "VK_KHR_surface", "VK_KHR_xcb_surface",
#ifdef PROGENY_VK_DEBUG
	"VK_EXT_debug_report"
#endif
};
static constexpr size_t instance_extensions_count = sizeof(instance_extensions) / sizeof(instance_extensions[0]);
//================================================================
static constexpr char const * const instance_layers [] = {
#ifdef PROGENY_VK_DEBUG
	"VK_LAYER_LUNARG_standard_validation",
#endif
};
static constexpr size_t instance_layers_count = sizeof(instance_layers) / sizeof(instance_layers[0]);
//================================================================

static void * vk_handle = nullptr;
static VkInstance vk_instance = VK_NULL_HANDLE;
VkSurfaceKHR vk::surface = VK_NULL_HANDLE;

static std::vector<vk::physical_device> physical_devices;

//================================================================
template <typename FUNC> FUNC vk_instance_func(char const * symbol) {
	FUNC f = reinterpret_cast<FUNC>(vkGetInstanceProcAddr(vk_instance, symbol));
	if (!f) srcthrow("unable to get required function from vulkan instance");
	return f;
}
#define VKISYM(func) func = vk_instance_func<PFN_##func>(#func)
//================================================================
#define VKR(call) res = call; if (res != VK_SUCCESS) srcthrow("\"%s\" unsuccessful: (%i)", #call, res);

void vk::instance::init() {
	vk_handle = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_NOW);
	if (!vk_handle) srcthrow("could not open vulkan library");
	vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(vk_handle, "vkGetInstanceProcAddr"));
	if (!vkGetInstanceProcAddr) srcthrow("could not find vkGetInstanceProcAddr in loaded vulkan library");
	
	#define VK_FN_SYM_GLOBAL
	#include "vk_functions.inl"
	
	VkResult res;

	uint32_t supext_cnt;
	VKR(vkEnumerateInstanceExtensionProperties(nullptr, &supext_cnt, nullptr))
	std::vector<VkExtensionProperties> supext {supext_cnt};
	VKR(vkEnumerateInstanceExtensionProperties(nullptr, &supext_cnt, supext.data()))
	
	for (char const * ext : instance_extensions) {
		bool c = false;
		for (VkExtensionProperties const & ep : supext) {
			if (!strcmp(ext, ep.extensionName)) c = true;
		}
		if (!c) srcthrow("required extension \"%s\" unsupported", ext);
	}
	
	uint32_t suplay_cnt;
	VKR(vkEnumerateInstanceLayerProperties(&suplay_cnt, nullptr))
	std::vector<VkLayerProperties> suplay {suplay_cnt};
	VKR(vkEnumerateInstanceLayerProperties(&suplay_cnt, suplay.data()))
	
	for (char const * ext : instance_layers) {
		bool c = false;
		for (VkLayerProperties const & ep : suplay) {
			if (!strcmp(ext, ep.layerName)) c = true;
		}
		if (!c) srcthrow("required layer \"%s\" unsupported", ext);
	}
	
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Progeny",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.pEngineName = "Progeny",
		.engineVersion = VK_MAKE_VERSION(0, 0, 1),
		.apiVersion = VK_MAKE_VERSION(1, 0, 21),
	};
	
	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = instance_layers_count,
		.ppEnabledLayerNames = instance_layers,
		.enabledExtensionCount = instance_extensions_count,
		.ppEnabledExtensionNames = instance_extensions,
	};
	
	VKR(vkCreateInstance(&instance_create_info, nullptr, &vk_instance))
	
	#define VK_FN_SYM_INSTANCE
	#include "vk_functions.inl"
	
	NativeSurfaceCreateInfo surface_create_info = control::vk_surface_create_info();
	VKR(vkCreateXcbSurfaceKHR(vk_instance, &surface_create_info, NULL, &vk::surface))
	
	uint32_t pdev_cnt;
	VKR(vkEnumeratePhysicalDevices(vk_instance, &pdev_cnt, NULL))
	std::vector<VkPhysicalDevice> pdevs {pdev_cnt};
	VKR(vkEnumeratePhysicalDevices(vk_instance, &pdev_cnt, pdevs.data()))
	
	for (VkPhysicalDevice & pdev : pdevs) {
		try {
			physical_device pdev2 {pdev};
			physical_devices.push_back(std::move(pdev2));
		} catch (com::exception & e) {
			com::printf("WARNING: a physical device could not be resolved: \"%s\"", e.msg.c_str());
			continue;
		}
	}
}

void vk::instance::term() noexcept {
	if (vk::surface) {
		vkDestroySurfaceKHR(vk_instance, vk::surface, nullptr);
		vk::surface = VK_NULL_HANDLE;
	}
	if (vk_instance) {
		vkDestroyInstance(vk_instance, nullptr);
		vk_instance = VK_NULL_HANDLE;
	}
	if (vk_handle) {
		dlclose(vk_handle);
		vk_handle = nullptr;
	}
}

VkDevice vk::instance::create_device(physical_device const & pdev, VkDeviceCreateInfo * info) {
	VkResult res; VkDevice dev;
	VKR(vkCreateDevice(pdev.handle, info, NULL, &dev))
	return dev;
}

void vk::instance::resolve_device_symbols(device & dev) {
	#define VK_FN_SYM_DEVICE
	#include "vk_functions.inl"
}

vk::physical_device::physical_device(VkPhysicalDevice & handle) : handle(handle) {
	VkResult res;
	vkGetPhysicalDeviceProperties(handle, &properties);
	uint32_t num;
	VKR(vkEnumerateDeviceExtensionProperties(handle, nullptr, &num, nullptr))
	this->extensions.resize(num);
	VKR(vkEnumerateDeviceExtensionProperties(handle, nullptr, &num, this->extensions.data()))
	VKR(vkEnumerateDeviceLayerProperties(handle, &num, nullptr))
	this->layers.resize(num);
	VKR(vkEnumerateDeviceLayerProperties(handle, &num, this->layers.data()))
	vkGetPhysicalDeviceQueueFamilyProperties(handle, &num, nullptr);
	this->queue_families.resize(num);
	vkGetPhysicalDeviceQueueFamilyProperties(handle, &num, this->queue_families.data());
	vkGetPhysicalDeviceMemoryProperties(handle, &this->memory_properties);
	
	this->queue_families_presentable.resize(num);
	for (uint32_t i = 0; i < this->queue_families.size(); i++) {
		vkGetPhysicalDeviceSurfaceSupportKHR(handle, i, vk::surface, &this->queue_families_presentable[i]);
	}
	
	bool has_dmt = false;
	VkMemoryPropertyFlags best_smt = 0;
	for (uint32_t i = 0; i < this->memory_properties.memoryTypeCount; i++) {
		if ((this->memory_properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			this->memory_index_staging = i;
			this->memory_index_device = i;
			best_smt = this->memory_properties.memoryTypes[i].propertyFlags;
			has_dmt = true;
			break;
		}
		if (!has_dmt && (this->memory_properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))) {
			this->memory_index_device = i;
			has_dmt = true;
		}
		if (!best_smt && (this->memory_properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
			this->memory_index_staging = i;
			best_smt = this->memory_properties.memoryTypes[i].propertyFlags;
		}
		if (!(best_smt & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) && ((this->memory_properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT))) {
			this->memory_index_staging = i;
			best_smt = this->memory_properties.memoryTypes[i].propertyFlags;
		}
	}
	
	if (!has_dmt) srcthrow("physical device has no device memory");
	if (!best_smt) srcthrow("physical device has no host visible memory");
}


std::vector<vk::physical_device> const & vk::get_physical_devices() {
	return physical_devices;
}
