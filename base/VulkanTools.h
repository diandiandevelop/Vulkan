/*
 * Assorted Vulkan helper functions
 *
 * Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "vulkan/vulkan.h"
#include "VulkanInitializers.hpp"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#if defined(_WIN32)
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#elif defined(__ANDROID__)
#include "VulkanAndroid.h"
#include <android/asset_manager.h>
#endif

// Custom define for better code readability
// 自定义定义以提高代码可读性
/**
 * @brief Vulkan 标志无值（用于表示无标志）
 */
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
// 默认栅栏超时时间（纳秒）
/**
 * @brief 默认栅栏超时时间（100秒）
 */
#define DEFAULT_FENCE_TIMEOUT 100000000000

// Macro to check and display Vulkan return results
// 用于检查和显示 Vulkan 返回结果的宏
/**
 * @brief 检查 Vulkan 函数调用结果，如果失败则输出错误并断言
 * @param f Vulkan 函数调用表达式
 * 
 * 此宏会执行 Vulkan 函数调用，检查返回值是否为 VK_SUCCESS。
 * 如果失败，会输出错误信息（包含文件名和行号）并触发断言。
 * Android 平台使用 LOGE 输出日志，其他平台使用 std::cout。
 */
#if defined(__ANDROID__)
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					/* 执行 Vulkan 函数调用并获取结果 */ \
	if (res != VK_SUCCESS)																				/* 如果结果不是成功 */ \
	{																									\
		LOGE("Fatal : VkResult is \" %s \" in %s at line %d", vks::tools::errorString(res).c_str(), __FILE__, __LINE__); /* Android 日志输出 */ \
		assert(res == VK_SUCCESS);																		/* 触发断言 */ \
	}																									\
}
#else
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					/* 执行 Vulkan 函数调用并获取结果 */ \
	if (res != VK_SUCCESS)																				/* 如果结果不是成功 */ \
	{																									\
		std::cout << "Fatal : VkResult is \"" << vks::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; /* 标准输出错误信息 */ \
		assert(res == VK_SUCCESS);																		/* 触发断言 */ \
	}																									\
}
#endif

/**
 * @brief 获取资源文件路径
 * @return 资源文件目录路径
 */
const std::string getAssetPath();
/**
 * @brief 获取着色器基础路径
 * @return 着色器目录路径
 */
const std::string getShaderBasePath();

namespace vks
{
	namespace tools
	{
		/** @brief Setting this path chnanges the place where the samples looks for assets and shaders */
		/** @brief 设置此路径会更改示例查找资源和着色器的位置 */
		extern std::string resourcePath;

		/** @brief Disable message boxes on fatal errors */
		/** @brief 在致命错误时禁用消息框 */
		extern bool errorModeSilent;

		/** @brief Returns an error code as a string */
		/** @brief 将错误代码返回为字符串 */
		/** @param errorCode Vulkan 结果错误代码 */
		/** @return 错误代码的字符串表示 */
		std::string errorString(VkResult errorCode);

		/** @brief Returns the device type as a string */
		/** @brief 将设备类型返回为字符串 */
		/** @param type 物理设备类型 */
		/** @return 设备类型的字符串表示 */
		std::string physicalDeviceTypeString(VkPhysicalDeviceType type);

		// Selected a suitable supported depth format starting with 32 bit down to 16 bit
		// Returns false if none of the depth formats in the list is supported by the device
		// 选择一个合适的支持的深度格式，从 32 位开始到 16 位
		// 如果设备不支持列表中的任何深度格式，则返回 false
		/** @param physicalDevice 物理设备句柄 */
		/** @param depthFormat 输出的深度格式 */
		/** @return 如果找到支持的格式返回 true */
		VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat);
		// Same as getSupportedDepthFormat but will only select formats that also have stencil
		// 与 getSupportedDepthFormat 相同，但仅选择也具有模板的格式
		/** @param physicalDevice 物理设备句柄 */
		/** @param depthStencilFormat 输出的深度模板格式 */
		/** @return 如果找到支持的格式返回 true */
		VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat* depthStencilFormat);

		// Returns true a given format support LINEAR filtering
		// 如果给定格式支持线性过滤，返回 true
		/** @param physicalDevice 物理设备句柄 */
		/** @param format 要检查的格式 */
		/** @param tiling 图像平铺方式 */
		/** @return 如果支持线性过滤返回 true */
		VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);
		// Returns true if a given format has a stencil part
		// 如果给定格式具有模板部分，返回 true
		/** @param format 要检查的格式 */
		/** @return 如果格式有模板部分返回 true */
		VkBool32 formatHasStencil(VkFormat format);

		// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
		// 将用于在子资源上设置图像布局的图像内存屏障放入给定的命令缓冲区
		/** @param cmdbuffer 命令缓冲区句柄 */
		/** @param image 图像句柄 */
		/** @param oldImageLayout 旧的图像布局 */
		/** @param newImageLayout 新的图像布局 */
		/** @param subresourceRange 子资源范围 */
		/** @param srcStageMask 源管线阶段掩码 */
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		// Uses a fixed sub resource layout with first mip level and layer
		// 使用固定的子资源布局，包含第一个 Mip 级别和层
		/**
		 * @brief 设置图像布局（简化版本，使用固定的子资源范围）
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param image 图像句柄
		 * @param aspectMask 图像方面掩码
		 * @param oldImageLayout 旧的图像布局
		 * @param newImageLayout 新的图像布局
		 * @param srcStageMask 源管线阶段掩码（默认所有命令）
		 * @param dstStageMask 目标管线阶段掩码（默认所有命令）
		 */
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		/** @brief Insert an image memory barrier into the command buffer */
		/** @brief 将图像内存屏障插入命令缓冲区 */
		/**
		 * @brief 插入图像内存屏障
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param image 图像句柄
		 * @param srcAccessMask 源访问掩码
		 * @param dstAccessMask 目标访问掩码
		 * @param oldImageLayout 旧的图像布局
		 * @param newImageLayout 新的图像布局
		 * @param srcStageMask 源管线阶段掩码
		 * @param dstStageMask 目标管线阶段掩码
		 * @param subresourceRange 子资源范围
		 */
		void insertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange);

		// Display error message and exit on fatal error
		// 显示错误消息并在致命错误时退出
		/**
		 * @brief 致命错误退出（使用退出代码）
		 * @param message 错误消息
		 * @param exitCode 退出代码
		 */
		void exitFatal(const std::string& message, int32_t exitCode);
		/**
		 * @brief 致命错误退出（使用 Vulkan 结果代码）
		 * @param message 错误消息
		 * @param resultCode Vulkan 结果代码
		 */
		void exitFatal(const std::string& message, VkResult resultCode);

		// Load a SPIR-V shader (binary)
		// 加载 SPIR-V 着色器（二进制）
		/**
		 * @brief 加载 SPIR-V 着色器模块
		 * @param assetManager Android 资源管理器（仅 Android）
		 * @param fileName 着色器文件名
		 * @param device Vulkan 设备句柄
		 * @return 着色器模块句柄
		 */
#if defined(__ANDROID__)
		VkShaderModule loadShader(AAssetManager* assetManager, const char *fileName, VkDevice device);
#else
		/**
		 * @brief 加载 SPIR-V 着色器模块
		 * @param fileName 着色器文件名
		 * @param device Vulkan 设备句柄
		 * @return 着色器模块句柄
		 */
		VkShaderModule loadShader(const char *fileName, VkDevice device);
#endif

		/** @brief Checks if a file exists */
		/** @brief 检查文件是否存在 */
		/**
		 * @brief 检查文件是否存在
		 * @param filename 文件名
		 * @return 如果文件存在返回 true
		 */
		bool fileExists(const std::string &filename);

		/**
		 * @brief 计算对齐后的大小（32位）
		 * @param value 原始值
		 * @param alignment 对齐值
		 * @return 对齐后的大小
		 */
		uint32_t alignedSize(uint32_t value, uint32_t alignment);
		/**
		 * @brief 计算对齐后的大小（VkDeviceSize）
		 * @param value 原始值
		 * @param alignment 对齐值
		 * @return 对齐后的大小
		 */
		VkDeviceSize alignedVkSize(VkDeviceSize value, VkDeviceSize alignment);
	}
}
