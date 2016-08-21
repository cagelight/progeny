
#ifdef VK_FN_IDECL
#undef VK_FN_IDECL

#define VK_TOP_PROC( func ) PFN_vk##func vk##func;
#define VK_GLOBAL_PROC( func ) PFN_vk##func vk##func;
#define VK_INSTANCE_PROC( func ) PFN_vk##func vk##func;
#define VK_DEVICE_PROC( func )
#define VK_SWAPCHAIN_PROC( func )

#endif

#ifdef VK_FN_DDECL
#undef VK_FN_DDECL

#define VK_TOP_PROC( func )
#define VK_GLOBAL_PROC( func )
#define VK_INSTANCE_PROC( func )
#define VK_DEVICE_PROC( func ) PFN_vk##func vk##func;
#define VK_SWAPCHAIN_PROC( func ) PFN_vk##func vk##func;

#endif

#ifdef VK_FN_SYM_GLOBAL
#undef VK_FN_SYM_GLOBAL

#define VK_TOP_PROC( func ) vk##func = reinterpret_cast<PFN_vk##func>(dlsym(vk_handle, "vk"#func)); if (!vk##func) srcthrow("could not find vk"#func" in loaded vulkan library");
#define VK_GLOBAL_PROC( func ) vk##func = (PFN_vk##func)vkGetInstanceProcAddr(NULL, "vk"#func); if (!vk##func) srcthrow("could not acquire required instance level function vk"#func" from vkGetInstanceProcAddr");
#define VK_INSTANCE_PROC( func )
#define VK_DEVICE_PROC( func )
#define VK_SWAPCHAIN_PROC( func )

#endif

#ifdef VK_FN_SYM_INSTANCE
#undef VK_FN_SYM_INSTANCE

#define VK_TOP_PROC( func )
#define VK_GLOBAL_PROC( func )
#define VK_INSTANCE_PROC( func ) vk##func = (PFN_vk##func)vkGetInstanceProcAddr(vk_instance, "vk"#func); if (!vk##func) srcthrow("could not acquire required instance level function vk"#func" from vkGetInstanceProcAddr");
#define VK_DEVICE_PROC( func )
#define VK_SWAPCHAIN_PROC( func )

#endif

#ifdef VK_FN_SYM_DEVICE
#undef VK_FN_SYM_DEVICE

#define VK_TOP_PROC( func )
#define VK_GLOBAL_PROC( func )
#define VK_INSTANCE_PROC( func )
#define VK_DEVICE_PROC( func ) dev.vk##func = (PFN_vk##func)vkGetDeviceProcAddr(dev.handle, "vk"#func); if (!dev.vk##func) srcthrow("could not acquire required instance level function vk"#func" from vkGetInstanceProcAddr");
#define VK_SWAPCHAIN_PROC( func )

#endif

#ifdef VK_FN_SYM_SWAPCHAIN
#undef VK_FN_SYM_SWAPCHAIN

#define VK_TOP_PROC( func )
#define VK_GLOBAL_PROC( func )
#define VK_INSTANCE_PROC( func )
#define VK_DEVICE_PROC( func ) 
#define VK_SWAPCHAIN_PROC( func ) dev->vk##func = (PFN_vk##func)vkGetDeviceProcAddr(dev->vk_device, "vk"#func); if (!dev->vk##func) { com_printf_error("could not find required device level symbol \"%s\" in loaded vulkan library", "vk"#func); goto finalize; }

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
VK_INSTANCE_PROC( EnumerateDeviceLayerProperties )
VK_INSTANCE_PROC( GetPhysicalDeviceMemoryProperties )

//Surface Extension
VK_INSTANCE_PROC( DestroySurfaceKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfaceSupportKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfaceCapabilitiesKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfaceFormatsKHR )
VK_INSTANCE_PROC( GetPhysicalDeviceSurfacePresentModesKHR )

//XCB Extension
VK_INSTANCE_PROC( CreateXcbSurfaceKHR )

//Debug Extension
#ifdef PROGENY_VK_DEBUG
VK_INSTANCE_PROC( CreateDebugReportCallbackEXT )
VK_INSTANCE_PROC( DebugReportMessageEXT )
VK_INSTANCE_PROC( DestroyDebugReportCallbackEXT )
#endif

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
VK_DEVICE_PROC( FlushMappedMemoryRanges )
VK_DEVICE_PROC( CmdSetViewport )
VK_DEVICE_PROC( CmdSetScissor )
VK_DEVICE_PROC( QueueWaitIdle )

//Swapchain Extension
VK_SWAPCHAIN_PROC( CreateSwapchainKHR )
VK_SWAPCHAIN_PROC( DestroySwapchainKHR )
VK_SWAPCHAIN_PROC( GetSwapchainImagesKHR )
VK_SWAPCHAIN_PROC( AcquireNextImageKHR )
VK_SWAPCHAIN_PROC( QueuePresentKHR )

//================================================================
//----------------------------------------------------------------
//================================================================

#undef VK_TOP_PROC
#undef VK_GLOBAL_PROC
#undef VK_INSTANCE_PROC
#undef VK_DEVICE_PROC
#undef VK_SWAPCHAIN_PROC
