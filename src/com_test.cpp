#include "com.hpp"
#include "vk.hpp"

#include "pmath.hpp"

typedef struct vk_test_vertex_s {
	float x, y;
	float r, g, b;
} vk_test_vertex_t;

static vk_test_vertex_t testv [] = {
	{0.5, 1, 1, 0, 0},
	{0.7, 0, 0, 1, 0},
	{0, 0, 0, 0, 1}
};

static vk::device * com_dev = nullptr;

static VkPipelineLayout rpass_layout;
static VkPipeline rpass_pipeline;
static VkRenderPass rpass = VK_NULL_HANDLE;

static VkCommandPool cmd_pool = VK_NULL_HANDLE;
static VkCommandBuffer frame_cmd = VK_NULL_HANDLE;

static VkBuffer testv_buffer = VK_NULL_HANDLE;
static VkDeviceMemory testv_mem = VK_NULL_HANDLE;

static void render_test_init() {
	VkResult res;
	
	VkAttachmentDescription attachment_desc = {
		.flags = 0,
		.format = vk::swapchain::format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
	
	VKR(com_dev->vkCreateRenderPass(com_dev->handle, &renderpass_create, NULL, &rpass));
	
	VkPipelineLayoutCreateInfo layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};
	VKR(com_dev->vkCreatePipelineLayout(com_dev->handle, &layout_create_info, NULL, &rpass_layout))
	
	vk::shader vert_test {*com_dev, vk::spirv::vert_test, vk::spirv::vert_test_size};
	vk::shader frag_test {*com_dev, vk::spirv::frag_test, vk::spirv::frag_test_size};
	
	VkPipelineShaderStageCreateInfo shader_stage_create_infos [] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vert_test.handle,
			.pName = "main",
			.pSpecializationInfo = NULL,
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = frag_test.handle,
			.pName = "main",
			.pSpecializationInfo = NULL,
		},
	};
	
	VkVertexInputBindingDescription vertex_binding_description = {
		.binding = 0,
		.stride = sizeof(vk_test_vertex_t),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	
	VkVertexInputAttributeDescription vertex_attribute_descriptions [2] = {
		{
			.location = 0,
			.binding = vertex_binding_description.binding,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(vk_test_vertex_t, x),
		},
		{
			.location = 1,
			.binding = vertex_binding_description.binding,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(vk_test_vertex_t, r),
		}
	};
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertex_binding_description,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = vertex_attribute_descriptions,
	};
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	
	VkPipelineViewportStateCreateInfo viewport_state_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = NULL,
		.scissorCount = 1,
		.pScissors = NULL,
	};
	
	VkDynamicState dynamic_states [2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	
	VkPipelineDynamicStateCreateInfo  dynamic_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states
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
		.pDynamicState = &dynamic_state_create_info,
		.layout = rpass_layout,
		.renderPass = rpass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};
	VKR(com_dev->vkCreateGraphicsPipelines(com_dev->handle, NULL, 1, &pipeline_create_info, NULL, &rpass_pipeline))
}

static void command_init() {
	VkResult res;
	VkCommandPoolCreateInfo cmd_pool_create = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = com_dev->queues[0].queue_family,
	};
	VKR(com_dev->vkCreateCommandPool(com_dev->handle, &cmd_pool_create, nullptr, &cmd_pool))
	VkCommandBufferAllocateInfo cmd_allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VKR(com_dev->vkAllocateCommandBuffers(com_dev->handle, &cmd_allocate, &frame_cmd))
}

static void test_init() {
	VkResult res;
	
	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = sizeof(testv),
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
	};
	
	VKR(com_dev->vkCreateBuffer(com_dev->handle, &buffer_create_info, NULL, &testv_buffer))
	
	VkMemoryRequirements buffer_memory_requirements;
	com_dev->vkGetBufferMemoryRequirements(com_dev->handle, testv_buffer, &buffer_memory_requirements);
	
	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = buffer_memory_requirements.size,
		.memoryTypeIndex = com_dev->parent.mem.staging,
	};
	VKR(com_dev->vkAllocateMemory(com_dev->handle, &memory_allocate_info, NULL, &testv_mem))
	
	VKR(com_dev->vkBindBufferMemory(com_dev->handle, testv_buffer, testv_mem, 0))
	
	void * vertex_buffer_memory_pointer;
	VKR(com_dev->vkMapMemory(com_dev->handle, testv_mem, 0, sizeof(testv), 0, &vertex_buffer_memory_pointer))

	memcpy(vertex_buffer_memory_pointer, testv, sizeof(testv));
	VkMappedMemoryRange flush_range = {
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		.pNext = NULL,
		.memory = testv_mem,
		.offset = 0,
		.size = VK_WHOLE_SIZE,
	};
	com_dev->vkFlushMappedMemoryRanges(com_dev->handle, 1, &flush_range);
	com_dev->vkUnmapMemory(com_dev->handle, testv_mem);
}

#include "text.hpp"

void com::test::init() {
		
		text::test();
	
		vk::physical_device const & pdev = vk::get_physical_devices()[0];
		
		vk::device::capability_set qset;
		qset.emplace_back(vk::device::capability::graphics | vk::device::capability::presentable);
		qset.emplace_back(vk::device::capability::transfer);
		vk::device::capability_sort(qset);
		
		vk::device::initializer ldi {pdev, qset};
		com_dev = new vk::device {ldi};
		
		command_init();
		test_init();
		
		vk::swapchain::init(*com_dev);
		render_test_init();
}

void com::test::term() noexcept {
	if (testv_buffer) {
		com_dev->vkDestroyBuffer(com_dev->handle, testv_buffer, nullptr);
		testv_buffer = VK_NULL_HANDLE;
	}
	if (testv_mem) {
		com_dev->vkFreeMemory(com_dev->handle, testv_mem, nullptr);
		testv_mem = VK_NULL_HANDLE;
	}
	if (cmd_pool) {
		com_dev->vkDestroyCommandPool(com_dev->handle, cmd_pool, nullptr);
		cmd_pool = VK_NULL_HANDLE;
	}
	if (rpass_layout) {
		com_dev->vkDestroyPipelineLayout(com_dev->handle, rpass_layout, nullptr);
		rpass_layout = VK_NULL_HANDLE;
	}
	if (rpass_pipeline) {
		com_dev->vkDestroyPipeline(com_dev->handle, rpass_pipeline, nullptr);
		rpass_pipeline = VK_NULL_HANDLE;
	}
	if (rpass) {
		com_dev->vkDestroyRenderPass(com_dev->handle, rpass, nullptr);
		rpass = VK_NULL_HANDLE;
	}
	vk::swapchain::term();
	if (com_dev) {
		delete com_dev;
		com_dev = nullptr;
	}
}

static constexpr VkClearValue clear = {
	.color = {{0.0f, 0.25f, 0.0f, 1.0f}},
};

void com::test::frame() {
	
	VkResult res;
	VkFramebuffer fb;
	
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};
	VKR(com_dev->vkBeginCommandBuffer(frame_cmd, &begin))
	
	vk::swapchain::frame_set fs = vk::swapchain::begin_frame(frame_cmd, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, com_dev->queues[0].queue_family);
	
	VkFramebufferCreateInfo framebuffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = rpass,
		.attachmentCount = 1,
		.pAttachments = &fs.img_view,
		.width = fs.extent.width,
		.height = fs.extent.height,
		.layers = 1,
	};
	com_dev->vkCreateFramebuffer(com_dev->handle, &framebuffer_create_info, nullptr, &fb);
	
	VkRenderPassBeginInfo rpass_begin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = rpass,
		.framebuffer = fb,
		.renderArea = {
			{0, 0}, fs.extent
		},
		.clearValueCount = 1,
		.pClearValues = &clear,
	};
	com_dev->vkCmdBeginRenderPass(frame_cmd, &rpass_begin, VK_SUBPASS_CONTENTS_INLINE);
	com_dev->vkCmdBindPipeline(frame_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, rpass_pipeline);
	
	VkViewport viewport = {0.0f, 0.0f, static_cast<float>(fs.extent.width), static_cast<float>(fs.extent.height), 0.0f, 1.0f};
	VkRect2D scissor = {{0, 0}, fs.extent};

	com_dev->vkCmdSetViewport(frame_cmd, 0, 1, &viewport);
	com_dev->vkCmdSetScissor(frame_cmd, 0, 1, &scissor);
	
	VkDeviceSize offset = 0;
    com_dev->vkCmdBindVertexBuffers(frame_cmd, 0, 1, &testv_buffer, &offset);
    com_dev->vkCmdDraw(frame_cmd, 3, 1, 0, 0);
	
	com_dev->vkCmdEndRenderPass(frame_cmd);
	vk::swapchain::end_frame(frame_cmd, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, com_dev->queues[0].queue_family);
	
	com_dev->vkDestroyFramebuffer(com_dev->handle, fb, nullptr);
}
