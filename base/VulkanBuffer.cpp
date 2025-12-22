/*
* Vulkan buffer class
*
* Encapsulates a Vulkan buffer
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanBuffer.h"

namespace vks
{	
	/** 
	* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	* 映射此缓冲区的内存范围。如果成功，mapped 指向指定的缓冲区范围。
	* 
	* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
	* @param size (可选) 要映射的内存范围大小。传递 VK_WHOLE_SIZE 以映射整个缓冲区范围。
	* @param offset (Optional) Byte offset from beginning
	* @param offset (可选) 从开始处的字节偏移量
	* 
	* @return VkResult of the buffer mapping call
	* @return 缓冲区映射调用的 VkResult
	*/
	VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
	{
		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	/**
	* Unmap a mapped memory range
	* 取消映射已映射的内存范围
	*
	* @note Does not return a result as vkUnmapMemory can't fail
	* @note 不返回结果，因为 vkUnmapMemory 不会失败
	*/
	void Buffer::unmap()
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}

	/** 
	* Attach the allocated memory block to the buffer
	* 将分配的内存块附加到缓冲区
	* 
	* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
	* @param offset (可选) 要绑定的内存区域的字节偏移量（从开始处）
	* 
	* @return VkResult of the bindBufferMemory call
	* @return bindBufferMemory 调用的 VkResult
	*/
	VkResult Buffer::bind(VkDeviceSize offset)
	{
		return vkBindBufferMemory(device, buffer, memory, offset);
	}

	/**
	* Setup the default descriptor for this buffer
	* 设置此缓冲区的默认描述符
	*
	* @param size (Optional) Size of the memory range of the descriptor
	* @param size (可选) 描述符的内存范围大小
	* @param offset (Optional) Byte offset from beginning
	* @param offset (可选) 从开始处的字节偏移量
	*
	*/
	void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
	{
		descriptor.offset = offset;
		descriptor.buffer = buffer;
		descriptor.range = size;
	}

	/**
	* Copies the specified data to the mapped buffer
	* 将指定数据复制到已映射的缓冲区
	* 
	* @param data Pointer to the data to copy
	* @param data 要复制的数据指针
	* @param size Size of the data to copy in machine units
	* @param size 要复制的数据大小（机器单位）
	*
	*/
	void Buffer::copyTo(void* data, VkDeviceSize size)
	{
		assert(mapped);
		memcpy(mapped, data, size);
	}

	/** 
	* Flush a memory range of the buffer to make it visible to the device
	* 刷新缓冲区的内存范围，使其对设备可见
	*
	* @note Only required for non-coherent memory
	* @note 仅对非一致性内存需要
	*
	* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	* @param size (可选) 要刷新的内存范围大小。传递 VK_WHOLE_SIZE 以刷新整个缓冲区范围。
	* @param offset (Optional) Byte offset from beginning
	* @param offset (可选) 从开始处的字节偏移量
	*
	* @return VkResult of the flush call
	* @return 刷新调用的 VkResult
	*/
	VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange{
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.memory = memory,
			.offset = offset,
			.size = size
		};
		return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	/**
	* Invalidate a memory range of the buffer to make it visible to the host
	* 使缓冲区的内存范围失效，使其对主机可见
	*
	* @note Only required for non-coherent memory
	* @note 仅对非一致性内存需要
	*
	* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	* @param size (可选) 要使失效的内存范围大小。传递 VK_WHOLE_SIZE 以使整个缓冲区范围失效。
	* @param offset (Optional) Byte offset from beginning
	* @param offset (可选) 从开始处的字节偏移量
	*
	* @return VkResult of the invalidate call
	* @return 失效调用的 VkResult
	*/
	VkResult Buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
	{
		VkMappedMemoryRange mappedRange{
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.memory = memory,
			.offset = offset,
			.size = size
		};
		return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
	}

	/** 
	* Release all Vulkan resources held by this buffer
	* 释放此缓冲区持有的所有 Vulkan 资源
	*/
	void Buffer::destroy()
	{
		if (buffer)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			buffer = VK_NULL_HANDLE;
		}
		if (memory)
		{
			vkFreeMemory(device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
	}
};
