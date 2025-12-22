/*
 * Vulkan device class
 *
 * Encapsulates a physical Vulkan device and its logical representation
 *
 * Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "VulkanBuffer.h"
#include "VulkanTools.h"
#include "vulkan/vulkan.h"
#include <algorithm>
#include <assert.h>
#include <exception>

namespace vks
{
struct VulkanDevice
{
	/** @brief Physical device representation */
	/** @brief 物理设备表示 */
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };;
	/** @brief Logical device representation (application's view of the device) */
	/** @brief 逻辑设备表示（应用程序对设备的视图） */
	VkDevice logicalDevice{ VK_NULL_HANDLE };
	/** @brief Properties of the physical device including limits that the application can check against */
	/** @brief 物理设备属性，包括应用程序可以检查的限制 */
	VkPhysicalDeviceProperties properties{};
	/** @brief Features of the physical device that an application can use to check if a feature is supported */
	/** @brief 物理设备功能，应用程序可用于检查是否支持某个功能 */
	VkPhysicalDeviceFeatures features{};
	/** @brief Features that have been enabled for use on the physical device */
	/** @brief 已在物理设备上启用的功能 */
	VkPhysicalDeviceFeatures enabledFeatures{};
	/** @brief Memory types and heaps of the physical device */
	/** @brief 物理设备的内存类型和堆 */
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	/** @brief Queue family properties of the physical device */
	/** @brief 物理设备的队列族属性 */
	std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
	/** @brief List of extensions supported by the device */
	/** @brief 设备支持的扩展列表 */
	std::vector<std::string> supportedExtensions{};
	/** @brief Default command pool for the graphics queue family index */
	/** @brief 图形队列族索引的默认命令池 */
	VkCommandPool commandPool{ VK_NULL_HANDLE };;
	/** @brief Contains queue family indices */
	/** @brief 包含队列族索引 */
	struct
	{
		uint32_t graphics;   // 图形队列族索引
		uint32_t compute;    // 计算队列族索引
		uint32_t transfer;  // 传输队列族索引
	} queueFamilyIndices;
	operator VkDevice() const
	{
		return logicalDevice;
	};
	/**
	 * @brief 构造函数
	 * @param physicalDevice 要使用的物理设备
	 */
	explicit VulkanDevice(VkPhysicalDevice physicalDevice);
	/**
	 * @brief 析构函数，释放逻辑设备
	 */
	~VulkanDevice();
	/**
	 * @brief 获取具有所有请求属性位的内存类型索引
	 * @param typeBits 资源支持的内存类型的位掩码（来自 VkMemoryRequirements）
	 * @param properties 请求的内存类型的属性位掩码
	 * @param memTypeFound (可选) 指向布尔值的指针，如果找到匹配的内存类型则设置为 true
	 * @return 请求的内存类型索引
	 */
	uint32_t        getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
	/**
	 * @brief 获取支持请求队列标志的队列族索引
	 * @param queueFlags 要查找队列族索引的队列标志
	 * @return 匹配标志的队列族索引
	 */
	uint32_t        getQueueFamilyIndex(VkQueueFlags queueFlags) const;
	/**
	 * @brief 创建逻辑设备
	 * @param enabledFeatures 启用的设备功能
	 * @param enabledExtensions 启用的扩展列表
	 * @param pNextChain 扩展链指针
	 * @param useSwapChain 是否使用交换链
	 * @param requestedQueueTypes 请求的队列类型
	 * @return 创建结果
	 */
	VkResult        createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char *> enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
	/**
	 * @brief 创建缓冲区（返回句柄版本）
	 * @param usageFlags 缓冲区使用标志
	 * @param memoryPropertyFlags 内存属性标志
	 * @param size 缓冲区大小
	 * @param buffer 输出的缓冲区句柄
	 * @param memory 输出的内存句柄
	 * @param data (可选) 要复制到缓冲区的数据
	 * @return 创建结果
	 */
	VkResult        createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data = nullptr);
	/**
	 * @brief 创建缓冲区（Buffer 结构体版本）
	 * @param usageFlags 缓冲区使用标志
	 * @param memoryPropertyFlags 内存属性标志
	 * @param buffer 输出的 Buffer 结构体
	 * @param size 缓冲区大小
	 * @param data (可选) 要复制到缓冲区的数据
	 * @return 创建结果
	 */
	VkResult        createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, vks::Buffer *buffer, VkDeviceSize size, void *data = nullptr);
	/**
	 * @brief 复制缓冲区
	 * @param src 源缓冲区
	 * @param dst 目标缓冲区
	 * @param queue 用于复制的队列
	 * @param copyRegion (可选) 复制区域
	 */
	void            copyBuffer(vks::Buffer *src, vks::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion = nullptr);
	/**
	 * @brief 创建命令池
	 * @param queueFamilyIndex 队列族索引
	 * @param createFlags 创建标志
	 * @return 命令池句柄
	 */
	VkCommandPool   createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	/**
	 * @brief 创建命令缓冲区（指定命令池）
	 * @param level 命令缓冲区级别
	 * @param pool 命令池句柄
	 * @param begin 是否立即开始记录
	 * @return 命令缓冲区句柄
	 */
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
	/**
	 * @brief 创建命令缓冲区（使用默认命令池）
	 * @param level 命令缓冲区级别
	 * @param begin 是否立即开始记录
	 * @return 命令缓冲区句柄
	 */
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
	/**
	 * @brief 刷新命令缓冲区（指定命令池）
	 * @param commandBuffer 命令缓冲区句柄
	 * @param queue 提交队列
	 * @param pool 命令池句柄
	 * @param free 是否释放命令缓冲区
	 */
	void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
	/**
	 * @brief 刷新命令缓冲区（使用默认命令池）
	 * @param commandBuffer 命令缓冲区句柄
	 * @param queue 提交队列
	 * @param free 是否释放命令缓冲区
	 */
	void            flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);
	/**
	 * @brief 检查扩展是否受支持
	 * @param extension 扩展名称
	 * @return 如果支持返回 true
	 */
	bool            extensionSupported(std::string extension);
	/**
	 * @brief 获取支持的深度格式
	 * @param checkSamplingSupport 是否检查采样支持
	 * @return 深度格式
	 */
	VkFormat        getSupportedDepthFormat(bool checkSamplingSupport);
};
}        // namespace vks
