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
static std::vector<vk::physical_device> physical_devices;

//================================================================
VkSurfaceKHR vk::surface::handle = VK_NULL_HANDLE;
static VkSurfaceCapabilitiesKHR surface_capabilities;
static std::vector<VkSurfaceFormatKHR> surface_formats;
static std::vector<VkPresentModeKHR> surface_present_modes;
VkSurfaceCapabilitiesKHR const & vk::surface::capabilities {surface_capabilities};
std::vector<VkSurfaceFormatKHR> const & vk::surface::formats {surface_formats};
std::vector<VkPresentModeKHR> const & vk::surface::present_modes {surface_present_modes};
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
	
	NativeSurfaceCreateInfo surface_create_info = vk::surface::create_info();
	VKR(vkCreateXcbSurfaceKHR(vk_instance, &surface_create_info, NULL, &vk::surface::handle))
	
	uint32_t pdev_cnt;
	VKR(vkEnumeratePhysicalDevices(vk_instance, &pdev_cnt, NULL))
	std::vector<VkPhysicalDevice> pdevs {pdev_cnt};
	VKR(vkEnumeratePhysicalDevices(vk_instance, &pdev_cnt, pdevs.data()))

	for (VkPhysicalDevice & pdev : pdevs) {
		try {
			physical_devices.emplace_back(pdev);
		} catch (com::exception & e) {
			com::printf("WARNING: a physical device could not be resolved: \"%s\"", e.msg.c_str());
			continue;
		}
	}
}

void vk::instance::term() noexcept {
	if (vk::surface::handle) {
		vkDestroySurfaceKHR(vk_instance, vk::surface::handle, nullptr);
		vk::surface::handle = VK_NULL_HANDLE;
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
	
	if (dev.overall_capability & device::capability::presentable) {
		#define VK_FN_SYM_SWAPCHAIN
		#include "vk_functions.inl"
	}
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
		vkGetPhysicalDeviceSurfaceSupportKHR(handle, i, vk::surface::handle, &this->queue_families_presentable[i]);
	}
	
	struct best_mem {
		physical_device const & parent;
		uint32_t index;
		VkMemoryType const * type = nullptr;
		VkMemoryHeap const * heap = nullptr;
		
		operator bool() {return type && heap;}
		best_mem(physical_device const & parent, uint32_t i) : parent(parent), index(i), type(&parent.memory_properties.memoryTypes[i]), heap(&parent.memory_properties.memoryHeaps[type->heapIndex]) {}
		void set(uint32_t new_index) {
			index = new_index;
			type = &parent.memory_properties.memoryTypes[index];
			heap = &parent.memory_properties.memoryHeaps[type->heapIndex];
		}
	};
	
	best_mem best_host_large {*this, 0};
	best_mem best_staging {*this, 0};
	best_mem best_device_ideal {*this, 0};
	best_mem best_device_large {*this, 0};
	
	for (uint32_t i = 1; i < this->memory_properties.memoryTypeCount; i++) {
		VkMemoryType & type = this->memory_properties.memoryTypes[i];
		VkMemoryHeap & heap = this->memory_properties.memoryHeaps[type.heapIndex];
		
		do { //host_large
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != (best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) best_host_large.set(i);
				break;
			}
			if (heap.size != best_host_large.heap->size) {
				if (heap.size > best_host_large.heap->size) best_host_large.set(i);
				break;
			}
			if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != (best_host_large.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
				if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) best_host_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != (best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) best_host_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != (best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) best_host_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != (best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) best_host_large.set(i);
				break;
			}
		} while(0);
		
		do { //staging
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != (best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) best_staging.set(i);
				break;
			}
			if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != (best_staging.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
				if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) best_staging.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != (best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) best_staging.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != (best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) best_staging.set(i);
				break;
			}
			if (heap.size != best_staging.heap->size) {
				if (heap.size > best_staging.heap->size) best_staging.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != (best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) best_staging.set(i);
				break;
			}
		} while(0);
		
		do { //device_ideal
			if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != (best_device_ideal.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
				if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) best_device_ideal.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != (best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) best_device_ideal.set(i);
				break;
			}
			if (heap.size != best_device_ideal.heap->size) {
				if (heap.size > best_device_ideal.heap->size) best_device_ideal.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != (best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) best_device_ideal.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != (best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) best_device_ideal.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != (best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) best_device_ideal.set(i);
				break;
			}
		} while(0);
		
		do { //device_large
			if ((heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != (best_device_large.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
				if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) best_device_large.set(i);
				break;
			}
			if (heap.size != best_device_large.heap->size) {
				if (heap.size > best_device_large.heap->size) best_device_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != (best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) best_device_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != (best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) best_device_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != (best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) best_device_large.set(i);
				break;
			}
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != (best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) best_device_large.set(i);
				break;
			}
		} while(0);
	}
	
	if (!best_host_large) srcthrow("physical device has no device memory");
	srcprintf_debug("%s: host_large (index %u): heap (flags: %u, device: %s, size: %zu), type (flags: %u, host: %s, device: %s, coherent: %s, cached: %s)", 
		this->properties.deviceName, 
		best_host_large.index, static_cast<unsigned int>(best_host_large.heap->flags), 
		best_host_large.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "yes" : "no",
		static_cast<size_t>(best_host_large.heap->size),
		static_cast<unsigned int>(best_host_large.type->propertyFlags),
		best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "yes" : "no",
		best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "yes" : "no",
		best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "yes" : "no",
		best_host_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "yes" : "no"	 
	);
	
	if (!best_staging) srcthrow("physical device has no device memory");
	srcprintf_debug("%s: staging (index %u): heap (flags: %u, device: %s, size: %zu), type (flags: %u, host: %s, device: %s, coherent: %s, cached: %s)", 
		this->properties.deviceName, 
		best_staging.index, static_cast<unsigned int>(best_staging.heap->flags), 
		best_staging.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "yes" : "no",
		static_cast<size_t>(best_staging.heap->size),
		static_cast<unsigned int>(best_staging.type->propertyFlags),
		best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "yes" : "no",
		best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "yes" : "no",
		best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "yes" : "no",
		best_staging.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "yes" : "no"	 
	);
	
	if (!best_device_ideal) srcthrow("physical device has no device memory");
	srcprintf_debug("%s: device_ideal (index %u): heap (flags: %u, device: %s, size: %zu), type (flags: %u, host: %s, device: %s, coherent: %s, cached: %s)", 
		this->properties.deviceName, 
		best_device_ideal.index, static_cast<unsigned int>(best_device_ideal.heap->flags), 
		best_device_ideal.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "yes" : "no",
		static_cast<size_t>(best_device_ideal.heap->size),
		static_cast<unsigned int>(best_device_ideal.type->propertyFlags),
		best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "yes" : "no",
		best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "yes" : "no",
		best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "yes" : "no",
		best_device_ideal.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "yes" : "no"	 
	);
	
	if (!best_device_large) srcthrow("physical device has no device memory");
	srcprintf_debug("%s: device_large (index %u): heap (flags: %u, device: %s, size: %zu), type (flags: %u, host: %s, device: %s, coherent: %s, cached: %s)", 
		this->properties.deviceName, 
		best_device_large.index, static_cast<unsigned int>(best_device_large.heap->flags), 
		best_device_large.heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "yes" : "no",
		static_cast<size_t>(best_device_large.heap->size),
		static_cast<unsigned int>(best_device_large.type->propertyFlags),
		best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "yes" : "no",
		best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "yes" : "no",
		best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "yes" : "no",
		best_device_large.type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "yes" : "no"	 
	);
	
	mem.host_large = best_host_large.index;
	mem.staging = best_staging.index;
	mem.device_ideal = best_device_ideal.index;
	mem.device_large = best_device_large.index;
}

std::vector<vk::physical_device> const & vk::get_physical_devices() {
	return physical_devices;
}

void vk::surface::setup(physical_device const & pdev) {
	VkResult res;
	uint32_t num;
	VKR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev.handle, handle, &surface_capabilities))
	VKR(vkGetPhysicalDeviceSurfaceFormatsKHR(pdev.handle, handle, &num, NULL))
	surface_formats.resize(num);
	VKR(vkGetPhysicalDeviceSurfaceFormatsKHR(pdev.handle, handle, &num, surface_formats.data()))
	VKR(vkGetPhysicalDeviceSurfacePresentModesKHR(pdev.handle, handle, &num, NULL))
	surface_present_modes.resize(num);
	VKR(vkGetPhysicalDeviceSurfacePresentModesKHR(pdev.handle, handle, &num, surface_present_modes.data()))
}
