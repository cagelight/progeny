#include "vk.hpp"
#include "control.hpp"
#include "pmath.hpp"

VkFormat vk::swapchain::format = VK_FORMAT_UNDEFINED;

static vk::device * swp_device = nullptr;
static VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
static VkFence swp_fence = VK_NULL_HANDLE;
static VkExtent2D swp_extent;
static vk::device::queue * swp_queue;

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
		uint32_t image_count = pmath::clamp<uint32_t>(3, surface::capabilities.minImageCount, surface::capabilities.maxImageCount);
		swp_extent = control::extent();
		VkSwapchainKHR old_swapchain = vk_swapchain;
		
		VkSwapchainCreateInfoKHR swap_chain_create_info = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = vk::surface::handle,
			.minImageCount = image_count,
			.imageFormat = selected_format.format,
			.imageColorSpace = selected_format.colorSpace,
			.imageExtent = swp_extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = selected_mode,
			.clipped = VK_TRUE,
			.oldSwapchain = old_swapchain,
		};

		swapchain::format = selected_format.format;
		
		VKR(swp_device->vkCreateSwapchainKHR(swp_device->handle, &swap_chain_create_info, nullptr, &vk_swapchain))
		if (old_swapchain != VK_NULL_HANDLE) swp_device->vkDestroySwapchainKHR(swp_device->handle, old_swapchain, nullptr);

		uint32_t swapchain_image_count;
		VKR(swp_device->vkGetSwapchainImagesKHR(swp_device->handle, vk_swapchain, &swapchain_image_count, nullptr))
		assert(swapchain_image_count == image_count);
		std::vector<VkImage> swapchain_image_temp {swapchain_image_count};
		VKR(swp_device->vkGetSwapchainImagesKHR(swp_device->handle, vk_swapchain, &swapchain_image_count, swapchain_image_temp.data()))
		image_sets.reserve(swapchain_image_count);
		for (VkImage img : swapchain_image_temp) {
			image_sets.emplace_back(img);
		}
		
		VkFenceCreateInfo fence_create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
		};
		
		VKR(swp_device->vkCreateFence(swp_device->handle, &fence_create_info, nullptr, &swp_fence))
		
	} catch (com::exception & e) {
		swapchain::term(); //don't leave in inconsistent state if error
		throw e; //rethrow after cleanup
	}
}

void vk::swapchain::reinit() {
	
}

void vk::swapchain::term() noexcept {
	if (swp_fence != VK_NULL_HANDLE) {
		swp_device->vkDestroyFence(swp_device->handle, swp_fence, nullptr);
		swp_fence = VK_NULL_HANDLE;
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
static VkFramebuffer current_fb = VK_NULL_HANDLE;

VkFramebuffer vk::swapchain::begin_frame(VkRenderPass rpass) {
	swp_device->vkResetFences(swp_device->handle, 1, &swp_fence);
	swp_device->vkAcquireNextImageKHR(swp_device->handle, vk_swapchain, UINT64_MAX, NULL, swp_fence, &current_image_index);
	VkFramebufferCreateInfo framebuffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.renderPass = rpass,
		.attachmentCount = 1,
		.pAttachments = &image_sets[current_image_index].image_view,
		.width = swp_extent.width,
		.height = swp_extent.height,
		.layers = 1,
	};
	swp_device->vkCreateFramebuffer(swp_device->handle, &framebuffer_create_info, NULL, &current_fb);
	swp_device->vkWaitForFences(swp_device->handle, 1, &swp_fence, VK_TRUE, UINT64_MAX);
	return current_fb;
}

void vk::swapchain::end_frame(VkSemaphore wait_sem) {
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = static_cast<uint32_t>(wait_sem == VK_NULL_HANDLE ? 0 : 1),
		.pWaitSemaphores = wait_sem ? &wait_sem : NULL,
		.swapchainCount = 1,
		.pSwapchains = &vk_swapchain,
		.pImageIndices = &current_image_index,
		.pResults = NULL,
	};
	swp_device->vkQueuePresentKHR(swp_queue->handle, &present_info);
	swp_device->vkResetFences(swp_device->handle, 1, &swp_fence);
	swp_device->vkQueueSubmit(swp_queue->handle, 0, NULL, swp_fence);
	swp_device->vkWaitForFences(swp_device->handle, 1, &swp_fence, VK_TRUE, UINT64_MAX);
	swp_device->vkDestroyFramebuffer(swp_device->handle, current_fb, NULL);
}
