#include "com.hpp"
#include "vk.hpp"

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


static void render_test_init() {
	VkResult res;
	bool stat = false;
	
	VkAttachmentDescription attachment_desc = {
		.flags = 0,
		.format = vk::swapchain::format,
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

void com::test::init() {
		vk::physical_device const & pdev = vk::get_physical_devices()[0];
		
		vk::device::capability_set qset;
		qset.emplace_back(vk::device::capability::graphics | vk::device::capability::presentable);
		qset.emplace_back(vk::device::capability::transfer);
		vk::device::capability_sort(qset);
		vk::device::initializer ldi {pdev, qset};
		com_dev = new vk::device {ldi};
		
		vk::swapchain::init(*com_dev);
		render_test_init();
}

void com::test::term() noexcept {
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

void com::test::frame() {
	//vk::swapchain::begin_frame();
	//vk::swapchain::end_frame(nullptr);
}
