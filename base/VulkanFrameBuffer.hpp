/*
* Vulkan framebuffer class
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <algorithm>
#include <iterator>
#include <vector>
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"
#include "VulkanTools.h"

namespace vks
{
	/**
	* @brief Encapsulates a single frame buffer attachment 
	* @brief 封装单个帧缓冲区附件
	*/
	struct FramebufferAttachment
	{
		VkImage image;                        // 图像句柄
		VkDeviceMemory memory;                // 内存句柄
		VkImageView view;                     // 图像视图句柄
		VkFormat format;                      // 图像格式
		VkImageSubresourceRange subresourceRange;  // 子资源范围
		VkAttachmentDescription description;  // 附件描述

		/**
		* @brief Returns true if the attachment has a depth component
		* @brief 如果附件具有深度分量，返回 true
		* @return 如果格式包含深度分量返回 true
		*/
		bool hasDepth()
		{
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_D16_UNORM,
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment has a stencil component
		* @brief 如果附件具有模板分量，返回 true
		* @return 如果格式包含模板分量返回 true
		*/
		bool hasStencil()
		{
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment is a depth and/or stencil attachment
		* @brief 如果附件是深度和/或模板附件，返回 true
		* @return 如果是深度或模板附件返回 true
		*/
		bool isDepthStencil()
		{
			return(hasDepth() || hasStencil());
		}

	};

	/**
	* @brief Describes the attributes of an attachment to be created
	* @brief 描述要创建的附件的属性
	*/
	struct AttachmentCreateInfo
	{
		uint32_t width, height;                    // 宽度和高度
		uint32_t layerCount;                       // 层数量
		VkFormat format;                           // 图像格式
		VkImageUsageFlags usage;                   // 图像使用标志
		VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;  // 采样数
	};

	/**
	* @brief Encapsulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
	* @brief 封装一个完整的 Vulkan 帧缓冲区，具有任意数量和组合的附件
	*/
	struct Framebuffer
	{
	private:
		vks::VulkanDevice *vulkanDevice;  // Vulkan 设备指针
	public:
		uint32_t width, height;                          // 帧缓冲区宽度和高度
		VkFramebuffer framebuffer;                      // 帧缓冲区句柄
		VkRenderPass renderPass;                         // 渲染通道句柄
		VkSampler sampler;                              // 采样器句柄
		std::vector<vks::FramebufferAttachment> attachments;  // 附件列表

		/**
		* Default constructor
		* 默认构造函数
		*
		* @param vulkanDevice Pointer to a valid VulkanDevice
		* @param vulkanDevice 指向有效 VulkanDevice 的指针
		*/
		Framebuffer(vks::VulkanDevice *vulkanDevice)
		{
			assert(vulkanDevice);
			this->vulkanDevice = vulkanDevice;
		}

		/**
		* Destroy and free Vulkan resources used for the framebuffer and all of its attachments
		* 销毁并释放帧缓冲区及其所有附件使用的 Vulkan 资源
		*/
		~Framebuffer()
		{
			assert(vulkanDevice);
			for (auto attachment : attachments)
			{
				vkDestroyImage(vulkanDevice->logicalDevice, attachment.image, nullptr);
				vkDestroyImageView(vulkanDevice->logicalDevice, attachment.view, nullptr);
				vkFreeMemory(vulkanDevice->logicalDevice, attachment.memory, nullptr);
			}
			vkDestroySampler(vulkanDevice->logicalDevice, sampler, nullptr);
			vkDestroyRenderPass(vulkanDevice->logicalDevice, renderPass, nullptr);
			vkDestroyFramebuffer(vulkanDevice->logicalDevice, framebuffer, nullptr);
		}

		/**
		* Add a new attachment described by createinfo to the framebuffer's attachment list
		* 向帧缓冲区的附件列表添加由 createinfo 描述的新附件
		*
		* @param createinfo Structure that specifies the framebuffer to be constructed
		* @param createinfo 指定要构造的帧缓冲区的结构
		*
		* @return Index of the new attachment
		* @return 新附件的索引
		*/
		uint32_t addAttachment(vks::AttachmentCreateInfo createinfo)
		{
			vks::FramebufferAttachment attachment;

			attachment.format = createinfo.format;

			VkImageAspectFlags aspectMask = VK_FLAGS_NONE;  // 方面掩码

			// Select aspect mask and layout depending on usage
			// 根据使用情况选择方面掩码和布局

			// Color attachment
			// 颜色附件
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			// Depth (and/or stencil) attachment
			// 深度（和/或模板）附件
			if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (attachment.hasDepth())
				{
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if (attachment.hasStencil())
				{
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			assert(aspectMask > 0);

			VkImageCreateInfo image = vks::initializers::imageCreateInfo();
			image.imageType = VK_IMAGE_TYPE_2D;
			image.format = createinfo.format;
			image.extent.width = createinfo.width;
			image.extent.height = createinfo.height;
			image.extent.depth = 1;
			image.mipLevels = 1;
			image.arrayLayers = createinfo.layerCount;
			image.samples = createinfo.imageSampleCount;
			image.tiling = VK_IMAGE_TILING_OPTIMAL;
			image.usage = createinfo.usage;

			VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;  // 内存需求

			// Create image for this attachment
			// 为此附件创建图像
			VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &image, nullptr, &attachment.image));
			vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, attachment.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 设备本地内存
			VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memAlloc, nullptr, &attachment.memory));
			VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, attachment.image, attachment.memory, 0));

			attachment.subresourceRange = {};
			attachment.subresourceRange.aspectMask = aspectMask;  // 方面掩码
			attachment.subresourceRange.levelCount = 1;           // Mip 级别数
			attachment.subresourceRange.layerCount = createinfo.layerCount;  // 层数

			VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
			imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;  // 根据层数选择视图类型
			imageView.format = createinfo.format;
			imageView.subresourceRange = attachment.subresourceRange;
			imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;  // 深度附件使用深度方面
			imageView.image = attachment.image;
			VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &imageView, nullptr, &attachment.view));

			// Fill attachment description
			// 填充附件描述
			attachment.description = {};
			attachment.description.samples = createinfo.imageSampleCount;  // 采样数
			attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;     // 加载操作：清除
			attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;  // 存储操作：如果用于采样则存储
			attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // 模板加载操作：不关心
			attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // 模板存储操作：不关心
			attachment.description.format = createinfo.format;
			attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // 初始布局：未定义
			// Final layout
			// If not, final layout depends on attachment type
			// 最终布局
			// 根据附件类型决定最终布局
			if (attachment.hasDepth() || attachment.hasStencil())
			{
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // 深度模板只读最优
			}
			else
			{
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // 着色器只读最优
			}

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		/**
		* Creates a default sampler for sampling from any of the framebuffer attachments
		* Applications are free to create their own samplers for different use cases 
		* 创建默认采样器，用于从任何帧缓冲区附件采样
		* 应用程序可以自由创建自己的采样器以用于不同的用例
		*
		* @param magFilter Magnification filter for lookups
		* @param magFilter 查找的放大过滤器
		* @param minFilter Minification filter for lookups
		* @param minFilter 查找的缩小过滤器
		* @param adressMode Addressing mode for the U,V and W coordinates
		* @param adressMode U、V 和 W 坐标的寻址模式
		*
		* @return VkResult for the sampler creation
		* @return 采样器创建的 VkResult
		*/
		VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode)
		{
			VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = adressMode;
			samplerInfo.addressModeV = adressMode;
			samplerInfo.addressModeW = adressMode;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			return vkCreateSampler(vulkanDevice->logicalDevice, &samplerInfo, nullptr, &sampler);
		}

		/**
		* Creates a default render pass setup with one sub pass
		* 创建具有一个子通道的默认渲染通道设置
		*
		* @return VK_SUCCESS if all resources have been created successfully
		* @return 如果所有资源都已成功创建，返回 VK_SUCCESS
		*/
		VkResult createRenderPass()
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;  // 附件描述列表
			for (auto& attachment : attachments)
			{
				attachmentDescriptions.push_back(attachment.description);
			};

			// Collect attachment references
			// 收集附件引用
			std::vector<VkAttachmentReference> colorReferences;  // 颜色附件引用列表
			VkAttachmentReference depthReference = {};           // 深度附件引用
			bool hasDepth = false;                                // 是否有深度附件
			bool hasColor = false;                                // 是否有颜色附件

			uint32_t attachmentIndex = 0;  // 附件索引

			for (auto& attachment : attachments)
			{
				if (attachment.isDepthStencil())
				{
					// Only one depth attachment allowed
					// 只允许一个深度附件
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				}
				else
				{
					colorReferences.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
					hasColor = true;
				}
				attachmentIndex++;
			};

			// Default render pass setup uses only one subpass
			// 默认渲染通道设置仅使用一个子通道
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;  // 图形管线绑定点
			if (hasColor)
			{
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth)
			{
				subpass.pDepthStencilAttachment = &depthReference;
			}

			// Use subpass dependencies for attachment layout transitions
			// 使用子通道依赖进行附件布局转换
			std::array<VkSubpassDependency, 4> dependencies{};  // 子通道依赖数组

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;


			dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].dstSubpass = 0;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			dependencies[2].srcSubpass = 0;
			dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			dependencies[3].srcSubpass = 0;
			dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[3].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			// Create render pass
			// 创建渲染通道
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 4;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));

			std::vector<VkImageView> attachmentViews;  // 附件视图列表
			for (auto& attachment : attachments)
			{
				attachmentViews.push_back(attachment.view);
			}

			// Find. max number of layers across attachments
			// 查找附件中的最大层数
			uint32_t maxLayers = 0;
			for (auto& attachment : attachments)
			{
				if (attachment.subresourceRange.layerCount > maxLayers)
				{
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			// Create framebuffer
			// 创建帧缓冲区
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = maxLayers;  // 使用最大层数
			VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->logicalDevice, &framebufferInfo, nullptr, &framebuffer));

			return VK_SUCCESS;
		}
	};
}