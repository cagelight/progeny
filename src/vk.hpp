#pragma once

#include "com.hpp"
#include "context.hpp"

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
	
	std::vector<vk::physical_device> const & get_physical_devices();
	extern VkSurfaceKHR surface;
	
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
		
		struct device_queue {
			capability::flags cap_flags;
			uint32_t queue_family;
			VkQueue queue;
		};
		
		typedef std::vector<device_queue> device_queue_set;
		
		struct logical_device_initializer {
			physical_device const & parent;
			std::vector<VkDeviceQueueCreateInfo> create_infos;
			std::vector<std::vector<float>> queue_priorities;
			device_queue_set queues;
			capability::flags overall_capability;
			std::vector<char const *> device_extensions;
			std::vector<char const *> device_layers {
			#ifdef PROGENY_DEBUG
				"VK_LAYER_LUNARG_standard_validation",
			#endif
			};
			
			logical_device_initializer() = delete;
			logical_device_initializer(physical_device const &, capability_set const &);
		};
		
		VkDevice handle;
		physical_device const & parent;
		device_queue_set queues;
		capability::flags overall_capability;
		std::vector<char const *> device_extensions;
		std::vector<char const *> device_layers;
		#define VK_FN_DDECL
		#include "vk_functions.inl"
		
		device() = delete;
		device(logical_device_initializer &);
		device(device const &) = delete;
		device & operator = (device const &) = delete;
		device(device &&) = default;
		
		~device();
	};
}
