/*
 * Vulkan examples debug wrapper
 *
 * Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once
#include "vulkan/vulkan.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif
#ifdef __ANDROID__
#include "VulkanAndroid.h"
#endif
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vks
{
	namespace debug
	{
		extern bool logToFile;                    // 是否将日志写入文件
		extern std::string logFileName;           // 日志文件名

		// Default debug callback
		// 默认调试回调函数
		/**
		 * @brief Vulkan 调试工具消息回调函数
		 * @param messageSeverity 消息严重程度标志
		 * @param messageType 消息类型标志
		 * @param pCallbackData 回调数据指针
		 * @param pUserData 用户数据指针
		 * @return 返回 VK_FALSE 以继续执行，VK_TRUE 以中止调用
		 */
		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		// Load debug function pointers and set debug callback
		// 加载调试函数指针并设置调试回调
		/**
		 * @brief 设置调试功能，加载函数指针并创建调试消息信使
		 * @param instance Vulkan 实例句柄
		 */
		void setupDebuging(VkInstance instance);
		// Clear debug callback
		// 清除调试回调
		/**
		 * @brief 释放调试回调
		 * @param instance Vulkan 实例句柄
		 */
		void freeDebugCallback(VkInstance instance);
		// Used to populate a VkDebugUtilsMessengerCreateInfoEXT with our example messenger function and desired flags
		// 用于使用示例信使函数和所需标志填充 VkDebugUtilsMessengerCreateInfoEXT
		/**
		 * @brief 设置调试消息信使创建信息
		 * @param debugUtilsMessengerCI 要填充的调试消息信使创建信息结构
		 */
		void setupDebugingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCI);
		/**
		 * @brief 记录日志消息
		 * @param message 要记录的日志消息
		 */
		void log(std::string message);
	}

	// Wrapper for the VK_EXT_debug_utils extension
	// These can be used to name Vulkan objects for debugging tools like RenderDoc
	// VK_EXT_debug_utils 扩展的包装器
	// 可用于为 Vulkan 对象命名，以便在 RenderDoc 等调试工具中使用
	namespace debugutils
	{
		/**
		 * @brief 设置调试工具扩展
		 * @param instance Vulkan 实例句柄
		 */
		void setup(VkInstance instance);
		/**
		 * @brief 在命令缓冲区中开始调试标签
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param caption 标签标题
		 * @param color 标签颜色（RGBA）
		 */
		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color);
		/**
		 * @brief 在命令缓冲区中结束调试标签
		 * @param cmdbuffer 命令缓冲区句柄
		 */
		void cmdEndLabel(VkCommandBuffer cmdbuffer);
	}
}
