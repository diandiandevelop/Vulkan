/*
* Extended sample base class for ray tracing based samples
*
* Copyright (C) 2020-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanRaytracingSample.h"

/**
 * @brief 设置渲染通道
 * 使用不同的颜色附件加载操作更新默认渲染通道，以保留附件内容
 * 通过此更改，我们可以在光线追踪场景上绘制 UI
 */
void VulkanRaytracingSample::setupRenderPass()
{
	// Update the default render pass with different color attachment load ops to keep attachment contents
	// With this change, we can e.g. draw an UI on top of the ray traced scene
	// 使用不同的颜色附件加载操作更新默认渲染通道，以保留附件内容
	// 通过此更改，我们可以在光线追踪场景上绘制 UI

	vkDestroyRenderPass(device, renderPass, nullptr);

	VkAttachmentLoadOp colorLoadOp{ VK_ATTACHMENT_LOAD_OP_LOAD };  // 颜色附件加载操作：加载（保留内容）
	VkImageLayout colorInitialLayout{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };  // 颜色初始布局：呈现源
	
	// 如果仅使用光线查询，则清除颜色附件
	if (rayQueryOnly) {
		colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	std::array<VkAttachmentDescription, 2> attachments{
		VkAttachmentDescription{
			.format = swapChain.colorFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = colorLoadOp,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = colorInitialLayout,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		VkAttachmentDescription{
			.format = depthFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	VkAttachmentReference colorReference{ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference{ .attachment = 1,.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorReference,
		.pDepthStencilAttachment = &depthReference,
	};

	std::array<VkSubpassDependency, 2> dependencies{
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_NONE_KHR,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		},
		VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		}
	};

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};
	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

/**
 * @brief 设置帧缓冲区
 * 为每个交换链图像创建帧缓冲区
 */
void VulkanRaytracingSample::setupFrameBuffer()
{

	frameBuffers.resize(swapChain.images.size());
	for (uint32_t i = 0; i < frameBuffers.size(); i++) {
		VkImageView attachments[2] { swapChain.imageViews[i], depthStencil.view };  // 附件：颜色图像视图和深度模板视图
		VkFramebufferCreateInfo frameBufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.width = width,
			.height = height,
			.layers = 1
		};
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

/**
 * @brief 启用光线追踪相关的扩展
 * 设置所需的 Vulkan 版本和扩展
 */
void VulkanRaytracingSample::enableExtensions()
{
	// Require Vulkan 1.1
	// 需要 Vulkan 1.1
	apiVersion = VK_API_VERSION_1_1;

	// Ray tracing related extensions required by this sample
	// 此示例所需的光线追踪相关扩展
	enabledDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);  // 加速结构扩展
	if (!rayQueryOnly) {
		enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);  // 光线追踪管线扩展
	}

	// Required by VK_KHR_acceleration_structure
	// VK_KHR_acceleration_structure 所需
	enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);        // 缓冲区设备地址扩展
	enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);     // 延迟主机操作扩展
	enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);         // 描述符索引扩展

	// Required for VK_KHR_ray_tracing_pipeline
	// VK_KHR_ray_tracing_pipeline 所需
	enabledDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);  // SPIR-V 1.4 扩展

	// Required by VK_KHR_spirv_1_4
	// VK_KHR_spirv_1_4 所需
	enabledDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);  // 着色器浮点控制扩展

	// Format for the storage image is decided at runtime, so we can't explicilily state it in the shader
	// 存储图像的格式在运行时决定，因此我们无法在着色器中明确声明
	enabledFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;  // 启用无格式存储图像写入
	enabledFeatures.shaderStorageImageReadWithoutFormat = VK_TRUE;   // 启用无格式存储图像读取
}

/**
 * @brief 创建暂存缓冲区
 * @param size 缓冲区大小（字节）
 * @return 创建的暂存缓冲区
 */
VulkanRaytracingSample::ScratchBuffer VulkanRaytracingSample::createScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratchBuffer{};
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = size,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // 存储缓冲区和着色器设备地址
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &scratchBuffer.handle));
	VkMemoryRequirements memoryRequirements{};  // 内存需求
	vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, scratchBuffer.handle, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR  // 分配设备地址标志
	};
	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &memoryAllocateFlagsInfo,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)  // 设备本地内存
	};
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &scratchBuffer.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, scratchBuffer.handle, scratchBuffer.memory, 0));
	VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.handle
	};
	scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAddresInfo);  // 获取设备地址
	return scratchBuffer;
}

/**
 * @brief 删除暂存缓冲区
 * @param scratchBuffer 要删除的暂存缓冲区引用
 */
void VulkanRaytracingSample::deleteScratchBuffer(ScratchBuffer& scratchBuffer)
{
	if (scratchBuffer.memory != VK_NULL_HANDLE) {
		vkFreeMemory(vulkanDevice->logicalDevice, scratchBuffer.memory, nullptr);
	}
	if (scratchBuffer.handle != VK_NULL_HANDLE) {
		vkDestroyBuffer(vulkanDevice->logicalDevice, scratchBuffer.handle, nullptr);
	}
}

/**
 * @brief 创建加速结构
 * @param accelerationStructure 要填充的加速结构引用
 * @param type 加速结构类型（顶层或底层）
 * @param buildSizeInfo 构建大小信息
 */
void VulkanRaytracingSample::createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buildSizeInfo.accelerationStructureSize,  // 加速结构大小
		.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // 加速结构存储和着色器设备地址
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &accelerationStructure.buffer));
	VkMemoryRequirements memoryRequirements{};  // 内存需求
	vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, accelerationStructure.buffer, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR  // 分配设备地址标志
	};
	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &memoryAllocateFlagsInfo,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)  // 设备本地内存
	};
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &accelerationStructure.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, accelerationStructure.buffer, accelerationStructure.memory, 0));
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = accelerationStructure.buffer,
		.size = buildSizeInfo.accelerationStructureSize,
		.type = type
	};
	vkCreateAccelerationStructureKHR(vulkanDevice->logicalDevice, &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = accelerationStructure.handle
	};
	accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vulkanDevice->logicalDevice, &accelerationDeviceAddressInfo);  // 获取设备地址
}

/**
 * @brief 删除加速结构
 * @param accelerationStructure 要删除的加速结构引用
 */
void VulkanRaytracingSample::deleteAccelerationStructure(AccelerationStructure& accelerationStructure)
{
	vkFreeMemory(device, accelerationStructure.memory, nullptr);
	vkDestroyBuffer(device, accelerationStructure.buffer, nullptr);
	vkDestroyAccelerationStructureKHR(device, accelerationStructure.handle, nullptr);
}

/**
 * @brief 获取缓冲区设备地址
 * @param buffer 缓冲区句柄
 * @return 设备地址
 */
uint64_t VulkanRaytracingSample::getBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = buffer
	};
	return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAI);
}

/**
 * @brief 创建存储图像（光线追踪着色器的输出目标）
 * @param format 图像格式
 * @param extent 图像范围（宽度、高度、深度）
 */
void VulkanRaytracingSample::createStorageImage(VkFormat format, VkExtent3D extent)
{
	// Release ressources if image is to be recreated
	// 如果要重新创建图像，释放资源
	if (storageImage.image != VK_NULL_HANDLE) {
		vkDestroyImageView(device, storageImage.view, nullptr);
		vkDestroyImage(device, storageImage.image, nullptr);
		vkFreeMemory(device, storageImage.memory, nullptr);
		storageImage = {};
	}

	VkImageCreateInfo image{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,  // 2D 图像
		.format = format,
		.extent = extent,
		.mipLevels = 1,                 // 单个 Mip 级别
		.arrayLayers = 1,               // 单个数组层
		.samples = VK_SAMPLE_COUNT_1_BIT,  // 单采样
		.tiling = VK_IMAGE_TILING_OPTIMAL,  // 最优平铺
		.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,  // 传输源和存储
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &image, nullptr, &storageImage.image));

	VkMemoryRequirements memReqs;  // 内存需求
	vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, storageImage.image, &memReqs);
	VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memReqs.size;
	memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 设备本地内存
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &storageImage.memory));
	VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, storageImage.image, storageImage.memory, 0));

	VkImageViewCreateInfo colorImageView{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = storageImage.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,  // 2D 视图
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,  // 颜色方面
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};
	VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &colorImageView, nullptr, &storageImage.view));

	// 将图像布局从 UNDEFINED 转换为 GENERAL（存储图像布局）
	VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vks::tools::setImageLayout(cmdBuffer, storageImage.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,  // 通用布局（用于存储图像）
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	vulkanDevice->flushCommandBuffer(cmdBuffer, queue);
}

/**
 * @brief 删除存储图像
 */
void VulkanRaytracingSample::deleteStorageImage()
{
	vkDestroyImageView(vulkanDevice->logicalDevice, storageImage.view, nullptr);
	vkDestroyImage(vulkanDevice->logicalDevice, storageImage.image, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, storageImage.memory, nullptr);
}

/**
 * @brief 准备光线追踪示例
 * 获取光线追踪相关的属性和功能，加载函数指针
 */
void VulkanRaytracingSample::prepare()
{
	VulkanExampleBase::prepare();
	// Get properties and features
	// 获取属性和功能
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &rayTracingPipelineProperties
	};
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);  // 获取光线追踪管线属性
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 deviceFeatures2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &accelerationStructureFeatures
	};
	vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);  // 获取加速结构功能
	// Get the function pointers required for ray tracing
	// 获取光线追踪所需的函数指针
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
}

/**
 * @brief 获取 SBT 条目的跨步设备地址区域
 * @param buffer 缓冲区句柄
 * @param handleCount 句柄数量
 * @return 跨步设备地址区域
 */
VkStridedDeviceAddressRegionKHR VulkanRaytracingSample::getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount)
{
	// 计算对齐后的句柄大小
	const uint32_t handleSizeAligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{
		.deviceAddress = getBufferDeviceAddress(buffer),  // 设备地址
		.stride = handleSizeAligned  // 跨步（每个条目的字节数）
	};
	stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;  // 总大小
	return stridedDeviceAddressRegionKHR;
}

/**
 * @brief 创建着色器绑定表（SBT）
 * @param shaderBindingTable 要填充的着色器绑定表引用
 * @param handleCount 句柄数量
 */
void VulkanRaytracingSample::createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
{
	// Create buffer to hold all shader handles for the SBT
	// 创建缓冲区以保存 SBT 的所有着色器句柄
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // 着色器绑定表和着色器设备地址
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  // 主机可见和一致
		&shaderBindingTable, 
		rayTracingPipelineProperties.shaderGroupHandleSize * handleCount));  // 总大小 = 句柄大小 × 句柄数量
	// Get the strided address to be used when dispatching the rays
	// 获取在调度光线时使用的跨步地址
	shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStridedDeviceAddressRegion(shaderBindingTable.buffer, handleCount);
	// Map persistent 
	// 映射持久化（主机可访问）
	shaderBindingTable.map();
}

/**
 * @brief 绘制 ImGui UI 叠加层
 * @param commandBuffer 命令缓冲区句柄
 * @param framebuffer 帧缓冲区句柄
 */
void VulkanRaytracingSample::drawUI(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer)
{
	VkClearValue clearValues[2]{};  // 清除值：颜色和深度模板
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffer,
		.renderArea = {.offset = {.x = 0, .y = 0, }, .extent = {.width = width, .height = height } },  // 渲染区域
		.clearValueCount = 2,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VulkanExampleBase::drawUI(commandBuffer);  // 调用基类的 UI 绘制
	vkCmdEndRenderPass(commandBuffer);
}
