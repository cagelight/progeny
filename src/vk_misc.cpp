#include "vk.hpp"

vk::shader::shader(device const & parent, uint8_t const * spv, size_t spv_len) : parent(parent) {
	VkShaderModuleCreateInfo module_create = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = spv_len,
		.pCode = reinterpret_cast<uint32_t const *>(spv),
	};
	VkResult res;
	VKR(parent.vkCreateShaderModule(parent.handle, &module_create, NULL, &handle))
}

vk::shader::~shader() {
	parent.vkDestroyShaderModule(parent.handle, handle, nullptr);
}
