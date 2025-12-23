/*
* Vulkan texture loader for KTX files
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

#include <VulkanTexture.h>

namespace vks
{
	/**
	 * @brief 更新描述符信息
	 */
	void Texture::updateDescriptor()
	{
		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	/**
	 * @brief 销毁纹理资源
	 */
	void Texture::destroy()
	{
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		vkDestroyImage(device->logicalDevice, image, nullptr);
		if (sampler)
		{
			vkDestroySampler(device->logicalDevice, sampler, nullptr);
		}
		vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
	}

	/**
	 * @brief 加载 KTX 文件
	 * @param filename 文件名
	 * @param target 输出的 KTX 纹理指针
	 * @return KTX 结果
	 */
	ktxResult Texture::loadKTXFile(std::string filename, ktxTexture **target)
	{
		ktxResult result = KTX_SUCCESS;
#if defined(__ANDROID__)
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		if (!asset) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		size_t size = AAsset_getLength(asset);
		assert(size > 0);
		ktx_uint8_t *textureData = new ktx_uint8_t[size];
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);
		result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);
		delete[] textureData;
#else
		if (!vks::tools::fileExists(filename)) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);			
#endif		
		return result;
	}

	/**
	* Load a 2D texture including all mip levels
	* 加载 2D 纹理，包括所有 Mip 级别
	*
	* @param filename File to load (supports .ktx)
	* @param filename 要加载的文件名（支持 .ktx）
	* @param format Vulkan format of the image data stored in the file
	* @param format 文件中存储的图像数据的 Vulkan 格式
	* @param device Vulkan device to create the texture on
	* @param device 要在其上创建纹理的 Vulkan 设备
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param copyQueue 用于纹理暂存复制命令的队列（必须支持传输）
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param imageUsageFlags (可选) 纹理图像的使用标志（默认为 VK_IMAGE_USAGE_SAMPLED_BIT）
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	* @param imageLayout (可选) 纹理的使用布局（默认为 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL）
	*
	*/
	void Texture2D::loadFromFile(std::string filename, VkFormat format, vks::VulkanDevice *device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);

		this->device = device;  // 保存设备指针
		width = ktxTexture->baseWidth;  // 获取纹理宽度
		height = ktxTexture->baseHeight;  // 获取纹理高度
		mipLevels = ktxTexture->numLevels;  // 获取 Mip 级别数量

		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);  // 获取 KTX 纹理数据指针
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);  // 获取 KTX 纹理数据大小

		// Get device properties for the requested texture format
		// 获取请求纹理格式的设备属性
		// 用于检查格式支持的功能（如线性过滤、最优平铺等）
		VkFormatProperties formatProperties;  // 格式属性
		vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);  // 查询格式属性

		// Use a separate command buffer for texture loading
		// 使用单独的命令缓冲区进行纹理加载
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// Create a host-visible staging buffer that contains the raw image data
		// 创建包含原始图像数据的主机可见暂存缓冲区
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // 结构体类型
			.size = ktxTextureSize,  // 缓冲区大小（KTX 纹理数据大小）
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // 用途：传输源（用于复制到图像）
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE  // 共享模式：独占（单队列族使用）
		};
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));  // 创建暂存缓冲区

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		// 获取暂存缓冲区的内存需求（对齐、内存类型位等）
		VkMemoryRequirements memReqs;  // 内存需求
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);  // 查询缓冲区内存需求
		VkMemoryAllocateInfo memAllocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // 结构体类型
			.allocationSize = memReqs.size,  // 分配大小
			// Get memory type index for a host visible buffer
			// 获取主机可见缓冲区的内存类型索引
			// 主机可见和一致性内存允许 CPU 直接访问，无需手动刷新
			.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		};
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));  // 分配暂存内存
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));  // 将内存绑定到缓冲区

		// Copy texture data into staging buffer
		// 将纹理数据复制到暂存缓冲区
		uint8_t* data{ nullptr };  // 映射的内存指针
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));  // 映射内存到主机可访问的地址空间
		memcpy(data, ktxTextureData, ktxTextureSize);  // 将 KTX 纹理数据复制到映射的内存
		vkUnmapMemory(device->logicalDevice, stagingMemory);  // 取消映射内存（一致性内存会自动刷新到 GPU）

		// Setup buffer copy regions for each mip level
		// 为每个 Mip 级别设置缓冲区复制区域
		std::vector<VkBufferImageCopy> bufferCopyRegions;  // 缓冲区到图像复制区域向量

		for (uint32_t i = 0; i < mipLevels; i++) {  // 遍历所有 Mip 级别
			ktx_size_t offset;  // KTX 图像在数据中的偏移量
			KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);  // 获取 Mip 级别 i 的图像偏移量（层级 0，面 0）
			assert(result == KTX_SUCCESS);  // 确保获取成功
			VkBufferImageCopy bufferCopyRegion{
				.bufferOffset = offset,  // 缓冲区中的偏移量（KTX 图像数据位置）
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,  // 图像方面（颜色）
					.mipLevel = i,  // Mip 级别
					.baseArrayLayer = 0,  // 基础数组层
					.layerCount = 1,  // 层数量（2D 纹理为 1）
				},
				.imageExtent = {
					.width = std::max(1u, ktxTexture->baseWidth >> i),  // Mip 级别 i 的宽度（右移 i 位，最小为 1）
					.height = std::max(1u, ktxTexture->baseHeight >> i),  // Mip 级别 i 的高度（右移 i 位，最小为 1）
					.depth = 1  // 深度（2D 纹理为 1）
				}
			};
			bufferCopyRegions.push_back(bufferCopyRegion);  // 添加到复制区域列表
		}

		// Create optimal tiled target image
		// 创建最优平铺的目标图像
		// 最优平铺提供最佳 GPU 访问性能，但 CPU 无法直接访问
		VkImageCreateInfo imageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // 结构体类型
			.imageType = VK_IMAGE_TYPE_2D,  // 图像类型（2D）
			.format = format,  // 图像格式
			.extent = {.width = width, .height = height, .depth = 1 },  // 图像尺寸（宽度、高度、深度）
			.mipLevels = mipLevels,  // Mip 级别数量
			.arrayLayers = 1,  // 数组层数量（2D 纹理为 1）
			.samples = VK_SAMPLE_COUNT_1_BIT,  // 采样数（非多重采样）
			.tiling = VK_IMAGE_TILING_OPTIMAL,  // 平铺模式（最优平铺，GPU 优化布局）
			.usage = imageUsageFlags,  // 图像用途（由参数指定）
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // 共享模式（独占模式，单队列族使用）
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,  // 初始布局（未定义，将在复制前转换）
		};
		// Ensure that the TRANSFER_DST bit is set for staging
		// 确保为暂存操作设置了 TRANSFER_DST 位
		// 图像必须能够作为传输目标才能接收从缓冲区的数据
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {  // 如果未设置传输目标用途
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // 添加传输目标用途
		}
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));  // 创建图像对象
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);  // 查询图像内存需求
		memAllocInfo.allocationSize = memReqs.size;  // 更新分配大小
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型索引（最佳 GPU 性能）
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));  // 分配设备本地内存
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));  // 将内存绑定到图像对象

		VkImageSubresourceRange subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .layerCount = 1, };  // 图像子资源范围（用于布局转换）

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		// 图像屏障：将最优图像（目标）从未定义布局转换为传输目标布局
		// 最优图像将用作复制的目标
		vks::tools::setImageLayout(
			copyCmd,  // 命令缓冲区
			image,  // 图像对象
			VK_IMAGE_LAYOUT_UNDEFINED,  // 源布局（未定义）
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // 目标布局（传输目标最优）
			subresourceRange);  // 子资源范围

		// Copy mip levels from staging buffer
		// 从暂存缓冲区复制 Mip 级别
		// 这将在 GPU 上执行复制操作，将数据从主机可见缓冲区传输到设备本地图像
		vkCmdCopyBufferToImage(
			copyCmd,  // 命令缓冲区
			stagingBuffer,  // 源缓冲区（暂存缓冲区）
			image,  // 目标图像
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // 图像布局（传输目标最优）
			static_cast<uint32_t>(bufferCopyRegions.size()),  // 复制区域数量
			bufferCopyRegions.data()  // 复制区域数组
		);

		// Change texture image layout to shader read after all mip levels have been copied
		// 在所有 Mip 级别复制完成后，将纹理图像布局更改为着色器读取布局
		// 这确保图像在着色器中可以正确访问
		this->imageLayout = imageLayout;  // 保存图像布局
		vks::tools::setImageLayout(
			copyCmd,  // 命令缓冲区
			image,  // 图像对象
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // 源布局（传输目标最优）
			imageLayout,  // 目标布局（着色器读取最优，由参数指定）
			subresourceRange);  // 子资源范围

		device->flushCommandBuffer(copyCmd, copyQueue);  // 刷新命令缓冲区（提交并等待完成）

		// Clean up staging resources
		// 清理暂存资源
		// 暂存缓冲区和内存不再需要，可以释放
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);  // 销毁暂存缓冲区
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);  // 释放暂存内存

		ktxTexture_Destroy(ktxTexture);  // 销毁 KTX 纹理对象

		// Create a default sampler
		// 创建默认采样器
		// 采样器定义如何从纹理中读取像素（过滤、寻址模式等）
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,  // 结构体类型
			.magFilter = VK_FILTER_LINEAR,  // 放大过滤（线性过滤，平滑放大）
			.minFilter = VK_FILTER_LINEAR,  // 缩小过滤（线性过滤，平滑缩小）
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,  // Mipmap 模式（线性插值）
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,  // U 方向寻址模式（重复）
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,  // V 方向寻址模式（重复）
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,  // W 方向寻址模式（重复）
			.mipLodBias = 0.0f,  // Mip LOD 偏移（无偏移）
			.anisotropyEnable = device->enabledFeatures.samplerAnisotropy,  // 是否启用各向异性过滤（如果设备支持）
			.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f,  // 最大各向异性级别（如果启用则使用设备最大值）
			.compareOp = VK_COMPARE_OP_NEVER,  // 比较操作（不进行比较）
			.minLod = 0.0f,  // 最小 LOD（细节级别）
			.maxLod = (float)mipLevels,  // 最大 LOD（所有 Mip 级别）
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE  // 边界颜色（不透明白色）
		};
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));  // 创建采样器

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		// 创建图像视图
		// 着色器不直接访问纹理，而是通过包含附加信息和子资源范围的图像视图来抽象
		VkImageViewCreateInfo viewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // 结构体类型
			.image = image,  // 要创建视图的图像
			.viewType = VK_IMAGE_VIEW_TYPE_2D,  // 视图类型（2D 图像）
			.format = format,  // 视图格式（与图像格式相同）
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1 },  // 子资源范围（颜色方面，所有 Mip 级别，单层）
		};
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));  // 创建图像视图

		// Update descriptor image info member that can be used for setting up descriptor sets
		// 更新描述符图像信息成员，可用于设置描述符集
		updateDescriptor();  // 更新描述符（采样器、图像视图、布局）
	}

	/**
	* Creates a 2D texture from a buffer
	*
	* @param buffer Buffer containing texture data to upload
	* @param bufferSize Size of the buffer in machine units
	* @param width Width of the texture to create
	* @param height Height of the texture to create
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) filter Texture filtering for the sampler (defaults to VK_FILTER_LINEAR)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*/
	/**
	 * @brief 从缓冲区创建 2D 纹理
	 * 从内存缓冲区创建 2D 纹理，包括创建暂存缓冲区、设备本地图像、复制数据等步骤
	 * 
	 * @param buffer 包含纹理数据的缓冲区指针
	 * @param bufferSize 缓冲区大小（字节）
	 * @param format 图像数据的 Vulkan 格式
	 * @param texWidth 要创建的纹理宽度（像素）
	 * @param texHeight 要创建的纹理高度（像素）
	 * @param device 要在其上创建纹理的 Vulkan 设备指针
	 * @param copyQueue 用于纹理暂存复制命令的队列（必须支持传输）
	 * @param filter 采样器的过滤模式（默认 VK_FILTER_LINEAR）
	 * @param imageUsageFlags 纹理图像的使用标志（默认 VK_IMAGE_USAGE_SAMPLED_BIT）
	 * @param imageLayout 纹理的使用布局（默认 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL）
	 */
	void Texture2D::fromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, vks::VulkanDevice *device, VkQueue copyQueue, VkFilter filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		assert(buffer);  // 确保缓冲区有效

		// 设置设备指针和纹理尺寸
		this->device = device;
		width = texWidth;
		height = texHeight;
		mipLevels = 1;  // 单 Mip 级别

		VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// 创建包含原始图像数据的主机可见暂存缓冲区
		// 暂存缓冲区用于将数据从主机内存传输到设备本地内存
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
		bufferCreateInfo.size = bufferSize;
		// 此缓冲区用作缓冲区复制的传输源
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // 独占模式

		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		// 获取暂存缓冲区的内存需求（对齐、内存类型位等）
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		// 获取主机可见缓冲区的内存类型索引
		// 主机可见和一致性内存允许 CPU 直接访问
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// 分配暂存缓冲区内存
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		// 将内存绑定到暂存缓冲区
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));

		// 将纹理数据复制到暂存缓冲区
		uint8_t *data{ nullptr };
		// 映射内存到主机可访问的地址空间
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		// 将数据从输入缓冲区复制到映射的内存
		memcpy(data, buffer, bufferSize);
		// 取消映射内存（一致性内存会自动刷新）
		vkUnmapMemory(device->logicalDevice, stagingMemory);

		// 定义缓冲区到图像的复制区域
		// 指定从缓冲区的哪个位置复制到图像的哪个子资源
		VkBufferImageCopy bufferCopyRegion{
			.bufferOffset = 0,  // 缓冲区中的偏移量
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,  // 图像方面（颜色）
				.mipLevel = 0,                              // Mip 级别
				.baseArrayLayer = 0,                        // 基础数组层
				.layerCount = 1                             // 层数量
			},
			.imageExtent = {
				.width = width,   // 图像宽度
				.height = height, // 图像高度
				.depth = 1,       // 图像深度（2D 图像为 1）
			}
		};

		// 创建最优平铺的目标图像
		// 最优平铺提供最佳性能，但 CPU 无法直接访问
		VkImageCreateInfo imageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,                    // 2D 图像
			.format = format,                                 // 图像格式
			.extent = {.width = width, .height = height, .depth = 1 }, // 图像尺寸
			.mipLevels = mipLevels,                           // Mip 级别数量
			.arrayLayers = 1,                                 // 数组层数量
			.samples = VK_SAMPLE_COUNT_1_BIT,                 // 采样数（非多重采样）
			.tiling = VK_IMAGE_TILING_OPTIMAL,                // 最优平铺（GPU 优化布局）
			.usage = imageUsageFlags,                         // 图像用途
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,         // 独占模式
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED       // 初始布局（未定义）
		};
		// 确保为暂存操作设置了 TRANSFER_DST 位
		// 图像必须能够作为传输目标才能接收从缓冲区的数据
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		// 创建图像对象
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));

		// 查询图像的内存需求
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;

		// 获取设备本地内存的内存类型索引
		// 设备本地内存提供最佳 GPU 访问性能
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		// 分配设备本地内存
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		// 将内存绑定到图像对象
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));

		// 定义图像子资源范围（用于布局转换）
		VkImageSubresourceRange subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .layerCount = 1 };

		// 使用单独的命令缓冲区进行纹理加载
		// 这样可以独立执行纹理加载操作，不影响主渲染命令
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		// 图像屏障：将最优图像（目标）从未定义布局转换为传输目标布局
		// 最优图像将用作复制的目标
		vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
		// 从暂存缓冲区复制 Mip 级别到图像
		// 这将在 GPU 上执行复制操作
		vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
		// 在所有 Mip 级别复制完成后，将纹理图像布局更改为着色器读取布局
		this->imageLayout = imageLayout;
		vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);
		// 刷新命令缓冲区（提交并等待完成）
		device->flushCommandBuffer(copyCmd, copyQueue);

		// 清理暂存资源
		// 暂存缓冲区和内存不再需要，可以释放
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);

		// Create sampler
		// 创建采样器（从缓冲区创建的纹理使用指定的过滤模式）
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,  // 结构体类型
			.magFilter = filter,  // 放大过滤（由参数指定）
			.minFilter = filter,  // 缩小过滤（由参数指定）
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,  // Mipmap 模式（线性，单 Mip 级别时不影响）
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,  // U 方向寻址模式（重复）
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,  // V 方向寻址模式（重复）
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,  // W 方向寻址模式（重复）
			.mipLodBias = 0.0f,  // Mip LOD 偏移
			.maxAnisotropy = 1.0f,  // 最大各向异性级别（禁用各向异性过滤）
			.compareOp = VK_COMPARE_OP_NEVER,  // 比较操作（不进行比较）
			.minLod = 0.0f,  // 最小 LOD
			.maxLod = 0.0f,  // 最大 LOD（单 Mip 级别，仅使用级别 0）
		};
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));  // 创建采样器

		// Create image view
		// 创建图像视图（2D 纹理视图，单 Mip 级别）
		VkImageViewCreateInfo viewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // 结构体类型
			.image = image,  // 要创建视图的图像
			.viewType = VK_IMAGE_VIEW_TYPE_2D,  // 视图类型（2D 图像）
			.format = format,  // 视图格式
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 },  // 子资源范围（颜色方面，单 Mip 级别，单层）
		};
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));  // 创建图像视图

		// Update descriptor image info member that can be used for setting up descriptor sets
		// 更新描述符图像信息成员，可用于设置描述符集
		updateDescriptor();  // 更新描述符
	}

	/**
	* Load a 2D texture array including all mip levels
	*
	* @param filename File to load (supports .ktx)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*
	*/
	void Texture2DArray::loadFromFile(std::string filename, VkFormat format, vks::VulkanDevice *device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);

		this->device = device;  // 保存设备指针
		width = ktxTexture->baseWidth;  // 获取纹理宽度
		height = ktxTexture->baseHeight;  // 获取纹理高度
		layerCount = ktxTexture->numLayers;  // 获取数组层数量
		mipLevels = ktxTexture->numLevels;  // 获取 Mip 级别数量

		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);  // 获取 KTX 纹理数据指针
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);  // 获取 KTX 纹理数据大小

		// Create a host-visible staging buffer that contains the raw image data
		// 创建包含原始图像数据的主机可见暂存缓冲区
		VkBuffer stagingBuffer;  // 暂存缓冲区
		VkDeviceMemory stagingMemory;  // 暂存内存

		VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();  // 初始化缓冲区创建信息
		bufferCreateInfo.size = ktxTextureSize;  // 缓冲区大小（KTX 纹理数据大小）
		// This buffer is used as a transfer source for the buffer copy
		// 此缓冲区用作缓冲区复制的传输源
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  // 用途：传输源
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // 共享模式：独占

		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));  // 创建暂存缓冲区

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		// 获取暂存缓冲区的内存需求（对齐、内存类型位等）
		VkMemoryRequirements memReqs;  // 内存需求
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);  // 查询缓冲区内存需求
		VkMemoryAllocateInfo memAllocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // 结构体类型
			.allocationSize = memReqs.size,  // 分配大小
			.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)  // 主机可见和一致性内存类型
		};
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));  // 分配暂存内存
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));  // 将内存绑定到缓冲区

		// Copy texture data into staging buffer
		// 将纹理数据复制到暂存缓冲区
		uint8_t *data{ nullptr };  // 映射的内存指针
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));  // 映射内存
		memcpy(data, ktxTextureData, ktxTextureSize);  // 复制 KTX 纹理数据
		vkUnmapMemory(device->logicalDevice, stagingMemory);  // 取消映射内存

		// Setup buffer copy regions for each layer including all of its miplevels
		// 为每个层（包括其所有 Mip 级别）设置缓冲区复制区域
		std::vector<VkBufferImageCopy> bufferCopyRegions;  // 缓冲区到图像复制区域向量

		for (uint32_t layer = 0; layer < layerCount; layer++) {  // 遍历所有数组层
			for (uint32_t level = 0; level < mipLevels; level++) {  // 遍历该层的所有 Mip 级别
				ktx_size_t offset;  // KTX 图像在数据中的偏移量
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, layer, 0, &offset);  // 获取 Mip 级别 level、层 layer 的图像偏移量
				assert(result == KTX_SUCCESS);  // 确保获取成功
				VkBufferImageCopy bufferCopyRegion{
					.bufferOffset = offset,  // 缓冲区中的偏移量
					.imageSubresource {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,  // 图像方面（颜色）
						.mipLevel = level,  // Mip 级别
						.baseArrayLayer = layer,  // 基础数组层
						.layerCount = 1,  // 层数量
					},
					.imageExtent {
						.width = ktxTexture->baseWidth >> level,  // Mip 级别 level 的宽度（右移 level 位）
						.height = ktxTexture->baseHeight >> level,  // Mip 级别 level 的高度（右移 level 位）
						.depth = 1,  // 深度（2D 纹理为 1）
					}
				};
				bufferCopyRegions.push_back(bufferCopyRegion);  // 添加到复制区域列表
			}
		}

		// Create optimal tiled target image
		// 创建最优平铺的目标图像（2D 纹理数组）
		VkImageCreateInfo imageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // 结构体类型
			.imageType = VK_IMAGE_TYPE_2D,  // 图像类型（2D）
			.format = format,  // 图像格式
			.extent = {.width = width, .height = height, .depth = 1 },  // 图像尺寸
			.mipLevels = mipLevels,  // Mip 级别数量
			.arrayLayers = layerCount,  // 数组层数量（纹理数组的层数）
			.samples = VK_SAMPLE_COUNT_1_BIT,  // 采样数（非多重采样）
			.tiling = VK_IMAGE_TILING_OPTIMAL,  // 平铺模式（最优平铺）
			.usage = imageUsageFlags,  // 图像用途
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // 共享模式（独占）
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,  // 初始布局（未定义）
		};
		// Ensure that the TRANSFER_DST bit is set for staging
		// 确保为暂存操作设置了 TRANSFER_DST 位
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {  // 如果未设置传输目标用途
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // 添加传输目标用途
		}
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));  // 创建图像对象

		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);  // 查询图像内存需求
		memAllocInfo.allocationSize = memReqs.size;  // 更新分配大小
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));  // 分配设备本地内存
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));  // 将内存绑定到图像

		// Use a separate command buffer for texture loading
		// 使用单独的命令缓冲区进行纹理加载
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);  // 创建主命令缓冲区
		// Image barrier for optimal image (target)
		// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		// 图像屏障：为最优图像（目标）的所有数组层设置初始布局
		VkImageSubresourceRange subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .layerCount = layerCount };  // 子资源范围（所有 Mip 级别和所有层）
		vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);  // 转换为传输目标布局
		// Copy the layers and mip levels from the staging buffer to the optimal tiled image
		// 从暂存缓冲区复制层和 Mip 级别到最优平铺图像
		vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());  // 执行复制
		// Change texture image layout to shader read after all faces have been copied
		// 在所有层复制完成后，将纹理图像布局更改为着色器读取布局
		this->imageLayout = imageLayout;  // 保存图像布局
		vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);  // 转换为着色器读取布局
		device->flushCommandBuffer(copyCmd, copyQueue);  // 刷新命令缓冲区

		// Create sampler
		// 创建采样器（2D 纹理数组使用边缘夹紧模式）
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,  // 结构体类型
			.magFilter = VK_FILTER_LINEAR,  // 放大过滤（线性）
			.minFilter = VK_FILTER_LINEAR,  // 缩小过滤（线性）
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,  // Mipmap 模式（线性）
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,  // U 方向寻址模式（边缘夹紧，避免层间混合）
			.addressModeV = samplerCreateInfo.addressModeU,  // V 方向寻址模式（与 U 相同）
			.addressModeW = samplerCreateInfo.addressModeU,  // W 方向寻址模式（与 U 相同）
			.mipLodBias = 0.0f,  // Mip LOD 偏移
			.anisotropyEnable = device->enabledFeatures.samplerAnisotropy,  // 是否启用各向异性过滤
			.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f,  // 最大各向异性级别
			.compareOp = VK_COMPARE_OP_NEVER,  // 比较操作（不进行比较）
			.minLod = 0.0f,  // 最小 LOD
			.maxLod = (float)mipLevels,  // 最大 LOD（所有 Mip 级别）
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,  // 边界颜色
		};
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));  // 创建采样器

		// Create image view
		// 创建图像视图（2D 纹理数组视图）
		VkImageViewCreateInfo viewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // 结构体类型
			.image = image,  // 要创建视图的图像
			.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,  // 视图类型（2D 纹理数组）
			.format = format,  // 视图格式
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = layerCount },  // 子资源范围（所有 Mip 级别和所有层）
		};
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));  // 创建图像视图

		// Clean up staging resources
		// 清理暂存资源
		ktxTexture_Destroy(ktxTexture);  // 销毁 KTX 纹理对象
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);  // 销毁暂存缓冲区
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);  // 释放暂存内存

		// Update descriptor image info member that can be used for setting up descriptor sets
		// 更新描述符图像信息成员，可用于设置描述符集
		updateDescriptor();  // 更新描述符
	}

	/**
	* Load a cubemap texture including all mip levels from a single file
	*
	* @param filename File to load (supports .ktx)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*
	*/
	void TextureCubeMap::loadFromFile(std::string filename, VkFormat format, vks::VulkanDevice *device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		ktxTexture* ktxTexture;
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);

		this->device = device;  // 保存设备指针
		width = ktxTexture->baseWidth;  // 获取纹理宽度
		height = ktxTexture->baseHeight;  // 获取纹理高度
		mipLevels = ktxTexture->numLevels;  // 获取 Mip 级别数量

		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);  // 获取 KTX 纹理数据指针
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);  // 获取 KTX 纹理数据大小

		// Create a host-visible staging buffer that contains the raw image data
		// 创建包含原始图像数据的主机可见暂存缓冲区
		VkBuffer stagingBuffer;  // 暂存缓冲区
		VkDeviceMemory stagingMemory;  // 暂存内存

		VkBufferCreateInfo bufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // 结构体类型
			.size = ktxTextureSize,  // 缓冲区大小（KTX 纹理数据大小）
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // 用途：传输源
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE  // 共享模式：独占
		};
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));  // 创建暂存缓冲区

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		// 获取暂存缓冲区的内存需求（对齐、内存类型位等）
		VkMemoryRequirements memReqs;  // 内存需求
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);  // 查询缓冲区内存需求
		VkMemoryAllocateInfo memAllocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // 结构体类型
			.allocationSize = memReqs.size,  // 分配大小
			.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)  // 主机可见和一致性内存类型
		};
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));  // 分配暂存内存
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));  // 将内存绑定到缓冲区

		// Copy texture data into staging buffer
		// 将纹理数据复制到暂存缓冲区
		uint8_t *data{ nullptr };  // 映射的内存指针
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));  // 映射内存
		memcpy(data, ktxTextureData, ktxTextureSize);  // 复制 KTX 纹理数据
		vkUnmapMemory(device->logicalDevice, stagingMemory);  // 取消映射内存

		// Setup buffer copy regions for each face including all of its mip levels
		// 为每个面（包括其所有 Mip 级别）设置缓冲区复制区域
		// 立方体贴图有 6 个面：+X, -X, +Y, -Y, +Z, -Z
		std::vector<VkBufferImageCopy> bufferCopyRegions;  // 缓冲区到图像复制区域向量
		for (uint32_t face = 0; face < 6; face++) {  // 遍历所有立方体贴图面（6 个面）
			for (uint32_t level = 0; level < mipLevels; level++) {  // 遍历该面的所有 Mip 级别
				ktx_size_t offset;  // KTX 图像在数据中的偏移量
				KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);  // 获取 Mip 级别 level、层 0、面 face 的图像偏移量
				assert(result == KTX_SUCCESS);  // 确保获取成功
				VkBufferImageCopy bufferCopyRegion{
					.bufferOffset = offset,  // 缓冲区中的偏移量
					.imageSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,  // 图像方面（颜色）
						.mipLevel = level,  // Mip 级别
						.baseArrayLayer = face,  // 基础数组层（立方体贴图面索引）
						.layerCount = 1  // 层数量（每个面为 1 层）
					},
					.imageExtent = {
						.width = ktxTexture->baseWidth >> level,  // Mip 级别 level 的宽度（右移 level 位）
						.height = ktxTexture->baseHeight >> level,  // Mip 级别 level 的高度（右移 level 位）
						.depth = 1  // 深度（2D 图像为 1）
					},
				};
				bufferCopyRegions.push_back(bufferCopyRegion);  // 添加到复制区域列表
			}
		}

		// Create optimal tiled target image
		// 创建最优平铺的目标图像（立方体贴图）
		VkImageCreateInfo imageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // 结构体类型
			// This flag is required for cube map images
			// 此标志是立方体贴图图像所必需的
			.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,  // 创建标志：立方体兼容（允许将图像用作立方体贴图）
			.imageType = VK_IMAGE_TYPE_2D,  // 图像类型（2D）
			.format = format,  // 图像格式
			.extent = { .width = width, .height = height, .depth = 1 },  // 图像尺寸
			.mipLevels = mipLevels,  // Mip 级别数量
			// Cube faces count as array layers in Vulkan
			// 在 Vulkan 中，立方体贴图面作为数组层计数
			.arrayLayers = 6,  // 数组层数量（立方体贴图有 6 个面）
			.samples = VK_SAMPLE_COUNT_1_BIT,  // 采样数（非多重采样）
			.tiling = VK_IMAGE_TILING_OPTIMAL,  // 平铺模式（最优平铺）
			.usage = imageUsageFlags,  // 图像用途
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // 共享模式（独占）
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,  // 初始布局（未定义）
		};
		// Ensure that the TRANSFER_DST bit is set for staging
		// 确保为暂存操作设置了 TRANSFER_DST 位
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)){  // 如果未设置传输目标用途
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // 添加传输目标用途
		}
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));  // 创建图像对象

		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);  // 查询图像内存需求
		memAllocInfo.allocationSize = memReqs.size;  // 更新分配大小
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));  // 分配设备本地内存
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));  // 将内存绑定到图像

		// Use a separate command buffer for texture loading
		// 使用单独的命令缓冲区进行纹理加载
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);  // 创建主命令缓冲区
		// Image barrier for optimal image (target)
		// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		// 图像屏障：为最优图像（目标）的所有数组层（面）设置初始布局
		VkImageSubresourceRange subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .layerCount = 6 };  // 子资源范围（所有 Mip 级别和所有 6 个面）
		vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);  // 转换为传输目标布局
		// Copy the cube map faces from the staging buffer to the optimal tiled image
		// 从暂存缓冲区复制立方体贴图面到最优平铺图像
		vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());  // 执行复制
		// Change texture image layout to shader read after all faces have been copied
		// 在所有面复制完成后，将纹理图像布局更改为着色器读取布局
		this->imageLayout = imageLayout;  // 保存图像布局
		vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);  // 转换为着色器读取布局
		device->flushCommandBuffer(copyCmd, copyQueue);  // 刷新命令缓冲区

		// Create sampler
		// 创建采样器（立方体贴图使用边缘夹紧模式，避免面间混合）
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,  // 结构体类型
			.magFilter = VK_FILTER_LINEAR,  // 放大过滤（线性）
			.minFilter = VK_FILTER_LINEAR,  // 缩小过滤（线性）
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,  // Mipmap 模式（线性）
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,  // U 方向寻址模式（边缘夹紧，避免面间混合）
			.addressModeV = samplerCreateInfo.addressModeU,  // V 方向寻址模式（与 U 相同）
			.addressModeW = samplerCreateInfo.addressModeU,  // W 方向寻址模式（与 U 相同）
			.mipLodBias = 0.0f,  // Mip LOD 偏移
			.anisotropyEnable = device->enabledFeatures.samplerAnisotropy,  // 是否启用各向异性过滤
			.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f,  // 最大各向异性级别
			.compareOp = VK_COMPARE_OP_NEVER,  // 比较操作（不进行比较）
			.minLod = 0.0f,  // 最小 LOD
			.maxLod = (float)mipLevels,  // 最大 LOD（所有 Mip 级别）
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE  // 边界颜色
		};
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerCreateInfo, nullptr, &sampler));  // 创建采样器

		// Create image view
		// 创建图像视图（立方体贴图视图）
		VkImageViewCreateInfo viewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // 结构体类型
			.image = image,  // 要创建视图的图像
			.viewType = VK_IMAGE_VIEW_TYPE_CUBE,  // 视图类型（立方体贴图）
			.format = format,  // 视图格式
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 6 },  // 子资源范围（所有 Mip 级别和所有 6 个面）
		};
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));  // 创建图像视图

		// Clean up staging resources
		// 清理暂存资源
		ktxTexture_Destroy(ktxTexture);  // 销毁 KTX 纹理对象
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);  // 销毁暂存缓冲区
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);  // 释放暂存内存

		// Update descriptor image info member that can be used for setting up descriptor sets
		// 更新描述符图像信息成员，可用于设置描述符集
		updateDescriptor();  // 更新描述符
	}

}
