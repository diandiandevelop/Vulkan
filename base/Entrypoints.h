/*
 * Vulkan entry points
 * Vulkan 入口点
 *
 * Platform specific macros for the example main entry points
 * 特定于平台的示例主入口点宏
 * 
 * Copyright (C) 2024-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#if defined(_WIN32)
/*
 * Windows
 * Windows 平台入口点
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						/* Windows 窗口过程函数 */ \
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);									/* 处理窗口消息 */ \
	}																								\
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												\
}																									\
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int) /* Windows 主入口点 */ \
{																									\
	for (int32_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };  			/* 解析命令行参数 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	vulkanExample->initVulkan();																	/* 初始化 Vulkan */ \
	vulkanExample->setupWindow(hInstance, WndProc);													/* 设置窗口 */ \
	vulkanExample->prepare();																		/* 准备渲染资源 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
	return 0;																						\
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
/*
 * Android
 * Android 平台入口点
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
void android_main(android_app* state)																/* Android 主入口点 */ \
{																									\
	androidApp = state;																				/* 设置全局 Android 应用状态 */ \
	vks::android::getDeviceConfig();																/* 获取设备配置 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	state->userData = vulkanExample;																/* 设置用户数据 */ \
	state->onAppCmd = VulkanExample::handleAppCommand;												/* 设置应用命令回调 */ \
	state->onInputEvent = VulkanExample::handleAppInput;											/* 设置输入事件回调 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
}

#elif defined(_DIRECT2DISPLAY)
/*
 * Direct-to-display
 * 直接显示平台入口点
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
static void handleEvent()                                											/* 事件处理函数（空实现） */ \
{																									\
}																									\
int main(const int argc, const char *argv[])													    /* 主入口点 */ \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				/* 解析命令行参数 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	vulkanExample->initVulkan();																	/* 初始化 Vulkan */ \
	vulkanExample->prepare();																		/* 准备渲染资源 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
	return 0;																						\
}

#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
/*
 * Direct FB
 * DirectFB 平台入口点
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
static void handleEvent(const DFBWindowEvent *event)												/* DirectFB 窗口事件处理函数 */ \
{																									\
	if (vulkanExample != NULL)																		/* 检查示例实例是否存在 */ \
	{																								\
		vulkanExample->handleEvent(event);															/* 处理窗口事件 */ \
	}																								\
}																									\
int main(const int argc, const char *argv[])													    /* 主入口点 */ \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				/* 解析命令行参数 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	vulkanExample->initVulkan();																	/* 初始化 Vulkan */ \
	vulkanExample->setupWindow();					 												/* 设置窗口 */ \
	vulkanExample->prepare();																		/* 准备渲染资源 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
	return 0;																						\
}

#elif (defined(VK_USE_PLATFORM_WAYLAND_KHR) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
 /*
  * Wayland / headless
  * Wayland / 无头平台入口点
  */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
int main(const int argc, const char *argv[])													    /* 主入口点 */ \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				/* 解析命令行参数 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	vulkanExample->initVulkan();																	/* 初始化 Vulkan */ \
	vulkanExample->setupWindow();					 												/* 设置窗口 */ \
	vulkanExample->prepare();																		/* 准备渲染资源 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
	return 0;																						\
}

#elif defined(VK_USE_PLATFORM_XCB_KHR)
/*
 * X11 Xcb
 * X11 XCB 平台入口点
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
static void handleEvent(const xcb_generic_event_t *event)											/* XCB 事件处理函数 */ \
{																									\
	if (vulkanExample != NULL)																		/* 检查示例实例是否存在 */ \
	{																								\
		vulkanExample->handleEvent(event);															/* 处理 XCB 事件 */ \
	}																								\
}																									\
int main(const int argc, const char *argv[])													    /* 主入口点 */ \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				/* 解析命令行参数 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	vulkanExample->initVulkan();																	/* 初始化 Vulkan */ \
	vulkanExample->setupWindow();					 												/* 设置窗口 */ \
	vulkanExample->prepare();																		/* 准备渲染资源 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
	return 0;																						\
}

#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
/*
 * iOS and macOS (using MoltenVK)
 * iOS 和 macOS 平台入口点（使用 MoltenVK）
 */
#if defined(VK_EXAMPLE_XCODE_GENERATED)
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
int main(const int argc, const char *argv[])														/* 主入口点 */ \
{																									\
	@autoreleasepool																				/* Objective-C 自动释放池 */ \
	{																								\
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };				/* 解析命令行参数 */ \
		vulkanExample = new VulkanExample();														/* 创建 Vulkan 示例实例 */ \
		vulkanExample->initVulkan();																/* 初始化 Vulkan */ \
		vulkanExample->setupWindow(nullptr);														/* 设置窗口 */ \
		vulkanExample->prepare();																	/* 准备渲染资源 */ \
		vulkanExample->renderLoop();																/* 进入渲染循环 */ \
		delete(vulkanExample);																		/* 清理资源 */ \
	}																								\
	return 0;																						\
}
#else
#define VULKAN_EXAMPLE_MAIN()  /* Xcode 生成的项目中为空宏 */
#endif

#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
/*
 * QNX Screen
 * QNX Screen 平台入口点
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		/* Vulkan 示例实例指针 */ \
int main(const int argc, const char *argv[])														/* 主入口点 */ \
{																									\
	for (int i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };						/* 解析命令行参数 */ \
	vulkanExample = new VulkanExample();															/* 创建 Vulkan 示例实例 */ \
	vulkanExample->initVulkan();																	/* 初始化 Vulkan */ \
	vulkanExample->setupWindow();																	/* 设置窗口 */ \
	vulkanExample->prepare();																		/* 准备渲染资源 */ \
	vulkanExample->renderLoop();																	/* 进入渲染循环 */ \
	delete(vulkanExample);																			/* 清理资源 */ \
	return 0;																						\
}
#endif
