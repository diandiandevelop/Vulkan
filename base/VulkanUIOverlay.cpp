
/*
* UI overlay class using ImGui
*
* Copyright (C) 2017-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanUIOverlay.h"

namespace vks 
{
	/**
	 * @brief UIOverlay 构造函数
	 * 初始化 ImGui 上下文并设置样式
	 */
	UIOverlay::UIOverlay()
	{
#if defined(__ANDROID__)		
		// Android 平台根据屏幕密度设置缩放因子
		if (vks::android::screenDensity >= ACONFIGURATION_DENSITY_XXHIGH) {
			scale = 3.5f;
		}
		else if (vks::android::screenDensity >= ACONFIGURATION_DENSITY_XHIGH) {
			scale = 2.5f;
		}
		else if (vks::android::screenDensity >= ACONFIGURATION_DENSITY_HIGH) {
			scale = 2.0f;
		};
#endif
		// Init ImGui
		// 初始化 ImGui 上下文
		ImGui::CreateContext();
		// Color scheme
		// 设置颜色方案（红色主题）
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
		style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		// Dimensions
		// 设置全局字体缩放
		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = scale;
	}

	/**
	 * @brief UIOverlay 析构函数
	 * 销毁 ImGui 上下文
	 */
	UIOverlay::~UIOverlay()	{
		if (ImGui::GetCurrentContext()) {
			ImGui::DestroyContext();
		}
	}

	/** Prepare all vulkan resources required to render the UI overlay */
	/** 准备渲染 UI 叠加层所需的所有 Vulkan 资源 */
	/**
	 * @brief 准备 UI 渲染资源
	 * 创建字体纹理、描述符池、描述符集布局等
	 */
	void UIOverlay::prepareResources()
	{
		assert(maxConcurrentFrames > 0);

		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		// 创建字体纹理
		unsigned char* fontData;  // 字体数据指针
		int texWidth, texHeight;  // 纹理宽度和高度
#if defined(__ANDROID__)
		float scale = (float)vks::android::screenDensity / (float)ACONFIGURATION_DENSITY_MEDIUM;
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, "Roboto-Medium.ttf", AASSET_MODE_STREAMING);
		if (asset) {
			size_t size = AAsset_getLength(asset);
			assert(size > 0);
			char *fontAsset = new char[size];
			AAsset_read(asset, fontAsset, size);
			AAsset_close(asset);
			io.Fonts->AddFontFromMemoryTTF(fontAsset, size, 14.0f * scale);
			delete[] fontAsset;
		}
#else
		const std::string filename = getAssetPath() + "Roboto-Medium.ttf";
		io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);
#endif
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);  // 上传大小（RGBA32，每像素4字节）

		// Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
		// 设置 ImGui 样式缩放因子以处理 Retina 和其他高 DPI 显示器（与上面的字体缩放相同）
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(scale);

		// Create target image for copy
		// 创建用于复制的目标图像
		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {.width = (uint32_t)texWidth, .height = (uint32_t)texHeight, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage));
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
		VkMemoryAllocateInfo memAllocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memReqs.size,
			.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &fontMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0));

		// Image view
		VkImageViewCreateInfo viewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = fontImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView));

		// Staging buffers for font data upload
		// 用于字体数据上传的暂存缓冲区
		vks::Buffer stagingBuffer;
		VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, uploadSize));
		stagingBuffer.map();
		memcpy(stagingBuffer.mapped, fontData, uploadSize);

		// Copy buffer data to font image
		// 将缓冲区数据复制到字体图像
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		// Prepare for transfer
		// 准备传输
		vks::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);
		// Copy
		VkBufferImageCopy bufferCopyRegion{
			.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
			.imageExtent = {.width = (uint32_t)texWidth, .height = (uint32_t)texHeight, .depth = 1 }
		};
		vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
		// Prepare for shader read
		vks::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		device->flushCommandBuffer(copyCmd, queue, true);

		stagingBuffer.destroy();

		// Font texture Sampler
		VkSamplerCreateInfo samplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.maxAnisotropy = 1.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		};
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

		// Descriptor pool
		VkDescriptorPoolSize poolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 };
		VkDescriptorPoolCreateInfo descriptorPoolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 2, .poolSizeCount = 1, .pPoolSizes = &poolSize };
		VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Descriptor set layout
		VkDescriptorSetLayoutBinding setLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		VkDescriptorSetLayoutCreateInfo descriptorLayout{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &setLayoutBinding };
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Descriptor set
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet));
		VkDescriptorImageInfo fontDescriptor{ .sampler = sampler, .imageView = fontView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkWriteDescriptorSet writeDescriptorSets = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor);
		vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSets, 0, nullptr);

		// Buffers per max. frames-in-flight
		buffers.resize(maxConcurrentFrames);
	}

	/**
	 * @brief 准备 UI 叠加层渲染管线
	 * 创建与主应用程序解耦的单独管线，用于 UI 叠加层渲染
	 * 
	 * @param pipelineCache 管线缓存句柄，用于加速管线创建
	 * @param renderPass 渲染通道句柄（如果使用动态渲染则为 VK_NULL_HANDLE）
	 * @param colorFormat 颜色附件格式
	 * @param depthFormat 深度附件格式
	 */
	void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat)
	{
		// 创建管线布局
		// 推送常量用于 UI 渲染参数（缩放和平移）
		VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(PushConstBlock) };
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,                              // 描述符集布局数量
			.pSetLayouts = &descriptorSetLayout,               // 描述符集布局（字体纹理）
			.pushConstantRangeCount = 1,                       // 推送常量范围数量
			.pPushConstantRanges = &pushConstantRange          // 推送常量范围
		};
		// 创建管线布局
		VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// 设置 UI 渲染的图形管线状态
		// 输入装配状态：三角形列表图元，无图元重启
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		// 光栅化状态：填充模式，无背面剔除，逆时针为正面
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		// 启用混合
		// UI 渲染需要 Alpha 混合以实现透明效果
		VkPipelineColorBlendAttachmentState blendAttachmentState{
			.blendEnable = VK_TRUE,                                    // 启用混合
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,          // 源颜色混合因子：源 Alpha
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // 目标颜色混合因子：1 - 源 Alpha
			.colorBlendOp = VK_BLEND_OP_ADD,                           // 颜色混合操作：相加
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // 源 Alpha 混合因子：1 - 源 Alpha
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,                // 目标 Alpha 混合因子：0
			.alphaBlendOp = VK_BLEND_OP_ADD,                           // Alpha 混合操作：相加
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT  // 颜色写入掩码：所有分量
		};

		// 基于 ImGui 顶点定义的顶点绑定和属性
		// ImDrawVert 包含位置、UV 坐标和颜色
		std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			{ .binding = 0, .stride = sizeof(ImDrawVert), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }  // 绑定 0：每个顶点一个数据
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, pos) },  // 位置属性（location 0）：2D 浮点坐标
			{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, uv) },   // UV 坐标属性（location 1）：2D 浮点坐标
			{ .location = 2, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(ImDrawVert, col) }, // 颜色属性（location 2）：RGBA8 归一化整数
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		// 颜色混合状态：1 个颜色附件，使用上述混合配置
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		// 深度模板状态：禁用深度测试和模板测试（UI 通常不需要深度）
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
		// 视口状态：1 个视口和 1 个剪裁矩形（使用动态状态）
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		// 多重采样状态：使用指定的采样数
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(rasterizationSamples);
		// 动态状态：视口和剪裁矩形在运行时设置
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		// 图形管线创建信息
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shaders.size()),  // 着色器阶段数量
			.pStages = shaders.data(),                             // 着色器阶段数组
			.pVertexInputState = &vertexInputState,                 // 顶点输入状态
			.pInputAssemblyState = &inputAssemblyState,            // 输入装配状态
			.pViewportState = &viewportState,                      // 视口状态
			.pRasterizationState = &rasterizationState,            // 光栅化状态
			.pMultisampleState = &multisampleState,                 // 多重采样状态
			.pDepthStencilState = &depthStencilState,               // 深度模板状态
			.pColorBlendState = &colorBlendState,                   // 颜色混合状态
			.pDynamicState = &dynamicState,                         // 动态状态
			.layout = pipelineLayout,                              // 管线布局
			.renderPass = renderPass,                               // 渲染通道（可能为 NULL）
			.subpass = subpass,                                     // 子通道索引
		};
#if defined(VK_KHR_dynamic_rendering)
		// 如果使用动态渲染（renderPass 为 NULL），必须在管线创建时定义颜色、深度和模板附件
		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
		if (renderPass == VK_NULL_HANDLE) {
			pipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,                          // 颜色附件数量
				.pColorAttachmentFormats = &colorFormat,            // 颜色附件格式
				.depthAttachmentFormat = depthFormat,               // 深度附件格式
				.stencilAttachmentFormat = depthFormat              // 模板附件格式（使用深度格式）
			};
			pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;  // 链接到扩展结构
		}
#endif
		// 创建图形管线
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	/**
	 * @brief 更新包含 ImGui 元素的顶点和索引缓冲区
	 * 当需要时更新顶点和索引缓冲区，将 ImGui 的绘制数据复制到 GPU 缓冲区
	 * 
	 * @param currentBuffer 当前缓冲区索引（用于多缓冲）
	 */
	void UIOverlay::update(uint32_t currentBuffer)
	{	
		// 获取 ImGui 的绘制数据
		ImDrawData* imDrawData = ImGui::GetDrawData();

		// 如果没有绘制数据，直接返回
		if (!imDrawData) {
			return;
		}

		// 注意：对齐在缓冲区创建内部完成
		// 计算所需的缓冲区大小
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);  // 顶点缓冲区大小
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);   // 索引缓冲区大小
		
		// 如果顶点或索引数量为 0，直接返回
		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return;
		}

		// 创建大小为块大小倍数的缓冲区，以最小化重新创建的需要
		// 这减少了频繁的缓冲区重新分配，提高性能
		const VkDeviceSize chunkSize = 16384;  // 块大小（16KB）
		// 向上对齐到块大小的倍数
		vertexBufferSize = ((vertexBufferSize + chunkSize - 1) / chunkSize) * chunkSize;
		indexBufferSize = ((indexBufferSize + chunkSize - 1) / chunkSize) * chunkSize;

		// 仅在必要时重新创建顶点缓冲区
		// 如果缓冲区不存在或大小不足，重新创建
		if ((buffers[currentBuffer].vertexBuffer.buffer == VK_NULL_HANDLE) || (buffers[currentBuffer].vertexBuffer.size < vertexBufferSize)) {
			// 取消映射并销毁旧缓冲区
			buffers[currentBuffer].vertexBuffer.unmap();
			buffers[currentBuffer].vertexBuffer.destroy();
			// 创建新的顶点缓冲区（主机可见，用于 CPU 写入）
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &buffers[currentBuffer].vertexBuffer, vertexBufferSize));
			buffers[currentBuffer].vertexCount = imDrawData->TotalVtxCount;  // 保存顶点数量
			// 映射缓冲区以便写入
			buffers[currentBuffer].vertexBuffer.map();
		}

		// 仅在必要时重新创建索引缓冲区
		// 如果缓冲区不存在或大小不足，重新创建
		if ((buffers[currentBuffer].indexBuffer.buffer == VK_NULL_HANDLE) || (buffers[currentBuffer].indexBuffer.size < indexBufferSize)) {
			// 取消映射并销毁旧缓冲区
			buffers[currentBuffer].indexBuffer.unmap();
			buffers[currentBuffer].indexBuffer.destroy();
			// 创建新的索引缓冲区（主机可见，用于 CPU 写入）
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &buffers[currentBuffer].indexBuffer, indexBufferSize));
			buffers[currentBuffer].indexCount = imDrawData->TotalIdxCount;  // 保存索引数量
			// 映射缓冲区以便写入
			buffers[currentBuffer].indexBuffer.map();
		}

		// 上传数据
		// 获取映射后的缓冲区指针
		ImDrawVert* vtxDst = (ImDrawVert*)buffers[currentBuffer].vertexBuffer.mapped;  // 顶点数据目标指针
		ImDrawIdx* idxDst = (ImDrawIdx*)buffers[currentBuffer].indexBuffer.mapped;      // 索引数据目标指针

		// 遍历所有 ImGui 绘制列表，复制顶点和索引数据
		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];  // 当前绘制列表
			// 复制顶点数据
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			// 复制索引数据
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			// 更新目标指针位置
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// 刷新以使写入对 GPU 可见
		// 对于非一致性内存，需要显式刷新
		buffers[currentBuffer].vertexBuffer.flush();
		buffers[currentBuffer].indexBuffer.flush();
	}

	/**
	 * @brief 绘制 UI（记录到命令缓冲区）
	 * 将 UI 绘制命令记录到命令缓冲区，包括绑定管线、描述符集、顶点/索引缓冲区和绘制调用
	 * 
	 * @param commandBuffer 命令缓冲区句柄，用于记录绘制命令
	 * @param currentBuffer 当前缓冲区索引（用于多缓冲）
	 */
	void UIOverlay::draw(const VkCommandBuffer commandBuffer, uint32_t currentBuffer)
	{
		// 获取 ImGui 的绘制数据
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;  // 顶点偏移量
		int32_t indexOffset = 0;    // 索引偏移量

		// 如果没有绘制数据或绘制列表为空，直接返回
		if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
			return;
		}

		// 如果缓冲区无效，直接返回
		if (buffers[currentBuffer].vertexBuffer.buffer == VK_NULL_HANDLE || buffers[currentBuffer].indexBuffer.buffer == VK_NULL_HANDLE) {
			return;
		}

		// 获取 ImGui IO 对象（用于获取显示尺寸）
		ImGuiIO& io = ImGui::GetIO();
		
		// 绑定图形管线
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 绑定描述符集（字体纹理）
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		// 设置推送常量（缩放和平移）
		// 缩放：将屏幕坐标转换为 NDC 坐标（-1 到 1）
		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		// 平移：将屏幕坐标原点从左上角移到中心
		pushConstBlock.translate = glm::vec2(-1.0f);
		// 推送常量到着色器
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

		// 确保缓冲区有效
		assert(buffers[currentBuffer].vertexBuffer.buffer != VK_NULL_HANDLE && buffers[currentBuffer].indexBuffer.buffer != VK_NULL_HANDLE);

		// 绑定顶点缓冲区
		VkDeviceSize offsets[1] = { 0 };  // 偏移量数组
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers[currentBuffer].vertexBuffer.buffer, offsets);
		// 绑定索引缓冲区（16 位无符号整数索引）
		vkCmdBindIndexBuffer(commandBuffer, buffers[currentBuffer].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		// 遍历所有绘制列表和绘制命令
		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];  // 当前绘制列表
			// 遍历绘制列表中的所有绘制命令
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];  // 当前绘制命令
				// 设置剪裁矩形（从 ImGui 的剪裁矩形转换）
				VkRect2D scissorRect{
					.offset = {.x = std::max((int32_t)(pcmd->ClipRect.x), 0), .y = std::max((int32_t)(pcmd->ClipRect.y), 0) },  // 偏移（确保非负）
					.extent = {.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x), .height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y) }  // 尺寸
				};
				// 设置剪裁矩形
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				// 执行索引绘制
				// ElemCount: 索引数量，1: 实例数量，indexOffset: 索引偏移，vertexOffset: 顶点偏移，0: 第一个实例 ID
				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				// 更新索引偏移量
				indexOffset += pcmd->ElemCount;
			}
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && TARGET_OS_SIMULATOR
			// Apple 设备模拟器不支持 vertexOffset > 0 的 vkCmdDrawIndexed()，因此重新绑定顶点缓冲区
			// 更新偏移量并重新绑定
			offsets[0] += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers[currentBuffer].vertexBuffer.buffer, offsets);
#else
			// 其他平台：更新顶点偏移量（使用 vertexOffset 参数）
			vertexOffset += cmd_list->VtxBuffer.Size;
#endif
		}
	}

	/**
	 * @brief 调整 UI 大小
	 * 更新 ImGui 的显示尺寸以适应窗口大小变化
	 * 
	 * @param width 新的宽度（像素）
	 * @param height 新的高度（像素）
	 */
	void UIOverlay::resize(uint32_t width, uint32_t height)
	{
		ImGuiIO& io = ImGui::GetIO();
		// 更新 ImGui 的显示尺寸
		io.DisplaySize = ImVec2((float)(width), (float)(height));
	}

	/**
	 * @brief 释放所有资源
	 * 销毁 UI 叠加层使用的所有 Vulkan 资源，包括缓冲区、图像、采样器、描述符和管线
	 */
	void UIOverlay::freeResources()
	{
		// 销毁所有帧的顶点和索引缓冲区
		for (auto& buffer : buffers) {
			buffer.vertexBuffer.destroy();  // 销毁顶点缓冲区
			buffer.indexBuffer.destroy();   // 销毁索引缓冲区
		}
		// 销毁字体相关资源
		vkDestroyImageView(device->logicalDevice, fontView, nullptr);   // 销毁字体图像视图
		vkDestroyImage(device->logicalDevice, fontImage, nullptr);      // 销毁字体图像
		vkFreeMemory(device->logicalDevice, fontMemory, nullptr);       // 释放字体内存
		// 销毁采样器
		vkDestroySampler(device->logicalDevice, sampler, nullptr);
		// 销毁描述符相关资源
		vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);  // 销毁描述符集布局
		vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);              // 销毁描述符池
		// 销毁管线相关资源
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);  // 销毁管线布局
		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);                // 销毁图形管线
	}

	bool UIOverlay::header(const char *caption)
	{
		return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
	}

	bool UIOverlay::checkBox(const char *caption, bool *value)
	{
		return ImGui::Checkbox(caption, value);
	}

	bool UIOverlay::checkBox(const char *caption, int32_t *value)
	{
		bool val = (*value == 1);
		bool res = ImGui::Checkbox(caption, &val);
		*value = val;
		return res;
	}

	bool UIOverlay::radioButton(const char* caption, bool value)
	{
		return ImGui::RadioButton(caption, value);
	}

	bool UIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision)
	{
		return ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
	}

	bool UIOverlay::sliderFloat(const char* caption, float* value, float min, float max)
	{
		return ImGui::SliderFloat(caption, value, min, max);
	}

	bool UIOverlay::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
	{
		return ImGui::SliderInt(caption, value, min, max);
	}

	bool UIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items)
	{
		if (items.empty()) {
			return false;
		}
		std::vector<const char*> charitems;
		charitems.reserve(items.size());
		for (size_t i = 0; i < items.size(); i++) {
			charitems.push_back(items[i].c_str());
		}
		uint32_t itemCount = static_cast<uint32_t>(charitems.size());
		return ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
	}

	bool UIOverlay::button(const char *caption)
	{
		return ImGui::Button(caption);
	}

	bool UIOverlay::colorPicker(const char* caption, float* color) {
		return ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
	}

	void UIOverlay::text(const char *formatstr, ...)
	{
		va_list args;
		va_start(args, formatstr);
		ImGui::TextV(formatstr, args);
		va_end(args);
	}
}
