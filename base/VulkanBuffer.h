/*
* Vulkan buffer class
*
* Encapsulates a Vulkan buffer
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanTools.h"

namespace vks
{	
	/**
	* @brief Encapsulates access to a Vulkan buffer backed up by device memory
	* @note To be filled by an external source like the VulkanDevice
	* @brief 封装对由设备内存支持的 Vulkan 缓冲区的访问
	* @note 由外部源（如 VulkanDevice）填充
	*/
	struct Buffer
	{
		VkDevice device;                                    // Vulkan 设备句柄
		VkBuffer buffer = VK_NULL_HANDLE;                  // Vulkan 缓冲区对象句柄
		VkDeviceMemory memory = VK_NULL_HANDLE;            // 设备内存句柄
		VkDescriptorBufferInfo descriptor;                 // 描述符缓冲区信息
		VkDeviceSize size = 0;                              // 缓冲区大小（字节）
		VkDeviceSize alignment = 0;                        // 内存对齐要求（字节）
		void* mapped = nullptr;                             // 映射后的内存指针
		/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
		/** @brief 缓冲区创建时由外部源填充的使用标志（用于后续查询） */
		VkBufferUsageFlags usageFlags;
		/** @brief Memory property flags to be filled by external source at buffer creation (to query at some later point) */
		/** @brief 缓冲区创建时由外部源填充的内存属性标志（用于后续查询） */
		VkMemoryPropertyFlags memoryPropertyFlags;
		uint64_t deviceAddress;                            // 设备地址（用于缓冲区设备地址扩展）
		VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void unmap();
		VkResult bind(VkDeviceSize offset = 0);
		void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void copyTo(void* data, VkDeviceSize size);
		VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void destroy();
	};
}