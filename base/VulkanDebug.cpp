/*
 * Vulkan examples debug wrapper
 *
 * Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "VulkanDebug.h"
#include <iostream>

namespace vks
{
	namespace debug
	{
		bool logToFile{ false };
		std::string logFileName{ "validation_output.txt" };

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;      // 创建调试消息信使的函数指针
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;    // 销毁调试消息信使的函数指针
		VkDebugUtilsMessengerEXT debugUtilsMessenger;                           // 调试消息信使句柄

		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			// Select prefix depending on flags passed to the callback
			// 根据传递给回调的标志选择前缀
			std::string prefix;

			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
				prefix = "VERBOSE: ";
#if defined(_WIN32)
				if (!logToFile) {
					prefix = "\033[32m" + prefix + "\033[0m";
				}
#endif
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
				prefix = "INFO: ";
#if defined(_WIN32)
				if (!logToFile) {
					prefix = "\033[36m" + prefix + "\033[0m";
				}
#endif
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
				prefix = "WARNING: ";
#if defined(_WIN32)
				if (!logToFile) {
					prefix = "\033[33m" + prefix + "\033[0m";
				}
#endif
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				prefix = "ERROR: ";
#if defined(_WIN32)
				if (!logToFile) {
					prefix = "\033[31m" + prefix + "\033[0m";
				}
#endif
			}


			// Display message to default output (console/logcat)
			// 将消息显示到默认输出（控制台/logcat）
			std::stringstream debugMessage;
			if (pCallbackData->pMessageIdName) {
				debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
			}
			else {
				debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "] : " << pCallbackData->pMessage;
			}

#if defined(__ANDROID__)
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				LOGE("%s", debugMessage.str().c_str());
			} else {
				LOGD("%s", debugMessage.str().c_str());
			}
#else
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				std::cerr << debugMessage.str() << "\n\n";
			} else {
				std::cout << debugMessage.str() << "\n\n";
			}
			if (logToFile) {
				log(debugMessage.str());
			}
			fflush(stdout);
#endif


			// The return value of this callback controls whether the Vulkan call that caused the validation message will be aborted or not
			// We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message to abort
			// If you instead want to have calls abort, pass in VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT
			// 此回调的返回值控制是否中止导致验证消息的 Vulkan 调用
			// 我们返回 VK_FALSE，因为我们不希望导致验证消息的 Vulkan 调用被中止
			// 如果您希望调用被中止，传递 VK_TRUE，函数将返回 VK_ERROR_VALIDATION_FAILED_EXT
			return VK_FALSE;
		}

		/**
		 * @brief 设置调试消息信使创建信息
		 * 配置调试消息信使的创建参数，包括消息严重程度和类型过滤
		 * 
		 * @param debugUtilsMessengerCI 要填充的调试消息信使创建信息结构引用
		 */
		void setupDebugingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCI)
		{
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;  // 结构体类型
			// 设置消息严重程度过滤：仅接收警告和错误消息
			debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			// 设置消息类型过滤：接收一般消息和验证层消息
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			// 设置用户回调函数
			debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessageCallback;
		}

		/**
		 * @brief 记录日志消息
		 * 将日志消息写入文件（如果启用了文件日志）
		 * 
		 * @param message 要记录的日志消息字符串
		 */
		void log(std::string message)
		{
			if (logToFile) {  // 如果启用了文件日志
				time_t timestamp;  // 时间戳
				time(&timestamp);   // 获取当前时间
				// 打开日志文件（追加模式）
				std::ofstream logfile;
				logfile.open(logFileName, std::ios_base::app);
				// 写入时间戳和消息（移除时间字符串中的换行符）
				logfile << strtok(ctime(&timestamp), "\n") << ": " << message << std::endl;
				logfile.close();  // 关闭文件
			}
		}

		/**
		 * @brief 设置调试功能
		 * 加载调试函数指针并创建调试消息信使
		 * 
		 * @param instance Vulkan 实例句柄，用于获取函数指针和创建信使
		 */
		void setupDebuging(VkInstance instance)
		{
			// 加载创建调试消息信使的函数指针
			vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			// 加载销毁调试消息信使的函数指针
			vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

			// 创建调试消息信使创建信息
			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
			setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
			// 创建调试消息信使
			VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
			assert(result == VK_SUCCESS);  // 确保创建成功
		}

		/**
		 * @brief 释放调试回调
		 * 销毁调试消息信使并释放相关资源
		 * 
		 * @param instance Vulkan 实例句柄，用于销毁信使
		 */
		void freeDebugCallback(VkInstance instance)
		{
			// 如果信使已创建，销毁它
			if (debugUtilsMessenger != VK_NULL_HANDLE)
			{
				vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
			}
		}
	}

	namespace debugutils
	{
		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };      // 开始调试标签的函数指针
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };            // 结束调试标签的函数指针
		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };      // 插入调试标签的函数指针

		/**
		 * @brief 设置调试工具扩展
		 * 加载调试工具扩展的函数指针，用于在命令缓冲区中添加调试标签
		 * 
		 * @param instance Vulkan 实例句柄，用于获取函数指针
		 */
		void setup(VkInstance instance)
		{
			// 加载开始调试标签的函数指针
			vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
			// 加载结束调试标签的函数指针
			vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
			// 加载插入调试标签的函数指针
			vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
		}

		/**
		 * @brief 在命令缓冲区中开始调试标签
		 * 在命令缓冲区中插入一个开始调试标签，用于在 RenderDoc 等工具中标记渲染区域
		 * 
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param caption 标签标题文本
		 * @param color 标签颜色（RGBA，范围 0.0-1.0）
		 */
		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color)
		{
			// 如果函数指针未加载，直接返回
			if (!vkCmdBeginDebugUtilsLabelEXT) {
				return;
			}
			// 创建调试标签信息结构
			VkDebugUtilsLabelEXT labelInfo{};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;  // 结构体类型
			labelInfo.pLabelName = caption.c_str();                      // 标签名称
			// 复制颜色数据到标签信息（RGBA，4 个浮点数）
			memcpy(labelInfo.color, &color[0], sizeof(float) * 4);
			// 在命令缓冲区中插入开始标签命令
			vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);
		}

		/**
		 * @brief 在命令缓冲区中结束调试标签
		 * 结束当前活动的调试标签区域
		 * 
		 * @param cmdbuffer 命令缓冲区句柄
		 */
		void cmdEndLabel(VkCommandBuffer cmdbuffer)
		{
			// 如果函数指针未加载，直接返回
			if (!vkCmdEndDebugUtilsLabelEXT) {
				return;
			}
			// 在命令缓冲区中插入结束标签命令
			vkCmdEndDebugUtilsLabelEXT(cmdbuffer);
		}

	};

}
