#include "vk.hpp"
#include "control.hpp"
#include "pmath.hpp"

VkFormat vk::swapchain::format = VK_FORMAT_UNDEFINED;

static vk::device * swp_device = nullptr;
static VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
static VkSwapchainCreateInfoKHR swp_create;
static VkExtent2D & swp_extent {swp_create.imageExtent};
static VkFence swp_fence = VK_NULL_HANDLE;
static VkSemaphore swp_semaphore = VK_NULL_HANDLE;
static vk::device::queue * swp_queue;

static constexpr VkImageSubresourceRange swp_img_range = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = 1,
};

vk::device const * vk::swapchain::get_device() noexcept {
	return swp_device;
}

struct swapchain_image_set {
	VkImage image;
	VkImageView image_view;
	
	swapchain_image_set(VkImage img) : image(img) {
		VkResult res;
		VkImageViewCreateInfo imageview_create = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = img,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = vk::swapchain::format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		VKR(swp_device->vkCreateImageView(swp_device->handle, &imageview_create, nullptr, &image_view))
	}
	~swapchain_image_set() {
		swp_device->vkDestroyImageView(swp_device->handle, image_view, nullptr);
	}
};

static std::vector<swapchain_image_set> image_sets;

void vk::swapchain::init(device &dev) {
	
	VkResult res;
	
	try {
		
		if (!(dev.overall_capability & device::capability::presentable)) srcthrow("attempted to initialize swapchain with a non-presentable device");
		swp_device = &dev;
		
		bool found_swp_qfam = false;
		for (device::queue & queue : dev.queues) {
			if (queue.cap_flags & device::capability::presentable) {
				if (found_swp_qfam) {
					com::print("WARNING: device bound to swapchain has multiple queues marked with presentable capability; the first one will be used by the swapchain.");
				} else {
					swp_queue = &queue;
					found_swp_qfam = true;
				}
			}
		}
		
		vk::surface::setup(dev.parent);
		if (!(vk::surface::capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) srcthrow("surface doesn't allow images to be transferred onto it (?)");
		
		VkSurfaceFormatKHR selected_format = surface::formats[0];
		if (selected_format.format == VK_FORMAT_UNDEFINED) {
			selected_format.format = VK_FORMAT_R8G8B8A8_UNORM;
			selected_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		} else if (surface::formats.size() > 1) {
			for (VkSurfaceFormatKHR surf : surface::formats) {
				if (surf.format == VK_FORMAT_R8G8B8A8_UNORM) {
					selected_format = surf;
					break;
				}
			}
		}
		
		VkPresentModeKHR selected_mode = VK_PRESENT_MODE_FIFO_KHR;
		uint32_t image_count = promath::clamp<uint32_t>(3, surface::capabilities.minImageCount, surface::capabilities.maxImageCount);
		
		swp_create = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = vk::surface::handle,
			.minImageCount = image_count,
			.imageFormat = selected_format.format,
			.imageColorSpace = selected_format.colorSpace,
			.imageExtent = vk::surface::capabilities.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = selected_mode,
			.clipped = VK_TRUE,
			.oldSwapchain = nullptr,
		};
		
		if (swp_extent.height == UINT32_MAX || swp_extent.width == UINT32_MAX) swp_extent = control::extent();

		swapchain::format = selected_format.format;
		
		VKR(swp_device->vkCreateSwapchainKHR(swp_device->handle, &swp_create, nullptr, &vk_swapchain))

		uint32_t swapchain_image_count;
		VKR(swp_device->vkGetSwapchainImagesKHR(swp_device->handle, vk_swapchain, &swapchain_image_count, nullptr))
		std::vector<VkImage> swapchain_image_temp {swapchain_image_count};
		VKR(swp_device->vkGetSwapchainImagesKHR(swp_device->handle, vk_swapchain, &swapchain_image_count, swapchain_image_temp.data()))
		image_sets.reserve(swapchain_image_count);
		
		VkCommandPool tmp_pool = VK_NULL_HANDLE;
		VkFence tmp_fence = VK_NULL_HANDLE;
		VkCommandBuffer tmp_buffer;
		
		try {
		
			VkCommandPoolCreateInfo cmd_pool_create = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = swp_device->queues[0].queue_family,
			};
			VKR(swp_device->vkCreateCommandPool(swp_device->handle, &cmd_pool_create, nullptr, &tmp_pool))
			VkCommandBufferAllocateInfo cmd_allocate = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = tmp_pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1,
			};
			VKR(swp_device->vkAllocateCommandBuffers(swp_device->handle, &cmd_allocate, &tmp_buffer))
			
			VkCommandBufferBeginInfo begin = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				.pInheritanceInfo = nullptr,
			};
			VKR(swp_device->vkBeginCommandBuffer(tmp_buffer, &begin))
			
			for (VkImage img : swapchain_image_temp) {
				VkImageMemoryBarrier swp_image_barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
					.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.srcQueueFamilyIndex = swp_queue->queue_family,
					.dstQueueFamilyIndex = swp_queue->queue_family,
					.image = img,
					.subresourceRange = swp_img_range,
				};
				swp_device->vkCmdPipelineBarrier(tmp_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swp_image_barrier);
				
				image_sets.emplace_back(img);
			}
			
			VKR(swp_device->vkEndCommandBuffer(tmp_buffer))
			
			VkSubmitInfo img_init_submit = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.pNext = nullptr,
				.waitSemaphoreCount = 0,
				.pWaitSemaphores = nullptr,
				.pWaitDstStageMask = nullptr,
				.commandBufferCount = 1,
				.pCommandBuffers = &tmp_buffer,
				.signalSemaphoreCount = 0,
				.pSignalSemaphores = nullptr,
			};
			
			
			VkFenceCreateInfo tmp_fence_create = {
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
			};
			VKR(swp_device->vkCreateFence(swp_device->handle, &tmp_fence_create, nullptr, &tmp_fence))
			VKR(swp_device->vkQueueSubmit(swp_queue->handle, 1, &img_init_submit, tmp_fence))
			VKR(swp_device->vkWaitForFences(swp_device->handle, 1, &tmp_fence, VK_TRUE, UINT64_MAX))
			swp_device->vkDestroyFence(swp_device->handle, tmp_fence, nullptr);
			swp_device->vkDestroyCommandPool(swp_device->handle, tmp_pool, nullptr);
		
		} catch (com::exception & e) {
			if (tmp_pool) swp_device->vkDestroyCommandPool(swp_device->handle, tmp_pool, nullptr);
			if (tmp_fence) swp_device->vkDestroyFence(swp_device->handle, tmp_fence, nullptr);
			throw e;
		}
		
		VkFenceCreateInfo fence_create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
		};
		
		VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
		};
		
		VKR(swp_device->vkCreateFence(swp_device->handle, &fence_create_info, nullptr, &swp_fence))
		VKR(swp_device->vkCreateSemaphore(swp_device->handle, &semaphore_create_info, nullptr, &swp_semaphore))
		
	} catch (com::exception & e) {
		swapchain::term(); //don't leave in inconsistent state if error
		throw e; //rethrow after cleanup
	}
}

static void vk_swapchain_reinit() {
	
	swp_create.oldSwapchain = vk_swapchain;
	VkResult VKR(swp_device->vkCreateSwapchainKHR(swp_device->handle, &swp_create, nullptr, &vk_swapchain));
	if (swp_create.oldSwapchain != VK_NULL_HANDLE) swp_device->vkDestroySwapchainKHR(swp_device->handle, swp_create.oldSwapchain, nullptr);
	
	uint32_t swapchain_image_count;
	VKR(swp_device->vkGetSwapchainImagesKHR(swp_device->handle, vk_swapchain, &swapchain_image_count, nullptr))
	std::vector<VkImage> swapchain_image_temp {swapchain_image_count};
	VKR(swp_device->vkGetSwapchainImagesKHR(swp_device->handle, vk_swapchain, &swapchain_image_count, swapchain_image_temp.data()))
	image_sets.clear();
	image_sets.reserve(swapchain_image_count);
	
	VkCommandPool tmp_pool = VK_NULL_HANDLE;
	VkFence tmp_fence = VK_NULL_HANDLE;
	VkCommandBuffer tmp_buffer;
	
	try {
	
		VkCommandPoolCreateInfo cmd_pool_create = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = swp_device->queues[0].queue_family,
		};
		VKR(swp_device->vkCreateCommandPool(swp_device->handle, &cmd_pool_create, nullptr, &tmp_pool))
		VkCommandBufferAllocateInfo cmd_allocate = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = tmp_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VKR(swp_device->vkAllocateCommandBuffers(swp_device->handle, &cmd_allocate, &tmp_buffer))
		
		VkCommandBufferBeginInfo begin = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};
		VKR(swp_device->vkBeginCommandBuffer(tmp_buffer, &begin))
		
		for (VkImage img : swapchain_image_temp) {
			VkImageMemoryBarrier swp_image_barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = swp_queue->queue_family,
				.dstQueueFamilyIndex = swp_queue->queue_family,
				.image = img,
				.subresourceRange = swp_img_range,
			};
			swp_device->vkCmdPipelineBarrier(tmp_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swp_image_barrier);
			
			image_sets.emplace_back(img);
		}
		
		VKR(swp_device->vkEndCommandBuffer(tmp_buffer))
		
		VkSubmitInfo img_init_submit = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &tmp_buffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr,
		};
		
		
		VkFenceCreateInfo tmp_fence_create = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
		};
		VKR(swp_device->vkCreateFence(swp_device->handle, &tmp_fence_create, nullptr, &tmp_fence))
		VKR(swp_device->vkQueueSubmit(swp_queue->handle, 1, &img_init_submit, tmp_fence))
		VKR(swp_device->vkWaitForFences(swp_device->handle, 1, &tmp_fence, VK_TRUE, UINT64_MAX))
		swp_device->vkDestroyFence(swp_device->handle, tmp_fence, nullptr);
		swp_device->vkDestroyCommandPool(swp_device->handle, tmp_pool, nullptr);
	
	} catch (com::exception & e) {
		if (tmp_pool) swp_device->vkDestroyCommandPool(swp_device->handle, tmp_pool, nullptr);
		if (tmp_fence) swp_device->vkDestroyFence(swp_device->handle, tmp_fence, nullptr);
		throw e;
	}
}

void vk::swapchain::check_reinit() {
	vk::surface::setup(swp_device->parent);
	VkExtent2D test_extent = vk::surface::capabilities.currentExtent;
	if (test_extent.height == UINT32_MAX || test_extent.width == UINT32_MAX) test_extent = control::extent();
	if (swp_extent.width != test_extent.width || swp_extent.height != test_extent.height) {
		swp_extent = vk::surface::capabilities.currentExtent;
		vk_swapchain_reinit();
	}
}

void vk::swapchain::term() noexcept {
	if (swp_fence != VK_NULL_HANDLE) {
		swp_device->vkDestroyFence(swp_device->handle, swp_fence, nullptr);
		swp_fence = VK_NULL_HANDLE;
	}
	if (swp_semaphore != VK_NULL_HANDLE) {
		swp_device->vkDestroySemaphore(swp_device->handle, swp_semaphore, nullptr);
		swp_semaphore = VK_NULL_HANDLE;
	}
	image_sets.clear();
	if (vk_swapchain != VK_NULL_HANDLE) {
		swp_device->vkDestroySwapchainKHR(swp_device->handle, vk_swapchain, nullptr);
		vk_swapchain = VK_NULL_HANDLE;
	}
	swp_device = nullptr;
	vk::swapchain::format = VK_FORMAT_UNDEFINED;
}

static uint32_t current_image_index = 0;

vk::swapchain::frame_set vk::swapchain::begin_frame(VkCommandBuffer cmd, VkAccessFlags dstAccess, VkImageLayout dstLayout, uint32_t dstQueueFamily) {
	swp_device->vkResetFences(swp_device->handle, 1, &swp_fence);
	swp_device->vkAcquireNextImageKHR(swp_device->handle, vk_swapchain, UINT64_MAX, nullptr, swp_fence, &current_image_index);
	swp_device->vkWaitForFences(swp_device->handle, 1, &swp_fence, VK_TRUE, UINT64_MAX);

	VkImageMemoryBarrier swp_image_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.dstAccessMask = dstAccess,
		.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.newLayout = dstLayout,
		.srcQueueFamilyIndex = swp_queue->queue_family,
		.dstQueueFamilyIndex = dstQueueFamily,
		.image = image_sets[current_image_index].image,
		.subresourceRange = swp_img_range,
	};
	swp_device->vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swp_image_barrier);
	
	return {image_sets[current_image_index].image, image_sets[current_image_index].image_view, swp_extent};
}

void vk::swapchain::end_frame(VkCommandBuffer cmd, VkAccessFlags srcAccess, VkImageLayout srcLayout, uint32_t srcQueueFamily) {
	VkResult res;
	VkImageMemoryBarrier swp_image_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = srcAccess,
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.oldLayout = srcLayout,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = srcQueueFamily,
		.dstQueueFamilyIndex = swp_queue->queue_family,
		.image = image_sets[current_image_index].image,
		.subresourceRange = swp_img_range,
	};
	swp_device->vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swp_image_barrier);
	VKR(swp_device->vkEndCommandBuffer(cmd))
	VkSubmitInfo cmd_submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
 		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &swp_semaphore,
	};
	swp_device->vkQueueSubmit(swp_queue->handle, 1, &cmd_submit, VK_NULL_HANDLE);
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &swp_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &vk_swapchain,
		.pImageIndices = &current_image_index,
		.pResults = nullptr,
	};
	swp_device->vkQueuePresentKHR(swp_queue->handle, &present_info);
	swp_device->vkResetFences(swp_device->handle, 1, &swp_fence);
	swp_device->vkQueueSubmit(swp_queue->handle, 0, nullptr, swp_fence);
	swp_device->vkWaitForFences(swp_device->handle, 1, &swp_fence, VK_TRUE, UINT64_MAX);
}
