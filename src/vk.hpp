#pragma once

#include "com.hpp"

#pragma once
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

typedef VkXcbSurfaceCreateInfoKHR NativeSurfaceCreateInfo;

#ifdef PROGENY_DEBUG
#define PROGENY_VK_DEBUG
#endif

namespace vk {
	
	struct device;
	
	struct physical_device {
		VkPhysicalDevice handle;
		VkPhysicalDeviceProperties properties;
		std::vector<VkExtensionProperties> extensions;
		std::vector<VkLayerProperties> layers;
		std::vector<VkQueueFamilyProperties> queue_families;
		std::vector<VkBool32> queue_families_presentable;
		VkPhysicalDeviceMemoryProperties memory_properties;
		uint32_t memory_index_staging, memory_index_device;
		
		physical_device() = delete;
		physical_device(VkPhysicalDevice &);
		physical_device(physical_device const &) = delete;
		physical_device & operator = (physical_device const &) = delete;
		physical_device(physical_device &&) = default;
	};
	
	namespace instance {
		void init();
		void term() noexcept;
		VkDevice create_device(physical_device const &, VkDeviceCreateInfo *);
		void resolve_device_symbols(device &);
	}
	
	namespace surface {
		extern VkSurfaceKHR handle;
		extern VkSurfaceCapabilitiesKHR const & capabilities;
		extern std::vector<VkSurfaceFormatKHR> const & formats;
		extern std::vector<VkPresentModeKHR> const & present_modes;
		void setup(physical_device const &);
		NativeSurfaceCreateInfo create_info();
	}
	
	std::vector<vk::physical_device> const & get_physical_devices();
	
	namespace swapchain {
		extern VkFormat format;
		
		device const * get_device() noexcept;
		void init(device &);
		void reinit(); // window size change, swap method change, etc.
		void term() noexcept;
		
		VkFramebuffer begin_frame(VkRenderPass);
		void end_frame(VkSemaphore);
	}
	
	struct device {
		
		struct capability { //ordered least important to most important, for sorting
			static constexpr uint32_t transfer = 1 << 0;
			static constexpr uint32_t compute = 1 << 1;
			static constexpr uint32_t graphics = 1 << 2;
			static constexpr uint32_t presentable = 1 << 3;
			typedef uint32_t flags;
		};
		
		typedef std::vector<capability::flags> capability_set;
		static inline void capability_sort(capability_set & set) {
			std::sort(set.begin(), set.end(), [](capability::flags a, capability::flags b){return a>b;});
		}
		
		struct queue {
			capability::flags cap_flags;
			uint32_t queue_family;
			VkQueue handle;
		};
		
		typedef std::vector<queue> queue_set;
		
		struct initializer {
			physical_device const & parent;
			std::vector<VkDeviceQueueCreateInfo> create_infos;
			std::vector<std::vector<float>> queue_priorities;
			queue_set queues;
			std::vector<uint32_t> queue_indicies;
			capability::flags overall_capability = 0;
			std::vector<char const *> device_extensions;
			std::vector<char const *> device_layers {
			#ifdef PROGENY_VK_DEBUG
				"VK_LAYER_LUNARG_standard_validation",
			#endif
			};
			
			initializer() = delete;
			initializer(physical_device const &, capability_set const &);
		};
		
		VkDevice handle;
		physical_device const & parent;
		queue_set queues;
		capability::flags overall_capability;
		std::vector<char const *> device_extensions;
		std::vector<char const *> device_layers;
		#define VK_FN_DDECL
		#include "vk_functions.inl"
		
		device() = default;
		device(initializer &);
		device(device const &) = delete;
		device(device &&) = default;
		
		device & operator = (device const &) = delete;
		bool operator == (device const & other) {return this->handle == other.handle;}
		
		~device();
	};
	
	struct shader {
		VkShaderModule handle;
		device const & parent;
		
		shader() = delete;
		shader(device const & parent, uint8_t const * spv, size_t spv_len);
		shader(shader const &) = delete;
		shader(shader &&) = default;
		
		shader & operator = (shader const &) = delete;
		
		~shader();
	};
	
	namespace spirv {
		extern uint8_t const vert_test [];
		extern size_t vert_test_size;
		extern uint8_t const frag_test [];
		extern size_t frag_test_size;
	}
}

#define VKR(call) res = call; if (res != VK_SUCCESS) srcthrow("\"%s\" unsuccessful: (%i)", #call, res);
