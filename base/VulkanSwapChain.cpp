/*
* Class wrapping access to the swap chain
* 
* A swap chain is a collection of framebuffers used for rendering and presentation to the windowing system
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanSwapChain.h"

/** @brief Creates the platform specific surface abstraction of the native platform window used for presentation */
/** @brief 创建用于呈现的本地平台窗口的平台特定表面抽象 */
#if defined(VK_USE_PLATFORM_WIN32_KHR)
/**
 * @brief 初始化 Windows 平台表面
 * @param platformHandle 平台句柄（HINSTANCE）
 * @param platformWindow 平台窗口（HWND）
 */
void VulkanSwapChain::initSurface(void* platformHandle, void* platformWindow)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
void VulkanSwapChain::initSurface(ANativeWindow* window)
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
void VulkanSwapChain::initSurface(IDirectFB* dfb, IDirectFBSurface* window)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
void VulkanSwapChain::initSurface(wl_display *display, wl_surface *window)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
void VulkanSwapChain::initSurface(xcb_connection_t* connection, xcb_window_t window)
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
void VulkanSwapChain::initSurface(void* view)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
void VulkanSwapChain::initSurface(CAMetalLayer* metalLayer)
#elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
void VulkanSwapChain::initSurface(uint32_t width, uint32_t height)
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
void VulkanSwapChain::initSurface(screen_context_t screen_context, screen_window_t screen_window)
#endif
{
	VkResult err = VK_SUCCESS;

	// Create the os-specific surface
	// 创建特定于操作系统的表面
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	// Windows 平台：创建 Win32 表面
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;  // 结构体类型
	surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;  // 窗口实例句柄
	surfaceCreateInfo.hwnd = (HWND)platformWindow;  // 窗口句柄
	err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);  // 创建 Win32 表面
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	// Android 平台：创建 Android 表面
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;  // 结构体类型
	surfaceCreateInfo.window = window;  // Android 窗口指针
	err = vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);  // 创建 Android 表面
#elif defined(VK_USE_PLATFORM_IOS_MVK)
	// iOS 平台：创建 iOS 表面（使用 MoltenVK）
	VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;  // 结构体类型
	surfaceCreateInfo.pNext = NULL;  // 扩展链指针
	surfaceCreateInfo.flags = 0;  // 创建标志
	surfaceCreateInfo.pView = view;  // iOS 视图指针
	err = vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface);  // 创建 iOS 表面
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	// macOS 平台：创建 macOS 表面（使用 MoltenVK）
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;  // 结构体类型
	surfaceCreateInfo.pNext = NULL;  // 扩展链指针
	surfaceCreateInfo.flags = 0;  // 创建标志
	surfaceCreateInfo.pView = view;  // macOS 视图指针
	err = vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface);  // 创建 macOS 表面
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	// Metal 平台：创建 Metal 表面
	VkMetalSurfaceCreateInfoEXT surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;  // 结构体类型
	surfaceCreateInfo.pNext = NULL;  // 扩展链指针
	surfaceCreateInfo.flags = 0;  // 创建标志
	surfaceCreateInfo.pLayer = metalLayer;  // Metal 层指针
	err = vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, NULL, &surface);  // 创建 Metal 表面
#elif defined(_DIRECT2DISPLAY)
	// Direct2Display 平台：创建直接到显示器的表面（绕过窗口系统）
	createDirect2DisplaySurface(width, height);  // 调用直接显示表面创建函数
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	// DirectFB 平台：创建 DirectFB 表面
	VkDirectFBSurfaceCreateInfoEXT surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;  // 结构体类型
	surfaceCreateInfo.dfb = dfb;  // DirectFB 主接口指针
	surfaceCreateInfo.surface = window;  // DirectFB 表面指针
	err = vkCreateDirectFBSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);  // 创建 DirectFB 表面
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	// Wayland 平台：创建 Wayland 表面
	VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;  // 结构体类型
	surfaceCreateInfo.display = display;  // Wayland 显示对象
	surfaceCreateInfo.surface = window;  // Wayland 表面对象
	err = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);  // 创建 Wayland 表面
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	// XCB 平台（Linux/X11）：创建 XCB 表面
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;  // 结构体类型
	surfaceCreateInfo.connection = connection;  // XCB 连接
	surfaceCreateInfo.window = window;  // XCB 窗口 ID
	err = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);  // 创建 XCB 表面
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
	// 无头平台：创建无头表面（用于无窗口渲染）
	VkHeadlessSurfaceCreateInfoEXT surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;  // 结构体类型
	PFN_vkCreateHeadlessSurfaceEXT fpCreateHeadlessSurfaceEXT = (PFN_vkCreateHeadlessSurfaceEXT)vkGetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT");  // 获取函数指针
	if (!fpCreateHeadlessSurfaceEXT){  // 如果获取失败
		vks::tools::exitFatal("Could not fetch function pointer for the headless extension!", -1);  // 输出致命错误
	}
	err = fpCreateHeadlessSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);  // 创建无头表面
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	// QNX Screen 平台：创建 QNX Screen 表面
	VkScreenSurfaceCreateInfoQNX surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;  // 结构体类型
	surfaceCreateInfo.pNext = NULL;  // 扩展链指针
	surfaceCreateInfo.flags = 0;  // 创建标志
	surfaceCreateInfo.context = screen_context;  // QNX Screen 上下文
	surfaceCreateInfo.window = screen_window;  // QNX Screen 窗口
	err = vkCreateScreenSurfaceQNX(instance, &surfaceCreateInfo, NULL, &surface);  // 创建 QNX Screen 表面
#endif

	if (err != VK_SUCCESS) {
		vks::tools::exitFatal("Could not create surface!", err);
	}

	// Get available queue family properties
	// 获取可用的队列族属性
	uint32_t queueCount;  // 队列族数量
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);  // 第一次调用：仅获取数量
	assert(queueCount >= 1);  // 确保至少有一个队列族
	std::vector<VkQueueFamilyProperties> queueProps(queueCount);  // 队列族属性向量
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());  // 第二次调用：获取所有队列族属性

	// Iterate over each queue to learn whether it supports presenting:
	// Find a queue with present support
	// Will be used to present the swap chain images to the windowing system
	// 遍历每个队列以了解它是否支持呈现：
	// 查找支持呈现的队列
	// 将用于将交换链图像呈现到窗口系统
	std::vector<VkBool32> supportsPresent(queueCount);  // 每个队列族是否支持呈现
	for (uint32_t i = 0; i < queueCount; i++) 
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);  // 查询队列族是否支持表面呈现
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	// 在队列族数组中搜索图形队列和呈现队列，尝试找到一个同时支持两者的队列
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;  // 图形队列族索引（初始化为无效值）
	uint32_t presentQueueNodeIndex = UINT32_MAX;  // 呈现队列族索引（初始化为无效值）
	for (uint32_t i = 0; i < queueCount; i++) {  // 遍历所有队列族
		if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)  {  // 如果队列支持图形操作
			if (graphicsQueueNodeIndex == UINT32_MAX) {  // 如果尚未找到图形队列
				graphicsQueueNodeIndex = i;  // 保存第一个图形队列索引
			}
			if (supportsPresent[i] == VK_TRUE) {  // 如果该队列同时支持呈现
				graphicsQueueNodeIndex = i;  // 更新图形队列索引
				presentQueueNodeIndex = i;  // 设置呈现队列索引
				break;  // 找到同时支持两者的队列，退出循环
			}
		}
	}
	if (presentQueueNodeIndex == UINT32_MAX) 
	{	
		// If there's no queue that supports both present and graphics
		// try to find a separate present queue
		// 如果没有队列同时支持呈现和图形
		// 尝试查找单独的呈现队列
		for (uint32_t i = 0; i < queueCount; ++i) {  // 遍历所有队列族
			if (supportsPresent[i] == VK_TRUE) {  // 如果队列支持呈现
				presentQueueNodeIndex = i;  // 设置呈现队列索引
				break;  // 找到呈现队列，退出循环
			}
		}
	}

	// Exit if either a graphics or a presenting queue hasn't been found
	// 如果未找到图形队列或呈现队列，退出
	if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)  {  // 如果未找到图形或呈现队列
		vks::tools::exitFatal("Could not find a graphics and/or presenting queue!", -1);  // 输出致命错误
	}
	if (graphicsQueueNodeIndex != presentQueueNodeIndex) {  // 如果图形队列和呈现队列不同（当前不支持）
		vks::tools::exitFatal("Separate graphics and presenting queues are not supported yet!", -1);  // 输出致命错误
	}
	queueNodeIndex = graphicsQueueNodeIndex;  // 保存队列族索引（图形和呈现使用同一队列族）

	// Get list of supported surface formats
	// 获取支持的表面格式列表
	uint32_t formatCount;  // 格式数量
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));  // 第一次调用：仅获取数量
	assert(formatCount > 0);  // 确保至少有一种格式
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);  // 表面格式向量
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));  // 第二次调用：获取所有格式

	// We want to get a format that best suits our needs, so we try to get one from a set of preferred formats
	// Initialize the format to the first one returned by the implementation in case we can't find one of the preffered formats
	// 我们希望获得最适合我们需求的格式，因此尝试从首选格式集合中获取一个
	// 如果找不到首选格式，则将格式初始化为实现返回的第一个格式
	VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];  // 选中的格式（默认为第一个）
	std::vector<VkFormat> preferredImageFormats = { 
		VK_FORMAT_B8G8R8A8_UNORM,  // BGRA 8位未归一化（Windows 常用）
		VK_FORMAT_R8G8B8A8_UNORM,  // RGBA 8位未归一化（通用）
		VK_FORMAT_A8B8G8R8_UNORM_PACK32  // ABGR 8位未归一化打包（某些平台）
	};
	for (auto& availableFormat : surfaceFormats) {  // 遍历所有可用格式
		if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end()) {  // 如果找到首选格式
			selectedFormat = availableFormat;  // 使用该格式
			break;  // 退出循环
		}
	}

	colorFormat = selectedFormat.format;  // 保存颜色格式
	colorSpace = selectedFormat.colorSpace;  // 保存颜色空间
}

/**
 * @brief 设置交换链上下文
 * 设置交换链创建和管理所需的 Vulkan 对象，必须在交换链创建之前调用
 * 
 * @param instance Vulkan 实例句柄，用于创建表面和交换链
 * @param physicalDevice 物理设备句柄，用于查询表面属性和能力
 * @param device 逻辑设备句柄，用于创建交换链对象
 */
void VulkanSwapChain::setContext(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
	this->instance = instance;              // 保存实例句柄
	this->physicalDevice = physicalDevice;  // 保存物理设备句柄
	this->device = device;                  // 保存逻辑设备句柄
}

/**
 * @brief 创建交换链并获取其图像
 * 根据给定的宽度和高度创建交换链，并获取交换链图像和图像视图
 * 
 * @param width 交换链宽度的引用（可能会调整以符合交换链的要求）
 * @param height 交换链高度的引用（可能会调整以符合交换链的要求）
 * @param vsync 是否启用垂直同步（默认 false）
 * @param fullscreen 是否全屏模式（默认 false）
 */
void VulkanSwapChain::create(uint32_t& width, uint32_t& height, bool vsync, bool fullscreen)
{
	assert(physicalDevice);  // 确保物理设备已设置
	assert(device);           // 确保逻辑设备已设置
	assert(instance);         // 确保实例已设置

	// 保存当前交换链句柄，以便后续重新创建时使用
	// 这有助于资源重用，并确保我们仍然可以呈现已获取的图像
	VkSwapchainKHR oldSwapchain = swapChain;

	// 获取物理设备表面属性和格式
	// 用于查询表面支持的能力，如最小/最大图像数量、图像尺寸范围等
	VkSurfaceCapabilitiesKHR surfaceCaps;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

	VkExtent2D swapchainExtent = {};  // 交换链图像尺寸
	// 如果宽度（和高度）等于特殊值 0xFFFFFFFF，表面尺寸将由交换链设置
	if (surfaceCaps.currentExtent.width == (uint32_t)-1) {
		// 如果表面尺寸未定义，使用请求的图像尺寸
		swapchainExtent.width = width;
		swapchainExtent.height = height;
	} else {
		// 如果表面尺寸已定义，交换链尺寸必须匹配
		swapchainExtent = surfaceCaps.currentExtent;
		width = surfaceCaps.currentExtent.width;   // 更新宽度以匹配表面
		height = surfaceCaps.currentExtent.height; // 更新高度以匹配表面
	}


	// 选择交换链的呈现模式
	// 呈现模式决定了图像如何从交换链呈现到屏幕
	uint32_t presentModeCount;  // 支持的呈现模式数量
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
	assert(presentModeCount > 0);  // 确保至少有一种呈现模式

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);  // 呈现模式列表
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

	// 根据规范，VK_PRESENT_MODE_FIFO_KHR 模式必须始终存在
	// 此模式等待垂直同步（"v-sync"），确保无撕裂但可能有延迟
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// 如果未请求垂直同步，尝试查找邮箱模式
	// 邮箱模式是可用的最低延迟非撕裂呈现模式
	if (!vsync)
	{
		for (size_t i = 0; i < presentModeCount; i++)
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				// 邮箱模式：如果队列已满，新图像会替换旧图像，最低延迟
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				// 立即模式：立即呈现，可能撕裂但延迟最低
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	// 确定图像数量
	// 使用最小图像数量 + 1 以实现三重缓冲（如果支持）
	uint32_t desiredNumberOfSwapchainImages = surfaceCaps.minImageCount + 1;
	// 如果设备限制了最大图像数量，确保不超过限制
	if ((surfaceCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfaceCaps.maxImageCount)) {
		desiredNumberOfSwapchainImages = surfaceCaps.maxImageCount;
	}

	// 查找表面的变换
	// 变换用于处理显示旋转（如设备旋转）
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		// 优先选择无旋转变换（恒等变换）
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else {
		// 否则使用当前变换
		preTransform = surfaceCaps.currentTransform;
	}

	// 查找支持的复合 Alpha 格式（并非所有设备都支持不透明 Alpha）
	// 复合 Alpha 用于窗口合成，控制窗口与背景的混合方式
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// 简单地选择第一个可用的复合 Alpha 格式
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,        // 不透明（无 Alpha）
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, // 预乘 Alpha
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, // 后乘 Alpha
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,        // 继承 Alpha
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfaceCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	// 创建交换链创建信息结构
	VkSwapchainCreateInfoKHR swapchainCI{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,                                    // 要呈现到的表面
		.minImageCount = desiredNumberOfSwapchainImages,       // 交换链中的最小图像数量
		.imageFormat = colorFormat,                            // 图像格式（从表面格式中选择）
		.imageColorSpace = colorSpace,                         // 颜色空间（从表面格式中选择）
		.imageExtent = { swapchainExtent.width, swapchainExtent.height }, // 图像尺寸
		.imageArrayLayers = 1,                                 // 图像数组层数（2D 图像为 1）
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,     // 图像用途（作为颜色附件）
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,         // 共享模式（独占模式，单队列族使用）
		.queueFamilyIndexCount = 0,                            // 队列族索引数量（独占模式为 0）
		.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform, // 预变换（处理旋转）
		.compositeAlpha = compositeAlpha,                       // 复合 Alpha 模式
		.presentMode = swapchainPresentMode,                   // 呈现模式
		// 设置 clipped 为 VK_TRUE 允许实现丢弃表面区域外的渲染
		.clipped = VK_TRUE,
		// 设置 oldSwapChain 为之前保存的交换链句柄，有助于资源重用并确保仍可呈现已获取的图像
		.oldSwapchain = oldSwapchain,
	};
	// 如果支持，在交换链图像上启用传输源用途（用于截图等）
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	// 如果支持，在交换链图像上启用传输目标用途（用于图像复制等）
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	// 创建交换链对象
	VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

	// 如果重新创建了现有交换链，销毁旧交换链和应用程序拥有的资源
	// 注意：图像由交换链拥有，不需要手动销毁
	if (oldSwapchain != VK_NULL_HANDLE) { 
		// 销毁所有旧的图像视图（应用程序创建的资源）
		for (auto i = 0; i < images.size(); i++) {
			vkDestroyImageView(device, imageViews[i], nullptr);
		}
		// 销毁旧交换链（这会自动销毁其拥有的图像）
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}
	// 获取（新）交换链图像
	// 首先查询图像数量
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
	images.resize(imageCount);  // 调整图像向量大小
	// 获取所有交换链图像句柄
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

	// 为每个交换链图像创建图像视图
	// 图像视图定义了如何访问图像（格式、子资源范围等）
	imageViews.resize(imageCount);
	for (auto i = 0; i < images.size(); i++)
	{
		VkImageViewCreateInfo colorAttachmentView{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = images[i],                                // 要创建视图的图像
			.viewType = VK_IMAGE_VIEW_TYPE_2D,                  // 视图类型（2D 图像）
			.format = colorFormat,                              // 视图格式（与交换链格式相同）
			.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, // 分量重排（默认映射）
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,        // 图像方面（颜色）
				.baseMipLevel = 0,                              // 基础 Mip 级别
				.levelCount = 1,                                 // Mip 级别数量
				.baseArrayLayer = 0,                             // 基础数组层
				.layerCount = 1                                  // 数组层数量
			},
		};
		// 创建图像视图
		VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &imageViews[i]));
	}
}

/**
 * @brief 获取交换链中的下一张图像
 * 从交换链中获取可用于渲染的图像索引
 * 
 * @param presentCompleteSemaphore 信号量，当图像准备好使用时会被发出信号（可选）
 * @param imageIndex 输出的图像索引引用，如果成功获取图像，此值会被设置
 * 
 * @note 通过将超时设置为 UINT64_MAX，将始终等待直到获取到下一张图像或发生实际错误
 * @note 这样我们就不需要处理 VK_NOT_READY 状态
 * 
 * @return 图像获取操作的 VkResult
 */
VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t& imageIndex)
{
	// 通过将超时设置为 UINT64_MAX，将始终等待直到获取到下一张图像或发生实际错误
	// 这样我们就不需要处理 VK_NOT_READY 状态
	return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, &imageIndex);
}

/**
 * @brief 清理交换链资源
 * 释放交换链和表面相关的所有 Vulkan 资源
 * 包括图像视图、交换链对象和表面对象
 */
void VulkanSwapChain::cleanup()
{
	// 如果交换链存在，清理相关资源
	if (swapChain != VK_NULL_HANDLE) {
		// 销毁所有图像视图（应用程序创建的资源）
		for (auto i = 0; i < images.size(); i++) {
			vkDestroyImageView(device, imageViews[i], nullptr);
		}
		// 销毁交换链（这会自动销毁其拥有的图像）
		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}
	// 如果表面存在，销毁表面对象
	if (surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}
	// 重置句柄
	surface = VK_NULL_HANDLE;
	swapChain = VK_NULL_HANDLE;
}

#if defined(_DIRECT2DISPLAY)
/**
 * @brief 创建直接到显示器的表面
 * 使用 Vulkan 显示扩展创建直接输出到显示器的表面，绕过窗口系统
 * 
 * @param width 显示表面宽度（像素）
 * @param height 显示表面高度（像素）
 */
void VulkanSwapChain::createDirect2DisplaySurface(uint32_t width, uint32_t height)
{
	uint32_t displayPropertyCount;  // 显示器属性数量
		
	// 获取显示器属性
	// 首先查询显示器数量
	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayPropertyCount, NULL);
	// 分配显示器属性数组
	VkDisplayPropertiesKHR* pDisplayProperties = new VkDisplayPropertiesKHR[displayPropertyCount];
	// 获取所有显示器属性
	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayPropertyCount, pDisplayProperties);

	// 获取显示平面属性
	// 显示平面用于将图像组合并输出到显示器
	uint32_t planePropertyCount;  // 显示平面数量
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &planePropertyCount, NULL);
	// 分配显示平面属性数组
	VkDisplayPlanePropertiesKHR* pPlaneProperties = new VkDisplayPlanePropertiesKHR[planePropertyCount];
	// 获取所有显示平面属性
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &planePropertyCount, pPlaneProperties);

	VkDisplayKHR display = VK_NULL_HANDLE;  // 选中的显示器句柄
	VkDisplayModeKHR displayMode;            // 选中的显示模式句柄
	VkDisplayModePropertiesKHR* pModeProperties;  // 显示模式属性数组
	bool foundMode = false;                  // 是否找到匹配的显示模式

	// 遍历所有显示器，查找匹配尺寸的显示模式
	for(uint32_t i = 0; i < displayPropertyCount;++i)
	{
		display = pDisplayProperties[i].display;  // 当前显示器
		uint32_t modeCount;  // 显示模式数量
		// 查询显示模式数量
		vkGetDisplayModePropertiesKHR(physicalDevice, display, &modeCount, NULL);
		// 分配显示模式属性数组
		pModeProperties = new VkDisplayModePropertiesKHR[modeCount];
		// 获取所有显示模式属性
		vkGetDisplayModePropertiesKHR(physicalDevice, display, &modeCount, pModeProperties);

		// 遍历所有显示模式，查找匹配请求尺寸的模式
		for (uint32_t j = 0; j < modeCount; ++j)
		{
			const VkDisplayModePropertiesKHR* mode = &pModeProperties[j];

			// 检查显示模式的可见区域是否匹配请求的尺寸
			if (mode->parameters.visibleRegion.width == width && mode->parameters.visibleRegion.height == height)
			{
				displayMode = mode->displayMode;  // 保存匹配的显示模式
				foundMode = true;                  // 标记已找到
				break;
			}
		}
		if (foundMode)
		{
			break;  // 找到匹配模式，退出显示器循环
		}
		delete [] pModeProperties;  // 释放当前显示器的模式数组
	}

	// 如果未找到匹配的显示模式，退出
	if(!foundMode)
	{
		vks::tools::exitFatal("Can't find a display and a display mode!", -1);
		return;
	}

	// 搜索可用的最佳显示平面
	// 显示平面用于将图像输出到显示器
	uint32_t bestPlaneIndex = UINT32_MAX;  // 最佳平面索引
	VkDisplayKHR* pDisplays = NULL;         // 平面支持的显示器数组
	// 遍历所有显示平面
	for(uint32_t i = 0; i < planePropertyCount; i++)
	{
		uint32_t planeIndex=i;  // 当前平面索引
		uint32_t displayCount;   // 平面支持的显示器数量
		// 查询平面支持的显示器数量
		vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, &displayCount, NULL);
		// 释放之前的显示器数组（如果存在）
		if (pDisplays)
		{
			delete [] pDisplays;
		}
		// 分配显示器数组
		pDisplays = new VkDisplayKHR[displayCount];
		// 获取平面支持的所有显示器
		vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, &displayCount, pDisplays);

		// 查找与当前显示器匹配的平面
		bestPlaneIndex = UINT32_MAX;
		for(uint32_t j = 0; j < displayCount; j++)
		{
			if(display == pDisplays[j])
			{
				bestPlaneIndex = i;  // 找到匹配的平面
				break;
			}
		}
		if(bestPlaneIndex != UINT32_MAX)
		{
			break;  // 找到匹配平面，退出循环
		}
	}

	// 如果未找到合适的平面，退出
	if(bestPlaneIndex == UINT32_MAX)
	{
		vks::tools::exitFatal("Can't find a plane for displaying!", -1);
		return;
	}

	// 获取显示平面的能力
	// 能力包括支持的 Alpha 模式、最小/最大源位置等
	VkDisplayPlaneCapabilitiesKHR planeCap;
	vkGetDisplayPlaneCapabilitiesKHR(physicalDevice, displayMode, bestPlaneIndex, &planeCap);
	VkDisplayPlaneAlphaFlagBitsKHR alphaMode = (VkDisplayPlaneAlphaFlagBitsKHR)0;  // Alpha 模式

	// 选择支持的 Alpha 模式（按优先级顺序）
	// Alpha 模式控制平面与背景的混合方式
	if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR)
	{
		// 每像素预乘 Alpha（最高质量）
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR)
	{
		// 每像素 Alpha
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR)
	{
		// 全局 Alpha
		alphaMode = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR)
	{
		// 不透明（无 Alpha）
		alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	}

	// 创建显示平面表面创建信息
	VkDisplaySurfaceCreateInfoKHR surfaceInfo{};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.pNext = NULL;                                                      // 扩展链指针
	surfaceInfo.flags = 0;                                                         // 创建标志
	surfaceInfo.displayMode = displayMode;                                         // 显示模式
	surfaceInfo.planeIndex = bestPlaneIndex;                                       // 平面索引
	surfaceInfo.planeStackIndex = pPlaneProperties[bestPlaneIndex].currentStackIndex; // 平面堆栈索引
	surfaceInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;                // 变换（恒等变换）
	surfaceInfo.globalAlpha = 1.0;                                                 // 全局 Alpha 值（不透明）
	surfaceInfo.alphaMode = alphaMode;                                              // Alpha 模式
	surfaceInfo.imageExtent.width = width;                                         // 图像宽度
	surfaceInfo.imageExtent.height = height;                                       // 图像高度

	// 创建显示平面表面
	VkResult result = vkCreateDisplayPlaneSurfaceKHR(instance, &surfaceInfo, NULL, &surface);
	if (result !=VK_SUCCESS) {
		vks::tools::exitFatal("Failed to create surface!", result);
	}

	// 清理临时分配的内存
	delete[] pDisplays;
	delete[] pModeProperties;
	delete[] pDisplayProperties;
	delete[] pPlaneProperties;
}
#endif 
