/*
* Initializers for Vulkan structures and objects used by the examples
* Saves lot of VK_STRUCTURE_TYPE assignments
* Some initializers are parameterized for convenience
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>
#include "vulkan/vulkan.h"

namespace vks
{
	namespace initializers
	{

		/**
		 * @brief 初始化内存分配信息结构
		 * @return 初始化的 VkMemoryAllocateInfo 结构
		 */
		inline VkMemoryAllocateInfo memoryAllocateInfo()
		{
			VkMemoryAllocateInfo memAllocInfo {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		/**
		 * @brief 初始化映射内存范围结构
		 * @return 初始化的 VkMappedMemoryRange 结构
		 */
		inline VkMappedMemoryRange mappedMemoryRange()
		{
			VkMappedMemoryRange mappedMemoryRange {};
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			return mappedMemoryRange;
		}

		/**
		 * @brief 初始化命令缓冲区分配信息结构
		 * @param commandPool 命令池句柄
		 * @param level 命令缓冲区级别
		 * @param bufferCount 缓冲区数量
		 * @return 初始化的 VkCommandBufferAllocateInfo 结构
		 */
		inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
			VkCommandPool commandPool, 
			VkCommandBufferLevel level, 
			uint32_t bufferCount)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.level = level;
			commandBufferAllocateInfo.commandBufferCount = bufferCount;
			return commandBufferAllocateInfo;
		}

		/**
		 * @brief 初始化命令池创建信息结构
		 * @return 初始化的 VkCommandPoolCreateInfo 结构
		 */
		inline VkCommandPoolCreateInfo commandPoolCreateInfo()
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo {};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;  // 结构体类型
			return cmdPoolCreateInfo;
		}

		/**
		 * @brief 初始化命令缓冲区开始信息结构
		 * @return 初始化的 VkCommandBufferBeginInfo 结构
		 */
		inline VkCommandBufferBeginInfo commandBufferBeginInfo()
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo {};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;  // 结构体类型
			return cmdBufferBeginInfo;
		}

		/**
		 * @brief 初始化命令缓冲区继承信息结构
		 * @return 初始化的 VkCommandBufferInheritanceInfo 结构
		 */
		inline VkCommandBufferInheritanceInfo commandBufferInheritanceInfo()
		{
			VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo {};
			cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;  // 结构体类型
			return cmdBufferInheritanceInfo;
		}

		/**
		 * @brief 初始化渲染通道开始信息结构
		 * @return 初始化的 VkRenderPassBeginInfo 结构
		 */
		inline VkRenderPassBeginInfo renderPassBeginInfo()
		{
			VkRenderPassBeginInfo renderPassBeginInfo {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;  // 结构体类型
			return renderPassBeginInfo;
		}

		/**
		 * @brief 初始化渲染通道创建信息结构
		 * @return 初始化的 VkRenderPassCreateInfo 结构
		 */
		inline VkRenderPassCreateInfo renderPassCreateInfo()
		{
			VkRenderPassCreateInfo renderPassCreateInfo {};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;  // 结构体类型
			return renderPassCreateInfo;
		}

		/** @brief Initialize an image memory barrier with no image transfer ownership */
		/** @brief 初始化图像内存屏障（无图像传输所有权） */
		/**
		 * @brief 初始化图像内存屏障
		 * @return 初始化的 VkImageMemoryBarrier 结构
		 */
		inline VkImageMemoryBarrier imageMemoryBarrier()
		{
			VkImageMemoryBarrier imageMemoryBarrier {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return imageMemoryBarrier;
		}

		/** @brief Initialize a buffer memory barrier with no image transfer ownership */
		/** @brief 初始化缓冲区内存屏障（无缓冲区传输所有权） */
		/**
		 * @brief 初始化缓冲区内存屏障
		 * @return 初始化的 VkBufferMemoryBarrier 结构
		 */
		inline VkBufferMemoryBarrier bufferMemoryBarrier()
		{
			VkBufferMemoryBarrier bufferMemoryBarrier {};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return bufferMemoryBarrier;
		}

		/**
		 * @brief 初始化内存屏障结构
		 * @return 初始化的 VkMemoryBarrier 结构
		 */
		inline VkMemoryBarrier memoryBarrier()
		{
			VkMemoryBarrier memoryBarrier {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;  // 结构体类型
			return memoryBarrier;
		}

		/**
		 * @brief 初始化图像创建信息结构
		 * @return 初始化的 VkImageCreateInfo 结构
		 */
		inline VkImageCreateInfo imageCreateInfo()
		{
			VkImageCreateInfo imageCreateInfo {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;  // 结构体类型
			return imageCreateInfo;
		}

		/**
		 * @brief 初始化采样器创建信息结构
		 * @return 初始化的 VkSamplerCreateInfo 结构（各向异性过滤设为 1.0）
		 */
		inline VkSamplerCreateInfo samplerCreateInfo()
		{
			VkSamplerCreateInfo samplerCreateInfo {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;  // 结构体类型
			samplerCreateInfo.maxAnisotropy = 1.0f;  // 最大各向异性过滤
			return samplerCreateInfo;
		}

		/**
		 * @brief 初始化图像视图创建信息结构
		 * @return 初始化的 VkImageViewCreateInfo 结构
		 */
		inline VkImageViewCreateInfo imageViewCreateInfo()
		{
			VkImageViewCreateInfo imageViewCreateInfo {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;  // 结构体类型
			return imageViewCreateInfo;
		}

		/**
		 * @brief 初始化帧缓冲区创建信息结构
		 * @return 初始化的 VkFramebufferCreateInfo 结构
		 */
		inline VkFramebufferCreateInfo framebufferCreateInfo()
		{
			VkFramebufferCreateInfo framebufferCreateInfo {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;  // 结构体类型
			return framebufferCreateInfo;
		}

		/**
		 * @brief 初始化信号量创建信息结构
		 * @return 初始化的 VkSemaphoreCreateInfo 结构
		 */
		inline VkSemaphoreCreateInfo semaphoreCreateInfo()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;  // 结构体类型
			return semaphoreCreateInfo;
		}

		/**
		 * @brief 初始化栅栏创建信息结构
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkFenceCreateInfo 结构
		 */
		inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)
		{
			VkFenceCreateInfo fenceCreateInfo {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;  // 结构体类型
			fenceCreateInfo.flags = flags;  // 创建标志
			return fenceCreateInfo;
		}

		/**
		 * @brief 初始化提交信息结构
		 * @return 初始化的 VkSubmitInfo 结构
		 */
		inline VkSubmitInfo submitInfo()
		{
			return { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };  // 使用指定初始化器
		}

		/**
		 * @brief 初始化视口结构
		 * @param width 视口宽度
		 * @param height 视口高度
		 * @param minDepth 最小深度值
		 * @param maxDepth 最大深度值
		 * @return 初始化的 VkViewport 结构
		 */
		inline VkViewport viewport(
			float width,
			float height,
			float minDepth,
			float maxDepth)
		{
			VkViewport viewport {};
			viewport.width = width;      // 视口宽度
			viewport.height = height;    // 视口高度
			viewport.minDepth = minDepth; // 最小深度值
			viewport.maxDepth = maxDepth; // 最大深度值
			return viewport;
		}

		/**
		 * @brief 初始化 2D 矩形结构
		 * @param width 矩形宽度
		 * @param height 矩形高度
		 * @param offsetX X 偏移量
		 * @param offsetY Y 偏移量
		 * @return 初始化的 VkRect2D 结构
		 */
		inline VkRect2D rect2D(
			int32_t width,
			int32_t height,
			int32_t offsetX,
			int32_t offsetY)
		{
			VkRect2D rect2D {};
			rect2D.extent.width = width;   // 矩形宽度
			rect2D.extent.height = height; // 矩形高度
			rect2D.offset.x = offsetX;     // X 偏移量
			rect2D.offset.y = offsetY;     // Y 偏移量
			return rect2D;
		}

		/**
		 * @brief 初始化缓冲区创建信息结构（无参数版本）
		 * @return 初始化的 VkBufferCreateInfo 结构
		 */
		inline VkBufferCreateInfo bufferCreateInfo()
		{
			VkBufferCreateInfo bufCreateInfo {};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  // 结构体类型
			return bufCreateInfo;
		}

		/**
		 * @brief 初始化缓冲区创建信息结构（带参数版本）
		 * @param usage 缓冲区使用标志
		 * @param size 缓冲区大小（字节）
		 * @return 初始化的 VkBufferCreateInfo 结构
		 */
		inline VkBufferCreateInfo bufferCreateInfo(
			VkBufferUsageFlags usage,
			VkDeviceSize size)
		{
			VkBufferCreateInfo bufCreateInfo {};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  // 结构体类型
			bufCreateInfo.usage = usage;  // 缓冲区使用标志
			bufCreateInfo.size = size;    // 缓冲区大小
			return bufCreateInfo;
		}

		/**
		 * @brief 初始化描述符池创建信息结构（指针版本）
		 * @param poolSizeCount 池大小数量
		 * @param pPoolSizes 池大小数组指针
		 * @param maxSets 最大描述符集数量
		 * @return 初始化的 VkDescriptorPoolCreateInfo 结构
		 */
		inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			uint32_t poolSizeCount,
			VkDescriptorPoolSize* pPoolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo {};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;  // 结构体类型
			descriptorPoolInfo.poolSizeCount = poolSizeCount;  // 池大小数量
			descriptorPoolInfo.pPoolSizes = pPoolSizes;        // 池大小数组
			descriptorPoolInfo.maxSets = maxSets;               // 最大描述符集数量
			return descriptorPoolInfo;
		}

		/**
		 * @brief 初始化描述符池创建信息结构（向量版本）
		 * @param poolSizes 池大小向量
		 * @param maxSets 最大描述符集数量
		 * @return 初始化的 VkDescriptorPoolCreateInfo 结构
		 */
		inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			const std::vector<VkDescriptorPoolSize>& poolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;  // 结构体类型
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());  // 池大小数量
			descriptorPoolInfo.pPoolSizes = poolSizes.data();  // 池大小数组
			descriptorPoolInfo.maxSets = maxSets;               // 最大描述符集数量
			return descriptorPoolInfo;
		}

		/**
		 * @brief 初始化描述符池大小结构
		 * @param type 描述符类型
		 * @param descriptorCount 描述符数量
		 * @return 初始化的 VkDescriptorPoolSize 结构
		 */
		inline VkDescriptorPoolSize descriptorPoolSize(
			VkDescriptorType type,
			uint32_t descriptorCount)
		{
			VkDescriptorPoolSize descriptorPoolSize {};
			descriptorPoolSize.type = type;                    // 描述符类型
			descriptorPoolSize.descriptorCount = descriptorCount;  // 描述符数量
			return descriptorPoolSize;
		}

		/**
		 * @brief 初始化描述符集布局绑定结构
		 * @param type 描述符类型
		 * @param stageFlags 着色器阶段标志
		 * @param binding 绑定索引
		 * @param descriptorCount 描述符数量（默认 1）
		 * @return 初始化的 VkDescriptorSetLayoutBinding 结构
		 */
		inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
			VkDescriptorType type,
			VkShaderStageFlags stageFlags,
			uint32_t binding,
			uint32_t descriptorCount = 1)
		{
			VkDescriptorSetLayoutBinding setLayoutBinding {};
			setLayoutBinding.descriptorType = type;           // 描述符类型
			setLayoutBinding.stageFlags = stageFlags;         // 着色器阶段标志
			setLayoutBinding.binding = binding;               // 绑定索引
			setLayoutBinding.descriptorCount = descriptorCount;  // 描述符数量
			return setLayoutBinding;
		}

		/**
		 * @brief 初始化描述符集布局创建信息结构（指针版本）
		 * @param pBindings 绑定数组指针
		 * @param bindingCount 绑定数量
		 * @return 初始化的 VkDescriptorSetLayoutCreateInfo 结构
		 */
		inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const VkDescriptorSetLayoutBinding* pBindings,
			uint32_t bindingCount)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;  // 结构体类型
			descriptorSetLayoutCreateInfo.pBindings = pBindings;      // 绑定数组
			descriptorSetLayoutCreateInfo.bindingCount = bindingCount;  // 绑定数量
			return descriptorSetLayoutCreateInfo;
		}

		/**
		 * @brief 初始化描述符集布局创建信息结构（向量版本）
		 * @param bindings 绑定向量
		 * @return 初始化的 VkDescriptorSetLayoutCreateInfo 结构
		 */
		inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;  // 结构体类型
			descriptorSetLayoutCreateInfo.pBindings = bindings.data();  // 绑定数组
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());  // 绑定数量
			return descriptorSetLayoutCreateInfo;
		}

		/**
		 * @brief 初始化管线布局创建信息结构
		 * @param pSetLayouts 描述符集布局数组指针
		 * @param setLayoutCount 描述符集布局数量（默认 1）
		 * @return 初始化的 VkPipelineLayoutCreateInfo 结构
		 */
		inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;  // 结构体类型
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;  // 描述符集布局数量
			pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;        // 描述符集布局数组
			return pipelineLayoutCreateInfo;
		}

		/**
		 * @brief 初始化管线布局创建信息结构（简化版本）
		 * @param setLayoutCount 描述符集布局数量（默认 1）
		 * @return 初始化的 VkPipelineLayoutCreateInfo 结构
		 */
		inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;  // 结构体类型
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;  // 描述符集布局数量
			return pipelineLayoutCreateInfo;
		}

		/**
		 * @brief 初始化描述符集分配信息结构
		 * @param descriptorPool 描述符池句柄
		 * @param pSetLayouts 描述符集布局数组指针
		 * @param descriptorSetCount 描述符集数量
		 * @return 初始化的 VkDescriptorSetAllocateInfo 结构
		 */
		inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
			VkDescriptorPool descriptorPool,
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t descriptorSetCount)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;  // 结构体类型
			descriptorSetAllocateInfo.descriptorPool = descriptorPool;  // 描述符池
			descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;        // 描述符集布局数组
			descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;  // 描述符集数量
			return descriptorSetAllocateInfo;
		}

		/**
		 * @brief 初始化描述符图像信息结构
		 * @param sampler 采样器句柄
		 * @param imageView 图像视图句柄
		 * @param imageLayout 图像布局
		 * @return 初始化的 VkDescriptorImageInfo 结构
		 */
		inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
		{
			VkDescriptorImageInfo descriptorImageInfo {};
			descriptorImageInfo.sampler = sampler;      // 采样器
			descriptorImageInfo.imageView = imageView;   // 图像视图
			descriptorImageInfo.imageLayout = imageLayout;  // 图像布局
			return descriptorImageInfo;
		}

		/**
		 * @brief 初始化写入描述符集结构（缓冲区版本）
		 * @param dstSet 目标描述符集句柄
		 * @param type 描述符类型
		 * @param binding 绑定索引
		 * @param bufferInfo 缓冲区信息指针
		 * @param descriptorCount 描述符数量（默认 1）
		 * @return 初始化的 VkWriteDescriptorSet 结构
		 */
		inline VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorBufferInfo* bufferInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;  // 结构体类型
			writeDescriptorSet.dstSet = dstSet;              // 目标描述符集
			writeDescriptorSet.descriptorType = type;         // 描述符类型
			writeDescriptorSet.dstBinding = binding;          // 目标绑定索引
			writeDescriptorSet.pBufferInfo = bufferInfo;      // 缓冲区信息
			writeDescriptorSet.descriptorCount = descriptorCount;  // 描述符数量
			return writeDescriptorSet;
		}

		/**
		 * @brief 初始化写入描述符集结构（图像版本）
		 * @param dstSet 目标描述符集句柄
		 * @param type 描述符类型
		 * @param binding 绑定索引
		 * @param imageInfo 图像信息指针
		 * @param descriptorCount 描述符数量（默认 1）
		 * @return 初始化的 VkWriteDescriptorSet 结构
		 */
		inline VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorImageInfo *imageInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;  // 结构体类型
			writeDescriptorSet.dstSet = dstSet;              // 目标描述符集
			writeDescriptorSet.descriptorType = type;         // 描述符类型
			writeDescriptorSet.dstBinding = binding;          // 目标绑定索引
			writeDescriptorSet.pImageInfo = imageInfo;        // 图像信息
			writeDescriptorSet.descriptorCount = descriptorCount;  // 描述符数量
			return writeDescriptorSet;
		}

		/**
		 * @brief 初始化顶点输入绑定描述结构
		 * @param binding 绑定索引
		 * @param stride 顶点数据步长（字节）
		 * @param inputRate 输入速率（顶点或实例）
		 * @return 初始化的 VkVertexInputBindingDescription 结构
		 */
		inline VkVertexInputBindingDescription vertexInputBindingDescription(
			uint32_t binding,
			uint32_t stride,
			VkVertexInputRate inputRate)
		{
			VkVertexInputBindingDescription vInputBindDescription {};
			vInputBindDescription.binding = binding;   // 绑定索引
			vInputBindDescription.stride = stride;     // 步长
			vInputBindDescription.inputRate = inputRate;  // 输入速率
			return vInputBindDescription;
		}

		/**
		 * @brief 初始化顶点输入属性描述结构
		 * @param binding 绑定索引
		 * @param location 属性位置（着色器中的 location）
		 * @param format 属性格式
		 * @param offset 属性在顶点数据中的偏移量（字节）
		 * @return 初始化的 VkVertexInputAttributeDescription 结构
		 */
		inline VkVertexInputAttributeDescription vertexInputAttributeDescription(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset)
		{
			VkVertexInputAttributeDescription vInputAttribDescription {};
			vInputAttribDescription.location = location;  // 属性位置
			vInputAttribDescription.binding = binding;   // 绑定索引
			vInputAttribDescription.format = format;     // 属性格式
			vInputAttribDescription.offset = offset;      // 偏移量
			return vInputAttribDescription;
		}

		/**
		 * @brief 初始化管线顶点输入状态创建信息结构（无参数版本）
		 * @return 初始化的 VkPipelineVertexInputStateCreateInfo 结构
		 */
		inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo()
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;  // 结构体类型
			return pipelineVertexInputStateCreateInfo;
		}

		/**
		 * @brief 初始化管线顶点输入状态创建信息结构（带参数版本）
		 * @param vertexBindingDescriptions 顶点绑定描述向量
		 * @param vertexAttributeDescriptions 顶点属性描述向量
		 * @return 初始化的 VkPipelineVertexInputStateCreateInfo 结构
		 */
		inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
			const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
			const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
		)
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;  // 结构体类型
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());  // 绑定描述数量
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();  // 绑定描述数组
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());  // 属性描述数量
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();  // 属性描述数组
			return pipelineVertexInputStateCreateInfo;
		}

		/**
		 * @brief 初始化管线输入装配状态创建信息结构
		 * @param topology 图元拓扑类型
		 * @param flags 创建标志
		 * @param primitiveRestartEnable 是否启用图元重启
		 * @return 初始化的 VkPipelineInputAssemblyStateCreateInfo 结构
		 */
		inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology topology,
			VkPipelineInputAssemblyStateCreateFlags flags,
			VkBool32 primitiveRestartEnable)
		{
			VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
			pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;  // 结构体类型
			pipelineInputAssemblyStateCreateInfo.topology = topology;  // 图元拓扑
			pipelineInputAssemblyStateCreateInfo.flags = flags;  // 创建标志
			pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;  // 图元重启启用
			return pipelineInputAssemblyStateCreateInfo;
		}

		/**
		 * @brief 初始化管线光栅化状态创建信息结构
		 * @param polygonMode 多边形模式
		 * @param cullMode 剔除模式
		 * @param frontFace 正面朝向
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkPipelineRasterizationStateCreateInfo 结构
		 */
		inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
			VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace,
			VkPipelineRasterizationStateCreateFlags flags = 0)
		{
			VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
			pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;  // 结构体类型
			pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;  // 多边形模式
			pipelineRasterizationStateCreateInfo.cullMode = cullMode;  // 剔除模式
			pipelineRasterizationStateCreateInfo.frontFace = frontFace;  // 正面朝向
			pipelineRasterizationStateCreateInfo.flags = flags;  // 创建标志
			pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;  // 深度夹紧禁用
			pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;  // 线宽
			return pipelineRasterizationStateCreateInfo;
		}

		/**
		 * @brief 初始化管线颜色混合附件状态结构
		 * @param colorWriteMask 颜色写入掩码
		 * @param blendEnable 是否启用混合
		 * @return 初始化的 VkPipelineColorBlendAttachmentState 结构
		 */
		inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
			VkColorComponentFlags colorWriteMask,
			VkBool32 blendEnable)
		{
			VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
			pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;  // 颜色写入掩码
			pipelineColorBlendAttachmentState.blendEnable = blendEnable;        // 混合启用标志
			return pipelineColorBlendAttachmentState;
		}

		/**
		 * @brief 初始化管线颜色混合状态创建信息结构
		 * @param attachmentCount 附件数量
		 * @param pAttachments 颜色混合附件状态数组指针
		 * @return 初始化的 VkPipelineColorBlendStateCreateInfo 结构
		 */
		inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
			uint32_t attachmentCount,
			const VkPipelineColorBlendAttachmentState * pAttachments)
		{
			VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
			pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;  // 结构体类型
			pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;  // 附件数量
			pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;  // 附件状态数组
			return pipelineColorBlendStateCreateInfo;
		}

		/**
		 * @brief 初始化管线深度模板状态创建信息结构
		 * @param depthTestEnable 是否启用深度测试
		 * @param depthWriteEnable 是否启用深度写入
		 * @param depthCompareOp 深度比较操作
		 * @return 初始化的 VkPipelineDepthStencilStateCreateInfo 结构
		 */
		inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
			VkBool32 depthTestEnable,
			VkBool32 depthWriteEnable,
			VkCompareOp depthCompareOp)
		{
			VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
			pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;  // 结构体类型
			pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;      // 深度测试启用
			pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;    // 深度写入启用
			pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;       // 深度比较操作
			pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;  // 背面模板比较操作
			return pipelineDepthStencilStateCreateInfo;
		}

		/**
		 * @brief 初始化管线视口状态创建信息结构
		 * @param viewportCount 视口数量
		 * @param scissorCount 剪裁矩形数量
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkPipelineViewportStateCreateInfo 结构
		 */
		inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
			uint32_t viewportCount,
			uint32_t scissorCount,
			VkPipelineViewportStateCreateFlags flags = 0)
		{
			VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
			pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;  // 结构体类型
			pipelineViewportStateCreateInfo.viewportCount = viewportCount;  // 视口数量
			pipelineViewportStateCreateInfo.scissorCount = scissorCount;    // 剪裁矩形数量
			pipelineViewportStateCreateInfo.flags = flags;                 // 创建标志
			return pipelineViewportStateCreateInfo;
		}

		/**
		 * @brief 初始化管线多重采样状态创建信息结构
		 * @param rasterizationSamples 光栅化采样数
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkPipelineMultisampleStateCreateInfo 结构
		 */
		inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
			VkSampleCountFlagBits rasterizationSamples,
			VkPipelineMultisampleStateCreateFlags flags = 0)
		{
			VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
			pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;  // 结构体类型
			pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;  // 光栅化采样数
			pipelineMultisampleStateCreateInfo.flags = flags;  // 创建标志
			return pipelineMultisampleStateCreateInfo;
		}

		/**
		 * @brief 初始化管线动态状态创建信息结构（指针版本）
		 * @param pDynamicStates 动态状态数组指针
		 * @param dynamicStateCount 动态状态数量
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkPipelineDynamicStateCreateInfo 结构
		 */
		inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const VkDynamicState * pDynamicStates,
			uint32_t dynamicStateCount,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo {};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;  // 结构体类型
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;        // 动态状态数组
			pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;  // 动态状态数量
			pipelineDynamicStateCreateInfo.flags = flags;                          // 创建标志
			return pipelineDynamicStateCreateInfo;
		}

		/**
		 * @brief 初始化管线动态状态创建信息结构（向量版本）
		 * @param pDynamicStates 动态状态向量
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkPipelineDynamicStateCreateInfo 结构
		 */
		inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const std::vector<VkDynamicState>& pDynamicStates,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;  // 结构体类型
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();  // 动态状态数组
			pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());  // 动态状态数量
			pipelineDynamicStateCreateInfo.flags = flags;  // 创建标志
			return pipelineDynamicStateCreateInfo;
		}

		/**
		 * @brief 初始化管线细分状态创建信息结构
		 * @param patchControlPoints 补丁控制点数量
		 * @return 初始化的 VkPipelineTessellationStateCreateInfo 结构
		 */
		inline VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
		{
			VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo {};
			pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;  // 结构体类型
			pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;  // 补丁控制点数量
			return pipelineTessellationStateCreateInfo;
		}

		/**
		 * @brief 初始化图形管线创建信息结构（带参数版本）
		 * @param layout 管线布局句柄
		 * @param renderPass 渲染通道句柄
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkGraphicsPipelineCreateInfo 结构
		 */
		inline VkGraphicsPipelineCreateInfo pipelineCreateInfo(
			VkPipelineLayout layout,
			VkRenderPass renderPass,
			VkPipelineCreateFlags flags = 0)
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;  // 结构体类型
			pipelineCreateInfo.layout = layout;        // 管线布局
			pipelineCreateInfo.renderPass = renderPass;  // 渲染通道
			pipelineCreateInfo.flags = flags;         // 创建标志
			pipelineCreateInfo.basePipelineIndex = -1;  // 基础管线索引（-1 表示无基础管线）
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;  // 基础管线句柄（NULL 表示无基础管线）
			return pipelineCreateInfo;
		}

		/**
		 * @brief 初始化图形管线创建信息结构（无参数版本）
		 * @return 初始化的 VkGraphicsPipelineCreateInfo 结构
		 */
		inline VkGraphicsPipelineCreateInfo pipelineCreateInfo()
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;  // 结构体类型
			pipelineCreateInfo.basePipelineIndex = -1;         // 基础管线索引（-1 表示无基础管线）
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;  // 基础管线句柄（NULL 表示无基础管线）
			return pipelineCreateInfo;
		}

		/**
		 * @brief 初始化计算管线创建信息结构
		 * @param layout 管线布局句柄
		 * @param flags 创建标志（默认 0）
		 * @return 初始化的 VkComputePipelineCreateInfo 结构
		 */
		inline VkComputePipelineCreateInfo computePipelineCreateInfo(
			VkPipelineLayout layout, 
			VkPipelineCreateFlags flags = 0)
		{
			VkComputePipelineCreateInfo computePipelineCreateInfo {};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;  // 结构体类型
			computePipelineCreateInfo.layout = layout;  // 管线布局
			computePipelineCreateInfo.flags = flags;    // 创建标志
			return computePipelineCreateInfo;
		}

		/**
		 * @brief 初始化推送常量范围结构
		 * @param stageFlags 着色器阶段标志
		 * @param size 推送常量大小（字节）
		 * @param offset 推送常量偏移量（字节）
		 * @return 初始化的 VkPushConstantRange 结构
		 */
		inline VkPushConstantRange pushConstantRange(
			VkShaderStageFlags stageFlags,
			uint32_t size,
			uint32_t offset)
		{
			VkPushConstantRange pushConstantRange {};
			pushConstantRange.stageFlags = stageFlags;  // 着色器阶段标志
			pushConstantRange.offset = offset;          // 偏移量
			pushConstantRange.size = size;               // 大小
			return pushConstantRange;
		}

		/**
		 * @brief 初始化稀疏绑定信息结构
		 * @return 初始化的 VkBindSparseInfo 结构
		 */
		inline VkBindSparseInfo bindSparseInfo()
		{
			VkBindSparseInfo bindSparseInfo{};
			bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;  // 结构体类型
			return bindSparseInfo;
		}

		/** @brief Initialize a map entry for a shader specialization constant */
		/** @brief 初始化着色器特化常量的映射条目 */
		/**
		 * @brief 初始化特化映射条目结构
		 * @param constantID 常量 ID
		 * @param offset 数据偏移量（字节）
		 * @param size 数据大小（字节）
		 * @return 初始化的 VkSpecializationMapEntry 结构
		 */
		inline VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
		{
			VkSpecializationMapEntry specializationMapEntry{};
			specializationMapEntry.constantID = constantID;  // 常量 ID
			specializationMapEntry.offset = offset;          // 偏移量
			specializationMapEntry.size = size;               // 大小
			return specializationMapEntry;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		/** @brief 初始化特化常量信息结构，传递给着色器阶段（指针版本） */
		/**
		 * @brief 初始化特化信息结构（指针版本）
		 * @param mapEntryCount 映射条目数量
		 * @param mapEntries 映射条目数组指针
		 * @param dataSize 特化数据大小（字节）
		 * @param data 特化数据指针
		 * @return 初始化的 VkSpecializationInfo 结构
		 */
		inline VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = mapEntryCount;  // 映射条目数量
			specializationInfo.pMapEntries = mapEntries;       // 映射条目数组
			specializationInfo.dataSize = dataSize;            // 数据大小
			specializationInfo.pData = data;                   // 数据指针
			return specializationInfo;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		/** @brief 初始化特化常量信息结构，传递给着色器阶段（向量版本） */
		/**
		 * @brief 初始化特化信息结构（向量版本）
		 * @param mapEntries 映射条目向量
		 * @param dataSize 特化数据大小（字节）
		 * @param data 特化数据指针
		 * @return 初始化的 VkSpecializationInfo 结构
		 */
		inline VkSpecializationInfo specializationInfo(const std::vector<VkSpecializationMapEntry> &mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());  // 映射条目数量
			specializationInfo.pMapEntries = mapEntries.data();  // 映射条目数组
			specializationInfo.dataSize = dataSize;               // 数据大小
			specializationInfo.pData = data;                      // 数据指针
			return specializationInfo;
		}

		// Ray tracing related
		// 光线追踪相关
		/**
		 * @brief 初始化加速结构几何结构
		 * @return 初始化的 VkAccelerationStructureGeometryKHR 结构
		 */
		inline VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR()
		{
			VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR{};
			accelerationStructureGeometryKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;  // 结构体类型
			return accelerationStructureGeometryKHR;
		}

		/**
		 * @brief 初始化加速结构构建几何信息结构
		 * @return 初始化的 VkAccelerationStructureBuildGeometryInfoKHR 结构
		 */
		inline VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR()
		{
			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR{};
			accelerationStructureBuildGeometryInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;  // 结构体类型
			return accelerationStructureBuildGeometryInfoKHR;
		}

		/**
		 * @brief 初始化加速结构构建大小信息结构
		 * @return 初始化的 VkAccelerationStructureBuildSizesInfoKHR 结构
		 */
		inline VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR()
		{
			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR{};
			accelerationStructureBuildSizesInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;  // 结构体类型
			return accelerationStructureBuildSizesInfoKHR;
		}

		/**
		 * @brief 初始化光线追踪着色器组创建信息结构
		 * @return 初始化的 VkRayTracingShaderGroupCreateInfoKHR 结构
		 */
		inline VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR()
		{
			VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR{};
			rayTracingShaderGroupCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;  // 结构体类型
			return rayTracingShaderGroupCreateInfoKHR;
		}

		/**
		 * @brief 初始化光线追踪管线创建信息结构
		 * @return 初始化的 VkRayTracingPipelineCreateInfoKHR 结构
		 */
		inline VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR()
		{
			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR{};
			rayTracingPipelineCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;  // 结构体类型
			return rayTracingPipelineCreateInfoKHR;
		}

		/**
		 * @brief 初始化写入描述符集加速结构结构
		 * @return 初始化的 VkWriteDescriptorSetAccelerationStructureKHR 结构
		 */
		inline VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR()
		{
			VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR{};
			writeDescriptorSetAccelerationStructureKHR.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;  // 结构体类型
			return writeDescriptorSetAccelerationStructureKHR;
		}

	}
}