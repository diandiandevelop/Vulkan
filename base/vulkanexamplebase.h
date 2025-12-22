/*
* Vulkan Example base class
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include "VulkanAndroid.h"
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
#include <directfb.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#elif defined(_DIRECT2DISPLAY)
//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
#include <TargetConditionals.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "vulkan/vulkan.h"

#include "CommandLineParser.hpp"
#include "keycodes.hpp"
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanUIOverlay.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "VulkanInitializers.hpp"
#include "camera.hpp"
#include "benchmark.hpp"

constexpr uint32_t maxConcurrentFrames{ 2 };

class VulkanExampleBase
{
private:
	std::string getWindowTitle() const;
	uint32_t destWidth{};
	uint32_t destHeight{};
	bool resizing = false;
	void handleMouseMove(int32_t x, int32_t y);
	void nextFrame();
	void updateOverlay();
	void createPipelineCache();
	void createCommandPool();
	void createSynchronizationPrimitives();
	void createSurface();
	void createSwapChain();
	void createCommandBuffers();
	void destroyCommandBuffers();
	std::string shaderDir = "glsl";
protected:
	// Returns the path to the root of the glsl, hlsl or slang shader directory.
	// 返回 glsl、hlsl 或 slang 着色器目录根路径
	/**
	 * @brief 获取着色器路径
	 * @return 着色器目录路径
	 */
	std::string getShadersPath() const;

	// Frame counter to display fps
	// 用于显示 FPS 的帧计数器
	uint32_t frameCounter = 0;  // 帧计数器
	uint32_t lastFPS = 0;       // 上一秒的 FPS
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;  // 时间戳
	// Vulkan instance, stores all per-application states
	// Vulkan 实例，存储所有每应用程序状态
	VkInstance instance{ VK_NULL_HANDLE };
	std::vector<std::string> supportedInstanceExtensions;  // 支持的实例扩展列表
	// Physical device (GPU) that Vulkan will use
	// Vulkan 将使用的物理设备（GPU）
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	// Stores physical device properties (for e.g. checking device limits)
	// 存储物理设备属性（例如检查设备限制）
	VkPhysicalDeviceProperties deviceProperties{};
	// Stores the features available on the selected physical device (for e.g. checking if a feature is available)
	// 存储所选物理设备上可用的功能（例如检查功能是否可用）
	VkPhysicalDeviceFeatures deviceFeatures{};
	// Stores all available memory (type) properties for the physical device
	// 存储物理设备的所有可用内存（类型）属性
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
	/** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor) */
	/** @brief 此示例要启用的物理设备功能集（必须在派生构造函数中设置） */
	VkPhysicalDeviceFeatures enabledFeatures{};
	/** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
	/** @brief 此示例要启用的设备扩展集（必须在派生构造函数中设置） */
	std::vector<const char*> enabledDeviceExtensions;
	/** @brief Set of instance extensions to be enabled for this example (must be set in the derived constructor) */
	/** @brief 此示例要启用的实例扩展集（必须在派生构造函数中设置） */
	std::vector<const char*> enabledInstanceExtensions;
	/** @brief Set of layer settings to be enabled for this example (must be set in the derived constructor) */
	/** @brief 此示例要启用的层设置集（必须在派生构造函数中设置） */
	std::vector<VkLayerSettingEXT> enabledLayerSettings;
	/** @brief Optional pNext structure for passing extension structures to device creation */
	/** @brief 用于将扩展结构传递给设备创建的可选 pNext 结构 */
	void* deviceCreatepNextChain = nullptr;
	/** @brief Logical device, application's view of the physical device (GPU) */
	/** @brief 逻辑设备，应用程序对物理设备（GPU）的视图 */
	VkDevice device{ VK_NULL_HANDLE };
	// Handle to the device graphics queue that command buffers are submitted to
	// 命令缓冲区提交到的设备图形队列句柄
	VkQueue queue{ VK_NULL_HANDLE };
	// Depth buffer format (selected during Vulkan initialization)
	// 深度缓冲区格式（在 Vulkan 初始化期间选择）
	VkFormat depthFormat{VK_FORMAT_UNDEFINED};
	// Command buffer pool
	// 命令缓冲区池
	VkCommandPool cmdPool{ VK_NULL_HANDLE };
	// Command buffers used for rendering
	// 用于渲染的命令缓冲区
	std::array<VkCommandBuffer, maxConcurrentFrames> drawCmdBuffers;
	// Global render pass for frame buffer writes
	// 用于帧缓冲区写入的全局渲染通道
	VkRenderPass renderPass{ VK_NULL_HANDLE };
	// List of available frame buffers (same as number of swap chain images)
	// 可用帧缓冲区列表（与交换链图像数量相同）
	std::vector<VkFramebuffer>frameBuffers;
	// Descriptor set pool
	// 描述符集池
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	// List of shader modules created (stored for cleanup)
	// 创建的着色器模块列表（存储用于清理）
	std::vector<VkShaderModule> shaderModules;
	// Pipeline cache object
	// 管线缓存对象
	VkPipelineCache pipelineCache{ VK_NULL_HANDLE };
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	// 包装交换链以将图像（帧缓冲区）呈现给窗口系统
	VulkanSwapChain swapChain;

	// Synchronization related objects and variables
	// These are used to have multiple frame buffers "in flight" to get some CPU/GPU parallelism
	// 同步相关对象和变量
	// 这些用于让多个帧缓冲区"在飞行中"，以获得一些 CPU/GPU 并行性
	uint32_t currentImageIndex{ 0 };  // 当前图像索引
	uint32_t currentBuffer{ 0 };      // 当前缓冲区索引
	std::array<VkSemaphore, maxConcurrentFrames> presentCompleteSemaphores{};  // 呈现完成信号量数组
	std::vector<VkSemaphore> renderCompleteSemaphores{};                     // 渲染完成信号量向量
	std::array<VkFence, maxConcurrentFrames> waitFences;                     // 等待栅栏数组

	bool requiresStencil{ false };  // 是否需要模板缓冲区
public:
	bool prepared = false;   // 是否已准备就绪
	bool resized = false;    // 是否已调整大小
	uint32_t width = 1280;   // 窗口宽度
	uint32_t height = 720;   // 窗口高度

	vks::UIOverlay ui;                    // UI 叠加层
	CommandLineParser commandLineParser;  // 命令行解析器

	/** @brief Last frame time measured using a high performance timer (if available) */
	/** @brief 使用高性能计时器测量的上一帧时间（如果可用） */
	float frameTimer = 1.0f;

	vks::Benchmark benchmark;  // 基准测试对象

	/** @brief Encapsulated physical and logical vulkan device */
	/** @brief 封装的物理和逻辑 Vulkan 设备 */
	vks::VulkanDevice *vulkanDevice{};

	/** @brief Example settings that can be changed e.g. by command line arguments */
	/** @brief 可以通过命令行参数等更改的示例设置 */
	struct Settings {
		/** @brief Activates validation layers (and message output) when set to true */
		/** @brief 设置为 true 时激活验证层（和消息输出） */
		bool validation = false;
		/** @brief Set to true if fullscreen mode has been requested via command line */
		/** @brief 如果通过命令行请求全屏模式，设置为 true */
		bool fullscreen = false;
		/** @brief Set to true if v-sync will be forced for the swapchain */
		/** @brief 如果交换链将强制垂直同步，设置为 true */
		bool vsync = false;
		/** @brief Enable UI overlay */
		/** @brief 启用 UI 叠加层 */
		bool overlay = true;
	} settings;

	/** @brief State of gamepad input (only used on Android) */
	/** @brief 游戏手柄输入状态（仅在 Android 上使用） */
	struct {
		glm::vec2 axisLeft = glm::vec2(0.0f);   // 左摇杆轴
		glm::vec2 axisRight = glm::vec2(0.0f); // 右摇杆轴
	} gamePadState;

	/** @brief State of mouse/touch input */
	/** @brief 鼠标/触摸输入状态 */
	struct {
		struct {
			bool left = false;    // 左键
			bool right = false;  // 右键
			bool middle = false; // 中键
		} buttons;
		glm::vec2 position;  // 位置
	} mouseState;

	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };  // 默认清除颜色

	static std::vector<const char*> args;  // 命令行参数列表

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	// 定义帧率独立的计时器值，限制在 -1.0...1.0 之间
	// 用于动画、旋转等
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	// 用于加速（或减慢）全局计时器的乘数
	float timerSpeed = 0.25f;
	bool paused = false;  // 是否暂停

	Camera camera;  // 相机对象

	std::string title = "Vulkan Example";  // 窗口标题
	std::string name = "vulkanExample";   // 示例名称
	uint32_t apiVersion = VK_API_VERSION_1_0;  // Vulkan API 版本

	/** @brief Default depth stencil attachment used by the default render pass */
	/** @brief 默认渲染通道使用的默认深度模板附件 */
	struct {
		VkImage image;        // 深度模板图像句柄
		VkDeviceMemory memory;  // 内存句柄
		VkImageView view;     // 图像视图句柄
	} depthStencil{};

	// OS specific
	// 特定于操作系统的变量
#if defined(_WIN32)
	HWND window;              // Windows 窗口句柄
	HINSTANCE windowInstance; // Windows 实例句柄
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	// true if application has focused, false if moved to background
	// 如果应用程序已获得焦点为 true，如果移动到后台为 false
	bool focused = false;  // 是否获得焦点
	struct TouchPos {
		int32_t x;  // 触摸 X 坐标
		int32_t y;  // 触摸 Y 坐标
	} touchPos{};  // 触摸位置
	bool touchDown = false;     // 是否按下
	double touchTimer = 0.0;    // 触摸计时器
	int64_t lastTapTime = 0;    // 最后点击时间
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
	void* view;
#if defined(VK_USE_PLATFORM_METAL_EXT)
	CAMetalLayer* metalLayer;
#endif
#if defined(VK_EXAMPLE_XCODE_GENERATED)
	bool quit = false;
#endif
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	bool quit = false;
	IDirectFB *dfb = nullptr;
	IDirectFBDisplayLayer *layer = nullptr;
	IDirectFBWindow *window = nullptr;
	IDirectFBSurface *surface = nullptr;
	IDirectFBEventBuffer *event_buffer = nullptr;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	wl_display *display = nullptr;
	wl_registry *registry = nullptr;
	wl_compositor *compositor = nullptr;
	struct xdg_wm_base *shell = nullptr;
	wl_seat *seat = nullptr;
	wl_pointer *pointer = nullptr;
	wl_keyboard *keyboard = nullptr;
	wl_surface *surface = nullptr;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	bool quit = false;
	bool configured = false;

#elif defined(_DIRECT2DISPLAY)
	bool quit = false;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	bool quit = false;
	xcb_connection_t *connection;
	xcb_screen_t *screen;
	xcb_window_t window;
	xcb_intern_atom_reply_t *atom_wm_delete_window;
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
	bool quit = false;
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	screen_context_t screen_context = nullptr;
	screen_window_t screen_window = nullptr;
	screen_event_t screen_event = nullptr;
	bool quit = false;
#endif

	/** @brief Default base class constructor */
	/** @brief 默认基类构造函数 */
	VulkanExampleBase();
	/**
	 * @brief 析构函数（虚函数）
	 */
	virtual ~VulkanExampleBase();
	/** @brief Setup the vulkan instance, enable required extensions and connect to the physical device (GPU) */
	/** @brief 设置 Vulkan 实例，启用所需扩展并连接到物理设备（GPU） */
	/**
	 * @brief 初始化 Vulkan
	 * @return 成功返回 true，失败返回 false
	 */
	bool initVulkan();

#if defined(_WIN32)
	void setupConsole(std::string title);
	void setupDPIAwareness();
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	static int32_t handleAppInput(struct android_app* app, AInputEvent* event);
	static void handleAppCommand(android_app* app, int32_t cmd);
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
	void* setupWindow(void* view);
	void displayLinkOutputCb();
	void mouseDragged(float x, float y);
	void windowWillResize(float x, float y);
	void windowDidResize();
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	IDirectFBSurface *setupWindow();
	void handleEvent(const DFBWindowEvent *event);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	struct xdg_surface *setupWindow();
	void initWaylandConnection();
	void setSize(int width, int height);
	static void registryGlobalCb(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
	void registryGlobal(struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
	static void registryGlobalRemoveCb(void *data, struct wl_registry *registry, uint32_t name);
	static void seatCapabilitiesCb(void *data, wl_seat *seat, uint32_t caps);
	void seatCapabilities(wl_seat *seat, uint32_t caps);
	static void pointerEnterCb(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);
	static void pointerLeaveCb(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
	static void pointerMotionCb(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
	void pointerMotion(struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
	static void pointerButtonCb(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	void pointerButton(struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	static void pointerAxisCb(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
	void pointerAxis(struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
	static void keyboardKeymapCb(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size);
	static void keyboardEnterCb(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
	static void keyboardLeaveCb(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
	static void keyboardKeyCb(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	void keyboardKey(struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	static void keyboardModifiersCb(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

#elif defined(_DIRECT2DISPLAY)
//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	xcb_window_t setupWindow();
	void initxcbConnection();
	void handleEvent(const xcb_generic_event_t *event);
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	void setupWindow();
	void handleEvent();
#else
	void setupWindow();
#endif
	/** @brief (Virtual) Creates the application wide Vulkan instance */
	/** @brief （虚函数）创建应用程序范围的 Vulkan 实例 */
	/**
	 * @brief 创建 Vulkan 实例
	 * @return 创建结果
	 */
	virtual VkResult createInstance();
	/** @brief (Pure virtual) Render function to be implemented by the sample application */
	/** @brief （纯虚函数）渲染函数，由示例应用程序实现 */
	virtual void render() = 0;
	/** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
	/** @brief （虚函数）按键后调用，可用于自定义按键处理 */
	/**
	 * @brief 按键处理
	 * @param keyCode 按键代码
	 */
	virtual void keyPressed(uint32_t);
	/** @brief (Virtual) Called after the mouse cursor moved and before internal events (like camera rotation) is handled */
	/** @brief （虚函数）鼠标移动后调用，在处理内部事件（如相机旋转）之前 */
	/**
	 * @brief 鼠标移动处理
	 * @param x X 坐标
	 * @param y Y 坐标
	 * @param handled 是否已处理（输出参数）
	 */
	virtual void mouseMoved(double x, double y, bool &handled);
	/** @brief (Virtual) Called when the window has been resized, can be used by the sample application to recreate resources */
	/** @brief （虚函数）窗口调整大小时调用，示例应用程序可用于重新创建资源 */
	virtual void windowResized();
	/** @brief (Virtual) Setup default depth and stencil views */
	/** @brief （虚函数）设置默认深度和模板视图 */
	virtual void setupDepthStencil();
	/** @brief (Virtual) Setup default framebuffers for all requested swapchain images */
	/** @brief （虚函数）为所有请求的交换链图像设置默认帧缓冲区 */
	virtual void setupFrameBuffer();
	/** @brief (Virtual) Setup a default renderpass */
	/** @brief （虚函数）设置默认渲染通道 */
	virtual void setupRenderPass();
	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	/** @brief （虚函数）读取物理设备功能后调用，可用于设置要在设备上启用的功能 */
	virtual void getEnabledFeatures();
	/** @brief (Virtual) Called after the physical device extensions have been read, can be used to enable extensions based on the supported extension listing*/
	/** @brief （虚函数）读取物理设备扩展后调用，可根据支持的扩展列表启用扩展 */
	virtual void getEnabledExtensions();

	/** @brief Prepares all Vulkan resources and functions required to run the sample */
	/** @brief 准备运行示例所需的所有 Vulkan 资源和函数 */
	virtual void prepare();

	/** @brief Loads a SPIR-V shader file for the given shader stage */
	/** @brief 为给定的着色器阶段加载 SPIR-V 着色器文件 */
	/**
	 * @brief 加载着色器
	 * @param fileName 文件名
	 * @param stage 着色器阶段
	 * @return 着色器阶段创建信息
	 */
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

	/**
	 * @brief 窗口调整大小（内部调用）
	 */
	void windowResize();

	/** @brief Entry point for the main render loop */
	/** @brief 主渲染循环的入口点 */
	void renderLoop();

	/** @brief Adds the drawing commands for the ImGui overlay to the given command buffer */
	/** @brief 将 ImGui 叠加层的绘制命令添加到给定的命令缓冲区 */
	/**
	 * @brief 绘制 UI
	 * @param commandBuffer 命令缓冲区句柄
	 */
	void drawUI(const VkCommandBuffer commandBuffer);

	/** Prepare the next frame for workload submission by acquiring the next swap chain image and waiting for the previous command buffer to finish */
	/** 通过获取下一个交换链图像并等待上一个命令缓冲区完成，准备下一帧以提交工作负载 */
	/**
	 * @brief 准备下一帧
	 * @param waitForFence 是否等待栅栏（默认 true）
	 */
	void prepareFrame(bool waitForFence = true);
	/** @brief Presents the current image to the swap chain */
	/** @brief 将当前图像呈现到交换链 */
	/**
	 * @brief 提交帧
	 * @param skipQueueSubmit 是否跳过队列提交（默认 false）
	 */
	void submitFrame(bool skipQueueSubmit = false);

	/** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
	/** @brief （虚函数）UI 叠加层更新时调用，可用于向叠加层添加自定义元素 */
	/**
	 * @brief UI 叠加层更新回调
	 * @param overlay UI 叠加层指针
	 */
	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay);

#if defined(_WIN32)
	/**
	 * @brief Windows 消息处理回调（虚函数）
	 * @param hWnd 窗口句柄
	 * @param uMsg 消息类型
	 * @param wParam 消息参数
	 * @param lParam 消息参数
	 */
	virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
};

#include "Entrypoints.h"