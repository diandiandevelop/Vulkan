/*
 * Vulkan device class
 * 
 * Encapsulates a physical Vulkan device and its logical representation
 *
 * Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
// SRS - Enable beta extensions and make VK_KHR_portability_subset visible
#define VK_ENABLE_BETA_EXTENSIONS
#endif
#include <VulkanDevice.h>
#include <unordered_set>

namespace vks
{	
	/**
	* Default constructor
	* 默认构造函数
	*
	* @param physicalDevice Physical device that is to be used
	* @param physicalDevice 要使用的物理设备
	*/
	VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice)
	{
		assert(physicalDevice);
		this->physicalDevice = physicalDevice;

		// Store Properties features, limits and properties of the physical device for later use
		// Device properties also contain limits and sparse properties
		// 存储物理设备的属性、功能和限制以供后续使用
		// 设备属性还包含限制和稀疏属性
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		// Features should be checked by the examples before using them
		// 功能应在使用前由示例检查
		vkGetPhysicalDeviceFeatures(physicalDevice, &features);
		// Memory properties are used regularly for creating all kinds of buffers
		// 内存属性经常用于创建各种缓冲区
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
		// Queue family properties, used for setting up requested queues upon device creation
		// 队列族属性，用于在创建设备时设置请求的队列
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		assert(queueFamilyCount > 0);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		// Get list of supported extensions
		// 获取支持的扩展列表
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
		if (extCount > 0)
		{
			std::vector<VkExtensionProperties> extensions(extCount);
			if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			{
				for (auto& ext : extensions)
				{
					supportedExtensions.push_back(ext.extensionName);
				}
			}
		}
	}

	/** 
	* Default destructor
	* 默认析构函数
	*
	* @note Frees the logical device
	* @note 释放逻辑设备
	*/
	VulkanDevice::~VulkanDevice()
	{
		if (commandPool)
		{
			vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		}
		if (logicalDevice)
		{
			vkDestroyDevice(logicalDevice, nullptr);
		}
	}

	/**
	* Get the index of a memory type that has all the requested property bits set
	* 获取具有所有请求属性位的内存类型索引
	*
	* @param typeBits Bit mask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
	* @param typeBits 资源支持的内存类型的位掩码（来自 VkMemoryRequirements）
	* @param properties Bit mask of properties for the memory type to request
	* @param properties 请求的内存类型的属性位掩码
	* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
	* @param memTypeFound (可选) 指向布尔值的指针，如果找到匹配的内存类型则设置为 true
	* 
	* @return Index of the requested memory type
	* @return 请求的内存类型索引
	*
	* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
	* @throw 如果 memTypeFound 为 null 且找不到支持请求属性的内存类型，则抛出异常
	*/
	uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const
	{
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)  // 遍历所有内存类型
		{
			if ((typeBits & 1) == 1)  // 检查当前内存类型是否在 typeBits 中（检查最低位）
			{
				if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)  // 检查内存类型的属性是否匹配请求的属性
				{
					if (memTypeFound)  // 如果提供了 memTypeFound 指针
					{
						*memTypeFound = true;  // 设置找到标志为 true
					}
					return i;  // 返回匹配的内存类型索引
				}
			}
			typeBits >>= 1;  // 右移一位，检查下一个内存类型
		}

		if (memTypeFound)  // 如果提供了 memTypeFound 指针
		{
			*memTypeFound = false;  // 设置找到标志为 false
			return 0;  // 返回 0（无效索引）
		}
		else  // 如果没有提供 memTypeFound 指针
		{
			throw std::runtime_error("Could not find a matching memory type");  // 抛出异常
		}
	}

	/**
	* Get the index of a queue family that supports the requested queue flags
	* SRS - support VkQueueFlags parameter for requesting multiple flags vs. VkQueueFlagBits for a single flag only
	* 获取支持请求队列标志的队列族索引
	* SRS - 支持 VkQueueFlags 参数以请求多个标志，而 VkQueueFlagBits 仅用于单个标志
	*
	* @param queueFlags Queue flags to find a queue family index for
	* @param queueFlags 要查找队列族索引的队列标志
	*
	* @return Index of the queue family index that matches the flags
	* @return 匹配标志的队列族索引
	*
	* @throw Throws an exception if no queue family index could be found that supports the requested flags
	* @throw 如果找不到支持请求标志的队列族索引，则抛出异常
	*/
	uint32_t VulkanDevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const
	{
		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		// 专用计算队列
		// 尝试找到支持计算但不支持图形的队列族索引
		if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					return i;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		// 专用传输队列
		// 尝试找到支持传输但不支持图形和计算的队列族索引
		if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					return i;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
		{
			if ((queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
			{
				return i;
			}
		}

		throw std::runtime_error("Could not find a matching queue family index");
	}

	/**
	* Create the logical device based on the assigned physical device, also gets default queue family indices
	*
	* @param enabledFeatures Can be used to enable certain features upon device creation
	* @param pNextChain Optional chain of pointer to extension structures
	* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
	* @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device  
	*
	* @return VkResult of the device creation call
	*/
	/**
	* 基于已选定的物理设备创建逻辑设备，同时获取默认的队列族索引。
	*
	* @param  enabledFeatures  可用于在设备创建时启用特定的功能特性。
	* @param  pNextChain       可选的扩展结构体指针链（扩展信息传递链）。
	* @param  useSwapChain     若为无窗口渲染（离屏渲染），请设为 false，以忽略交换链相关的设备扩展。
	* @param  requestedQueueTypes  位掩码标志，用于指定需要从设备中请求获取的队列类型。
	*
	* @return  设备创建调用对应的 VkResult 状态码。
	*/
	VkResult VulkanDevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain, VkQueueFlags requestedQueueTypes)
	{			
		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types
		// 所需的队列需要在逻辑设备创建时请求
		// 由于 Vulkan 实现的队列族配置不同，这可能有点棘手，特别是如果应用程序请求不同的队列类型

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};  // 队列创建信息列表

		// Get queue family indices for the requested queue family types
		// Note that the indices may overlap depending on the implementation
		// 获取请求的队列族类型的队列族索引
		// 注意：根据实现，索引可能会重叠

		const float defaultQueuePriority(0.0f);  // 默认队列优先级       // 队列优先级（0.0~1.0，高优先级队列先执行命令）

		// Graphics queue
		// 图形队列
		if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)  // 如果请求图形队列  过requestedQueueTypes位掩码判断是否需要启用对应队列，且相同队列族仅配置一次（避免重复创建）
		{
			queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);  // 获取图形队列族索引
			VkDeviceQueueCreateInfo queueInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // 结构体类型
				.queueFamilyIndex = queueFamilyIndices.graphics,        // 队列族索引
				.queueCount = 1,                                       // 队列数量
				.pQueuePriorities = &defaultQueuePriority              // 队列优先级数组
			};
			queueCreateInfos.push_back(queueInfo);  // 添加到队列创建信息列表
		}
		else  // 如果未请求图形队列
		{
			queueFamilyIndices.graphics = 0;  // 设置为 0（无效索引）
		}

		// Dedicated compute queue
		// 专用计算队列
		if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)  // 如果请求计算队列
		{
			queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);  // 获取计算队列族索引
			if (queueFamilyIndices.compute != queueFamilyIndices.graphics)  // 如果计算队列族索引与图形队列族索引不同
			{
				// If compute family index differs, we need an additional queue create info for the compute queue
				// 如果计算族索引不同，我们需要为计算队列添加额外的队列创建信息
				VkDeviceQueueCreateInfo queueInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // 结构体类型
					.queueFamilyIndex = queueFamilyIndices.compute,        // 队列族索引
					.queueCount = 1,                                       // 队列数量
					.pQueuePriorities = &defaultQueuePriority,             // 队列优先级数组
				};
				queueCreateInfos.push_back(queueInfo);  // 添加到队列创建信息列表
			}
		}
		else  // 如果未请求计算队列
		{
			// Else we use the same queue
			// 否则我们使用相同的队列
			queueFamilyIndices.compute = queueFamilyIndices.graphics;  // 使用图形队列族索引
		}

		// Dedicated transfer queue
		// 专用传输队列
		if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)  // 如果请求传输队列
		{
			queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);  // 获取传输队列族索引
			if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute))  // 如果传输队列族索引与图形和计算队列族索引都不同
			{
				// If transfer family index differs, we need an additional queue create info for the transfer queue
				// 如果传输族索引不同，我们需要为传输队列添加额外的队列创建信息
				VkDeviceQueueCreateInfo queueInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // 结构体类型
					.queueFamilyIndex = queueFamilyIndices.transfer,        // 队列族索引
					.queueCount = 1,                                       // 队列数量
					.pQueuePriorities = &defaultQueuePriority              // 队列优先级数组
				};
				queueCreateInfos.push_back(queueInfo);  // 添加到队列创建信息列表
			}
		}
		else  // 如果未请求传输队列
		{
			// Else we use the same queue
			// 否则我们使用相同的队列
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;  // 使用图形队列族索引
		}

		// Create the logical device representation
		// 创建逻辑设备表示
		std::vector<const char*> deviceExtensions(enabledExtensions);  // 设备扩展列表（复制传入的扩展）
		if (useSwapChain)  // 如果使用交换链
		{
			// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			// 如果设备将通过交换链用于显示，我们需要请求交换链扩展
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);  // 添加交换链扩展名称
		}

		VkDeviceCreateInfo deviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,              // 结构体类型
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),  // 队列创建信息数量
			.pQueueCreateInfos = queueCreateInfos.data(),               // 队列创建信息数组
			.pEnabledFeatures = &enabledFeatures                         // 启用的功能
		};
		
		// If a pNext(Chain) has been passed, we need to add it to the device creation info
		// 如果传递了 pNext（链），我们需要将其添加到设备创建信息中
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};  // 物理设备功能 2 结构
		if (pNextChain) {  // 如果提供了 pNext 链
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;  // 结构体类型
			physicalDeviceFeatures2.features = enabledFeatures;  // 功能
			physicalDeviceFeatures2.pNext = pNextChain;  // 链接到扩展结构链
			deviceCreateInfo.pEnabledFeatures = nullptr;  // 清空旧的功能指针（使用 pNext 链）
			deviceCreateInfo.pNext = &physicalDeviceFeatures2;  // 设置 pNext 链
		}

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_KHR_portability_subset)
		// SRS - When running on iOS/macOS with MoltenVK and VK_KHR_portability_subset is defined and supported by the device, enable the extension
		// SRS - 在 iOS/macOS 上使用 MoltenVK 运行时，如果定义了 VK_KHR_portability_subset 且设备支持，则启用该扩展
		if (extensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))  // 如果支持可移植性子集扩展
		{
			deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);  // 添加可移植性子集扩展名称
		}
#endif

		if (deviceExtensions.size() > 0)  // 如果有设备扩展
		{
			for (const char* enabledExtension : deviceExtensions)  // 遍历所有启用的扩展
			{
				if (!extensionSupported(enabledExtension)) {  // 如果扩展不支持
					std::cerr << "Enabled device extension \"" << enabledExtension << "\" is not present at device level\n";  // 输出警告信息
				}
			}

			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();  // 设置启用的扩展数量
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();  // 设置启用的扩展名称数组
		}

		this->enabledFeatures = enabledFeatures;  // 保存启用的功能

		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);  // 创建逻辑设备
		if (result != VK_SUCCESS)  // 如果创建失败
		{
			return result;  // 返回错误结果
		}

		// Create a default command pool for graphics command buffers
		// 为图形命令缓冲区创建默认命令池
		commandPool = createCommandPool(queueFamilyIndices.graphics);  // 创建命令池

		return result;  // 返回创建结果
	}

	/**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param size Size of the buffer in byes
	* @param buffer Pointer to the buffer handle acquired by the function
	* @param memory Pointer to the memory handle acquired by the function
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/
	/**
	 * @brief 在设备上创建缓冲区（返回句柄版本）
	 * 创建 Vulkan 缓冲区对象并分配设备内存，可选择性地将数据复制到缓冲区
	 * 
	 * @param usageFlags 缓冲区使用标志位掩码（如索引缓冲区、顶点缓冲区、统一缓冲区等）
	 * @param memoryPropertyFlags 缓冲区内存属性标志（如设备本地、主机可见、一致性等）
	 * @param size 缓冲区大小（字节）
	 * @param buffer 输出的缓冲区句柄指针
	 * @param memory 输出的内存句柄指针
	 * @param data 要复制到缓冲区的数据指针（可选，如果未设置则不复制数据）
	 * 
	 * @return 如果缓冲区句柄和内存已创建且（可选传递的）数据已复制，返回 VK_SUCCESS
	 */
	VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data)
	{
		// 创建缓冲区句柄
		// 缓冲区对象定义了缓冲区的用途和大小
		VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // 独占模式（单队列族使用）
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

		// 创建支持缓冲区句柄的内存
		// 首先查询缓冲区的内存需求（对齐、内存类型位等）
		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小（已对齐）
		// 查找符合缓冲区属性的内存类型索引
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
		// 如果缓冲区设置了 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT，还需要在分配时启用相应标志
		// 这用于缓冲区设备地址扩展，允许着色器直接访问缓冲区
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;  // 启用设备地址分配标志
			memAlloc.pNext = &allocFlagsInfo;
		}
		// 分配设备内存
		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));
			
		// 如果传递了缓冲区数据指针，映射缓冲区并复制数据
		if (data != nullptr)
		{
			void *mapped;  // 映射后的内存指针
			// 映射内存到主机可访问的地址空间
			VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
			// 将数据复制到映射的内存
			memcpy(mapped, data, size);
			// 如果未请求主机一致性，手动刷新以使写入对设备可见
			// 非一致性内存需要显式刷新以确保 GPU 能看到主机的写入
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange mappedRange{
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.memory = *memory,
					.size = size
				};
				// 刷新映射的内存范围
				vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
			}
			// 取消映射内存
			vkUnmapMemory(logicalDevice, *memory);
		}

		// 将内存附加到缓冲区对象
		// 这会将分配的内存绑定到缓冲区，使缓冲区可以使用该内存
		VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

		return VK_SUCCESS;
	}

	/**
	* Create a buffer on the device
	*
	* @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex, uniform buffer)
	* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
	* @param buffer Pointer to a vk::Vulkan buffer object
	* @param size Size of the buffer in bytes
	* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
	*
	* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
	*/
	/**
	 * @brief 在设备上创建缓冲区（Buffer 结构体版本）
	 * 创建 Vulkan 缓冲区对象并填充 Buffer 结构体，可选择性地将数据复制到缓冲区
	 * 
	 * @param usageFlags 缓冲区使用标志位掩码（如索引缓冲区、顶点缓冲区、统一缓冲区等）
	 * @param memoryPropertyFlags 缓冲区内存属性标志（如设备本地、主机可见、一致性等）
	 * @param buffer 输出的 Buffer 结构体指针
	 * @param size 缓冲区大小（字节）
	 * @param data 要复制到缓冲区的数据指针（可选，如果未设置则不复制数据）
	 * 
	 * @return 如果缓冲区句柄和内存已创建且（可选传递的）数据已复制，返回 VK_SUCCESS
	 */
	VkResult VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, vks::Buffer *buffer, VkDeviceSize size, void *data)
	{
		// 设置设备句柄
		buffer->device = logicalDevice;

		// 创建缓冲区句柄
		// 缓冲区对象定义了缓冲区的用途和大小
		VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
		VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer));

		// 创建支持缓冲区句柄的内存
		// 首先查询缓冲区的内存需求（对齐、内存类型位等）
		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小（已对齐）
		// 查找符合缓冲区属性的内存类型索引
		memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
		// 如果缓冲区设置了 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT，还需要在分配时启用相应标志
		// 这用于缓冲区设备地址扩展，允许着色器直接访问缓冲区
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;  // 启用设备地址分配标志
			memAlloc.pNext = &allocFlagsInfo;
		}
		// 分配设备内存
		VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory));

		// 保存缓冲区的元数据
		buffer->alignment = memReqs.alignment;              // 内存对齐要求
		buffer->size = size;                                 // 缓冲区大小
		buffer->usageFlags = usageFlags;                     // 使用标志
		buffer->memoryPropertyFlags = memoryPropertyFlags;   // 内存属性标志

		// 如果传递了缓冲区数据指针，映射缓冲区并复制数据
		if (data != nullptr)
		{
			// 映射内存到主机可访问的地址空间
			VK_CHECK_RESULT(buffer->map());
			// 将数据复制到映射的内存
			memcpy(buffer->mapped, data, size);
			// 如果未请求主机一致性，手动刷新以使写入对设备可见
			if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				buffer->flush();

			// 取消映射内存
			buffer->unmap();
		}

		// 初始化覆盖整个缓冲区大小的默认描述符
		// 这用于在描述符集中引用此缓冲区
		buffer->setupDescriptor();

		// 将内存附加到缓冲区对象
		// 这会将分配的内存绑定到缓冲区，使缓冲区可以使用该内存
		return buffer->bind();
	}

	/**
	* Copy buffer data from src to dst using VkCmdCopyBuffer
	* 
	* @param src Pointer to the source buffer to copy from
	* @param dst Pointer to the destination buffer to copy to
	* @param queue Pointer
	* @param copyRegion (Optional) Pointer to a copy region, if NULL, the whole buffer is copied
	*
	* @note Source and destination pointers must have the appropriate transfer usage flags set (TRANSFER_SRC / TRANSFER_DST)
	*/
	/**
	 * @brief 复制缓冲区数据
	 * 使用命令缓冲区将数据从源缓冲区复制到目标缓冲区
	 * 
	 * @param src 源缓冲区指针（要从中复制的缓冲区）
	 * @param dst 目标缓冲区指针（要复制到的缓冲区）
	 * @param queue 用于执行复制命令的队列（必须支持传输操作）
	 * @param copyRegion 复制区域指针（可选，如果为 NULL 则复制整个缓冲区）
	 * 
	 * @note 源缓冲区和目标缓冲区必须具有相应的传输使用标志（TRANSFER_SRC / TRANSFER_DST）
	 */
	void VulkanDevice::copyBuffer(vks::Buffer *src, vks::Buffer *dst, VkQueue queue, VkBufferCopy *copyRegion)
	{
		// 确保目标缓冲区大小足够
		assert(dst->size >= src->size);
		// 确保源缓冲区有效
		assert(src->buffer);
		// 创建主命令缓冲区并立即开始记录
		VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy bufferCopy{};  // 缓冲区复制区域
		// 如果未指定复制区域，复制整个源缓冲区
		if (copyRegion == nullptr)
		{
			bufferCopy.size = src->size;  // 复制整个缓冲区
		}
		else
		{
			bufferCopy = *copyRegion;  // 使用指定的复制区域
		}

		// 记录缓冲区复制命令
		// 这将在 GPU 上执行复制操作，比 CPU 复制更高效
		vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

		// 刷新命令缓冲区（结束记录、提交到队列并等待完成）
		flushCommandBuffer(copyCmd, queue);
	}

	/** 
	* Create a command pool for allocation command buffers from
	* 创建用于分配命令缓冲区的命令池
	* 
	* @param queueFamilyIndex Family index of the queue to create the command pool for
	* @param queueFamilyIndex 要为其创建命令池的队列族索引
	* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	* @param createFlags (可选) 命令池创建标志（默认为 VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT）
	*
	* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
	* @note 从创建的池中分配的命令缓冲区只能提交到具有相同族索引的队列
	*
	* @return A handle to the created command buffer
	* @return 创建的命令缓冲区句柄
	*/
	VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
	{
		VkCommandPoolCreateInfo cmdPoolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,  // 结构体类型
			.flags = createFlags,                                  // 创建标志
			.queueFamilyIndex = queueFamilyIndex                    // 队列族索引
		};
		VkCommandPool cmdPool;
		VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));  // 创建命令池
		return cmdPool;  // 返回命令池句柄
	}

	/**
	* Allocate a command buffer from the command pool
	* 从命令池分配命令缓冲区
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param level 新命令缓冲区的级别（主级或次级）
	* @param pool Command pool from which the command buffer will be allocated
	* @param pool 将从中分配命令缓冲区的命令池
	* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	* @param begin (可选) 如果为 true，将在新命令缓冲区上开始记录（vkBeginCommandBuffer）（默认为 false）
	*
	* @return A handle to the allocated command buffer
	* @return 分配的命令缓冲区句柄
	*/
	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(pool, level, 1);  // 命令缓冲区分配信息
		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));  // 分配命令缓冲区
		// If requested, also start recording for the new command buffer
		// 如果请求，同时开始在新命令缓冲区上记录
		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();  // 命令缓冲区开始信息
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));  // 开始命令缓冲区记录
		}
		return cmdBuffer;  // 返回命令缓冲区句柄
	}
			
	/**
	* Allocate a command buffer from the default command pool
	* 从默认命令池分配命令缓冲区
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param level 新命令缓冲区的级别（主级或次级）
	* @param begin If true, recording on the new command buffer will be started (Defaults to false)
	* @param begin 如果为 true，将在新命令缓冲区上开始记录（默认为 false）
	*
	* @return A handle to the allocated command buffer
	* @return 分配的命令缓冲区句柄
	*/
	VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
	{
		return createCommandBuffer(level, commandPool, begin);  // 使用默认命令池创建命令缓冲区
	}

	/**
	* Finish command buffer recording and submit it to a queue
	* 完成命令缓冲区记录并将其提交到队列
	*
	* @param commandBuffer Command buffer to flush
	* @param commandBuffer 要刷新的命令缓冲区
	* @param queue Queue to submit the command buffer to
	* @param queue 要将命令缓冲区提交到的队列
	* @param pool Command pool on which the command buffer has been created
	* @param pool 创建命令缓冲区的命令池
	* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
	* @param free (可选) 提交后释放命令缓冲区（默认为 true）
	*
	* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
	* @note 命令缓冲区提交到的队列必须与分配它的池具有相同的族索引
	* @note Uses a fence to ensure command buffer has finished executing
	* @note 使用栅栏确保命令缓冲区已完成执行
	*/
	void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
	{
		if (commandBuffer == VK_NULL_HANDLE)  // 检查命令缓冲区是否有效
		{
			return;
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));  // 结束命令缓冲区记录

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,  // 结构体类型
			.commandBufferCount = 1,                  // 命令缓冲区数量
			.pCommandBuffers = &commandBuffer         // 命令缓冲区数组
		};
		// Create fence to ensure that the command buffer has finished executing
		// 创建栅栏以确保命令缓冲区已完成执行
		VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);  // 栅栏创建信息
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));  // 创建栅栏
		// Submit to the queue
		// 提交到队列
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));  // 提交命令缓冲区到队列
		// Wait for the fence to signal that command buffer has finished executing
		// 等待栅栏发出信号，表示命令缓冲区已完成执行
		VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));  // 等待栅栏
		vkDestroyFence(logicalDevice, fence, nullptr);  // 销毁栅栏
		if (free)
		{
			vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);  // 释放命令缓冲区
		}
	}

	/**
	* Finish command buffer recording and submit it to a queue (using default command pool)
	* 完成命令缓冲区记录并将其提交到队列（使用默认命令池）
	*
	* @param commandBuffer Command buffer to flush
	* @param commandBuffer 要刷新的命令缓冲区
	* @param queue Queue to submit the command buffer to
	* @param queue 要将命令缓冲区提交到的队列
	* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
	* @param free (可选) 提交后释放命令缓冲区（默认为 true）
	*/
	void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
	{
		return flushCommandBuffer(commandBuffer, queue, commandPool, free);  // 使用默认命令池刷新命令缓冲区
	}

	/**
	* Check if an extension is supported by the (physical device)
	* 检查（物理设备）是否支持某个扩展
	*
	* @param extension Name of the extension to check
	* @param extension 要检查的扩展名称
	*
	* @return True if the extension is supported (present in the list read at device creation time)
	* @return 如果扩展受支持（在设备创建时读取的列表中存在），返回 true
	*/
	bool VulkanDevice::extensionSupported(std::string extension)
	{
		return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());  // 在支持的扩展列表中查找
	}

	/**
	* Select the best-fit depth format for this device from a list of possible depth (and stencil) formats
	* 从可能的深度（和模板）格式列表中为当前设备选择最适合的深度格式
	*
	* @param checkSamplingSupport Check if the format can be sampled from (e.g. for shader reads)
	* @param checkSamplingSupport 检查格式是否可以被采样（例如用于着色器读取）
	*
	* @return The depth format that best fits for the current device
	* @return 最适合当前设备的深度格式
	*
	* @throw Throws an exception if no depth format fits the requirements
	* @throw 如果没有深度格式符合要求，则抛出异常
	*/
	VkFormat VulkanDevice::getSupportedDepthFormat(bool checkSamplingSupport)
	{
		// All depth formats may be optional, so we need to find a suitable depth format to use
		// 所有深度格式都可能是可选的，因此我们需要找到合适的深度格式来使用
		std::vector<VkFormat> depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };  // 深度格式列表（按优先级排序）
		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);  // 查询格式属性
			// Format must support depth stencil attachment for optimal tiling
			// 格式必须支持深度模板附件以用于最佳平铺
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (checkSamplingSupport) {  // 如果需要检查采样支持
					if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {  // 如果不支持采样
						continue;  // 跳过此格式
					}
				}
				return format;  // 返回找到的格式
			}
		}
		throw std::runtime_error("Could not find a matching depth format");  // 未找到匹配的深度格式，抛出异常
	}

};
