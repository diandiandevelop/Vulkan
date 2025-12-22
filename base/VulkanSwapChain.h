/*
* Class wrapping access to the swap chain
* 
* A swap chain is a collection of framebuffers used for rendering and presentation to the windowing system
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <vector>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"

#ifdef __ANDROID__
#include "VulkanAndroid.h"
#endif

#ifdef __APPLE__
#include <sys/utsname.h>
#endif

class VulkanSwapChain
{
private: 
	VkInstance instance{ VK_NULL_HANDLE };              // Vulkan 实例句柄
	VkDevice device{ VK_NULL_HANDLE };                  // Vulkan 设备句柄
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };  // 物理设备句柄
	VkSurfaceKHR surface{ VK_NULL_HANDLE };            // 表面句柄
public:
	VkFormat colorFormat{};                             // 颜色格式
	VkColorSpaceKHR colorSpace{};                      // 颜色空间
	VkSwapchainKHR swapChain{ VK_NULL_HANDLE };         // 交换链句柄
	std::vector<VkImage> images{};                      // 交换链图像列表
	std::vector<VkImageView> imageViews{};             // 图像视图列表
	uint32_t queueNodeIndex{ UINT32_MAX };              // 队列节点索引
	uint32_t imageCount{ 0 };                           // 图像数量

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	void initSurface(void* platformHandle, void* platformWindow);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	void initSurface(ANativeWindow* window);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	void initSurface(IDirectFB* dfb, IDirectFBSurface* window);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	void initSurface(wl_display* display, wl_surface* window);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	void initSurface(xcb_connection_t* connection, xcb_window_t window);
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
	void initSurface(void* view);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	void initSurface(CAMetalLayer* metalLayer);
#elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
	void initSurface(uint32_t width, uint32_t height);
#if defined(_DIRECT2DISPLAY)
	void createDirect2DisplaySurface(uint32_t width, uint32_t height);
#endif
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	void initSurface(screen_context_t screen_context, screen_window_t screen_window);
#endif
	/* Set the Vulkan objects required for swapchain creation and management, must be called before swapchain creation */
	/* 设置交换链创建和管理所需的 Vulkan 对象，必须在交换链创建之前调用 */
	/**
	 * @brief 设置交换链上下文
	 * @param instance Vulkan 实例句柄
	 * @param physicalDevice 物理设备句柄
	 * @param device 逻辑设备句柄
	 */
	void setContext(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
	/**
	* Create the swapchain and get its images with given width and height
	* 创建交换链并获取其图像（使用给定的宽度和高度）
	* 
	* @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param width 交换链宽度的指针（可能会调整以符合交换链的要求）
	* @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param height 交换链高度的指针（可能会调整以符合交换链的要求）
	* @param vsync (Optional, default = false) Can be used to force vsync-ed rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
	* @param vsync (可选，默认 = false) 可用于强制垂直同步渲染（通过使用 VK_PRESENT_MODE_FIFO_KHR 作为呈现模式）
	* @param fullscreen (Optional, default = false) 是否全屏模式
	*/
	void create(uint32_t& width, uint32_t& height, bool vsync = false, bool fullscreen = false);
	/**
	* Acquires the next image in the swap chain
	* 获取交换链中的下一张图像
	* 
	* @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
	* @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
	* 
	* @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
	* 
	* @return VkResult of the image acquisition
	*/
	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t& imageIndex);
	/* Free all Vulkan resources acquired by the swapchain */
	void cleanup();
};
