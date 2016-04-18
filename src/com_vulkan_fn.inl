
#ifdef VK_FN_DECL
#undef VK_FN_DECL

#define VK_TOP_PROC( func ) PFN_vk##func vk_##func;
#define VK_GLOBAL_PROC( func ) PFN_vk##func vk_##func;
#define VK_INSTANCE_PROC( func ) PFN_vk##func vk_##func;
#define VK_DEVICE_PROC( func ) PFN_vk##func vk_##func;

#endif

#ifdef VK_FN_SYM_GLOBAL
#undef VK_FN_SYM_GLOBAL

#define VK_TOP_PROC( func ) vk_##func = (PFN_vk##func)dlsym(vk_handle, "vk"#func); if (!vk_##func) { com_printf_error("could not find required top level symbol \"%s\" in loaded vulkan library", "vk"#func); return false; }
#define VK_GLOBAL_PROC( func ) vk_##func = (PFN_vk##func)vk_GetInstanceProcAddr(NULL, "vk"#func); if (!vk_##func) { com_printf_error("could not find required instance level symbol \"%s\" in loaded vulkan library", "vk"#func); return false; }
#define VK_INSTANCE_PROC( func )
#define VK_DEVICE_PROC( func )

#endif

#ifdef VK_FN_SYM_INSTANCE
#undef VK_FN_SYM_INSTANCE

#define VK_TOP_PROC( func )
#define VK_GLOBAL_PROC( func )
#define VK_INSTANCE_PROC( func ) vk_##func = (PFN_vk##func)vk_GetInstanceProcAddr(vk_instance, "vk"#func); if (!vk_##func) { com_printf_error("could not find required instance level symbol \"%s\" in loaded vulkan library", "vk"#func); return false; }
#define VK_DEVICE_PROC( func )

#endif

#ifdef VK_FN_SYM_DEVICE
#undef VK_FN_SYM_DEVICE

#define VK_TOP_PROC( func )
#define VK_GLOBAL_PROC( func )
#define VK_INSTANCE_PROC( func )
#define VK_DEVICE_PROC( func ) vk_##func = (PFN_vk##func)vk_GetDeviceProcAddr(vk_device, "vk"#func); if (!vk_##func) { com_printf_error("could not find required device level symbol \"%s\" in loaded vulkan library", "vk"#func); return false; }

#endif

//================================================================
//----------------------------------------------------------------
//================================================================

VK_TOP_PROC(GetInstanceProcAddr)

VK_GLOBAL_PROC(CreateInstance)
VK_GLOBAL_PROC(EnumerateInstanceExtensionProperties)
VK_GLOBAL_PROC(EnumerateInstanceLayerProperties)

//================================================================
//----------------------------------------------------------------
//================================================================

VK_INSTANCE_PROC( DestroyInstance )
VK_INSTANCE_PROC( EnumeratePhysicalDevices )
VK_INSTANCE_PROC( GetPhysicalDeviceProperties )
VK_INSTANCE_PROC( GetPhysicalDeviceFeatures )
VK_INSTANCE_PROC( GetPhysicalDeviceQueueFamilyProperties )
VK_INSTANCE_PROC( CreateDevice )
VK_INSTANCE_PROC( GetDeviceProcAddr )
VK_INSTANCE_PROC( EnumerateDeviceExtensionProperties )
VK_INSTANCE_PROC( GetPhysicalDeviceMemoryProperties )

//Surface Extension
VK_INSTANCE_PROC( DestroySurfaceKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfaceSupportKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfaceCapabilitiesKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfaceFormatsKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfacePresentModesKHR )

//XCB Extension
VK_INSTANCE_PROC( CreateXcbSurfaceKHR )

//================================================================
//----------------------------------------------------------------
//================================================================

VK_DEVICE_PROC( GetDeviceQueue )
VK_DEVICE_PROC( DestroyDevice )
VK_DEVICE_PROC( DeviceWaitIdle )
VK_DEVICE_PROC( CreateCommandPool )
VK_DEVICE_PROC( AllocateCommandBuffers )
VK_DEVICE_PROC( BeginCommandBuffer )
VK_DEVICE_PROC( CmdPipelineBarrier )
VK_DEVICE_PROC( CmdClearColorImage )
VK_DEVICE_PROC( EndCommandBuffer )
VK_DEVICE_PROC( CreateSemaphore )
VK_DEVICE_PROC( DestroySemaphore )
VK_DEVICE_PROC( QueueSubmit )
VK_DEVICE_PROC( FreeCommandBuffers )
VK_DEVICE_PROC( DestroyCommandPool )
VK_DEVICE_PROC( CreateRenderPass )
VK_DEVICE_PROC( DestroyRenderPass )
VK_DEVICE_PROC( CreateImageView )
VK_DEVICE_PROC( DestroyImageView )
VK_DEVICE_PROC( CreateFramebuffer )
VK_DEVICE_PROC( DestroyFramebuffer )
VK_DEVICE_PROC( CreateShaderModule )
VK_DEVICE_PROC( DestroyShaderModule )
VK_DEVICE_PROC( CreatePipelineLayout )
VK_DEVICE_PROC( DestroyPipelineLayout )
VK_DEVICE_PROC( CreateGraphicsPipelines )
VK_DEVICE_PROC( DestroyPipeline )
VK_DEVICE_PROC( CmdBeginRenderPass )
VK_DEVICE_PROC( CmdEndRenderPass )
VK_DEVICE_PROC( CmdBindPipeline )
VK_DEVICE_PROC( CmdDraw )
VK_DEVICE_PROC( CreateImage )
VK_DEVICE_PROC( CmdCopyImage )
VK_DEVICE_PROC( DestroyImage )
VK_DEVICE_PROC( GetImageMemoryRequirements )
VK_DEVICE_PROC( AllocateMemory )
VK_DEVICE_PROC( BindImageMemory )
VK_DEVICE_PROC( FreeMemory )
VK_DEVICE_PROC( CreateBuffer )
VK_DEVICE_PROC( DestroyBuffer )
VK_DEVICE_PROC( GetBufferMemoryRequirements )
VK_DEVICE_PROC( BindBufferMemory )
VK_DEVICE_PROC( MapMemory )
VK_DEVICE_PROC( UnmapMemory )
VK_DEVICE_PROC( CmdBindVertexBuffers )
VK_DEVICE_PROC( CreateFence )
VK_DEVICE_PROC( DestroyFence )
VK_DEVICE_PROC( ResetFences )
VK_DEVICE_PROC( WaitForFences )

//Swapchain Extension
VK_DEVICE_PROC( CreateSwapchainKHR )
VK_DEVICE_PROC( DestroySwapchainKHR )
VK_DEVICE_PROC( GetSwapchainImagesKHR )
VK_DEVICE_PROC( AcquireNextImageKHR )
VK_DEVICE_PROC( QueuePresentKHR )

//================================================================
//----------------------------------------------------------------
//================================================================

#undef VK_TOP_PROC
#undef VK_GLOBAL_PROC
#undef VK_INSTANCE_PROC
#undef VK_DEVICE_PROC
