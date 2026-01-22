/*
* Vulkan Example - Texture loading (and display) example (including mip maps)
* 
* This sample shows how to upload a 2D texture to the device and how to display it. In Vulkan this is done using images, views and samplers.
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
/*
* Vulkan 示例 - 纹理加载（以及显示）示例（包含 mip 贴图）
*
* 本示例展示如何将一张 2D 纹理上传到 GPU 并显示出来。在 Vulkan 中，这是通过 image、image view 与 sampler 来完成的。
*
* 版权 (C) 2016-2025 归 Sascha Willems 所有 - www.saschawillems.de
*
* 本代码遵循 MIT 许可证 (MIT) 授权 (http://opensource.org/licenses/MIT)。
*/

#include "vulkanexamplebase.h"
#include <ktx.h>
#include <ktxvulkan.h>

// Vertex layout for this example
// 本示例所使用的顶点数据布局
struct Vertex {
	float pos[3];
	float uv[2];
	float normal[3];
};

class VulkanExample : public VulkanExampleBase
{
public:
	// Contains all Vulkan objects that are required to store and use a texture
	// Note that this repository contains a texture class (VulkanTexture.hpp) that encapsulates texture loading functionality in a class that is used in subsequent demos
	// 该结构体包含了存储与使用纹理所需的全部 Vulkan 对象
	// 注意：本仓库还提供了一个纹理类（VulkanTexture.hpp），将纹理加载功能封装在类中，后续示例会直接复用该类
	struct Texture {
		VkSampler sampler{ VK_NULL_HANDLE };
		VkImage image{ VK_NULL_HANDLE };
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory{ VK_NULL_HANDLE };
		VkImageView view{ VK_NULL_HANDLE };
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t mipLevels{ 0 };
	} texture;

	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount{ 0 };

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 viewPos;
		// This is used to change the bias for the level-of-detail (mips) in the fragment shader
		// 该参数用于在片元着色器中调整 LOD（mip 级别）的偏移量
		float lodBias = 0.0f;
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	VulkanExample() : VulkanExampleBase()
	{
		title = "Texture loading";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f, 15.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample()
	{
		if (device) {
			destroyTextureImage(texture);
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			vertexBuffer.destroy();
			indexBuffer.destroy();
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
		}
	}

	// Enable physical device features required for this example
	// 启用本示例所需的物理设备特性
	virtual void getEnabledFeatures()
	{
		// Enable anisotropic filtering if supported
		// 如果设备支持各向异性过滤，则在此启用
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	/*
		Upload texture image data to the GPU

		Vulkan offers two types of image tiling (memory layout):

		Linear tiled images:
			These are stored as is and can be copied directly to. But due to the linear nature they're not a good match for GPUs and format and feature support is very limited.
			It's not advised to use linear tiled images for anything else than copying from host to GPU if buffer copies are not an option.
			Linear tiling is thus only implemented for learning purposes, one should always prefer optimal tiled image.

		Optimal tiled images:
			These are stored in an implementation specific layout matching the capability of the hardware. They usually support more formats and features and are much faster.
			Optimal tiled images are stored on the device and not accessible by the host. So they can't be written directly to (like liner tiled images) and always require
			some sort of data copy, either from a buffer or	a linear tiled image.

		In Short: Always use optimal tiled images for rendering.
	*/
	/*
		将纹理图像数据上传至 GPU

		Vulkan 提供两种图像平铺（内存布局）方式：

		线性平铺图像（Linear tiled images）：
			这类图像按原始形式存储，可直接向其执行拷贝操作。但由于其线性存储特性，它们与 GPU 的适配性不佳，
			且格式和功能支持非常有限。
			除非无法使用缓冲区拷贝，否则不建议将线性平铺图像用于除“从主机（CPU）向 GPU 拷贝数据”之外的任何场景。
			因此实现线性平铺仅作为学习用途，在实际开发中应始终优先使用最优平铺图像。

		最优平铺图像（Optimal tiled images）：
			这类图像以适配硬件能力的、由驱动实现定义的布局存储。它们通常支持更多格式和功能，且性能表现远更出色。
			最优平铺图像存储在设备端（GPU），主机（CPU）无法直接访问。因此无法像线性平铺图像那样直接写入数据，
			必须通过某种数据拷贝方式（从缓冲区或线性平铺图像）完成数据传输。

		简而言之：渲染场景下应始终使用最优平铺图像。
	*/
	void loadTexture()
	{
		// We use the Khronos texture format (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
		// 使用 Khronos 的 KTX 纹理格式（https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/）
		std::string filename = getAssetPath() + "textures/metalplate01_rgba.ktx";
		// Texture data contains 4 channels (RGBA) with unnormalized 8-bit values, this is the most commonly supported format
		// 纹理数据包含 4 个通道（RGBA），每个通道为未归一化的 8 位整型，这是支持最广泛的一种纹理格式
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		ktxResult result;
		ktxTexture* ktxTexture;

#if defined(__ANDROID__)
		// Textures are stored inside the apk on Android (compressed)
		// So they need to be loaded via the asset manager
		// 在 Android 平台上纹理数据被压缩打包在 APK 内
		// 因此需要通过 AssetManager 来加载
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		if (!asset) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		ktx_uint8_t *textureData = new ktx_uint8_t[size];
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);
		result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		delete[] textureData;
#else
		if (!vks::tools::fileExists(filename)) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
#endif
		assert(result == KTX_SUCCESS);

		// Get properties required for using and upload texture data from the ktx texture object
		// 从 ktx 纹理对象中获取上传纹理数据及使用时所需的属性信息
		texture.width = ktxTexture->baseWidth;          //这是mipleve=0 也就是最大的图片的宽度  
		texture.height = ktxTexture->baseHeight;        //这是mipleve=0 也就是最大的图片的高度 
		texture.mipLevels = ktxTexture->numLevels;
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);    //获取像素数据起始指针（对应之前讲的 mip 数据区域）
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);  // 获取像素数据总字节数

		// We prefer using staging to copy the texture data to a device local optimal image
		// 推荐通过中转缓冲区（staging buffer）把纹理数据拷贝到设备本地的 optimal image
		VkBool32 useStaging = true;

		// Only use linear tiling if forced
		// 只有在“被强制要求”的情况下才考虑使用线性平铺
		bool forceLinearTiling = false;
		if (forceLinearTiling) {
			// Don't use linear if format is not supported for (linear) shader sampling
			// Get device properties for the requested texture format
			// 如果该格式在（线性平铺模式下）不支持着色器采样，则不要使用线性平铺
			// 这里查询物理设备对指定纹理格式的属性支持情况
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
			useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}

		VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs = {};

		if (useStaging) {
			// Copy data to an optimal tiled image
			// This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device
			// 将数据拷贝到 optimal tiled 图像中
			// 先把纹理数据加载到主机本地缓冲区，再从该缓冲区拷贝到设备本地的 optimal image

			// Create a host-visible staging buffer that contains the raw image data
			// This buffer will be the data source for copying texture data to the optimal tiled image on the device
		

// ------------------------------------------------------1.创建一个主机可见的 staging buffer----------------------------------------------------------------------
			// 1.创建一个主机可见的 staging buffer，用于存放原始纹理数据
			// 后续会以该缓冲区作为数据源，将纹理数据拷贝到设备本地图像
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
			bufferCreateInfo.size = ktxTextureSize;
			// This buffer is used as a transfer source for the buffer copy
			// 该缓冲区将作为传输源，在复制操作中提供数据
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));

			// Get memory requirements for the staging buffer (alignment, memory type bits)
			// 查询 staging 缓冲区的内存需求（对齐方式、内存类型掩码等）
			vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type index for a host visible buffer
			// 选择一个支持主机可见属性的内存类型索引
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

			// Copy texture data into host local staging buffer
			// 将纹理数据拷贝到主机本地的 staging 缓冲区  // 3. 将KTX像素数据拷贝到Staging Buffer
			uint8_t *data;
			VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
			memcpy(data, ktxTextureData, ktxTextureSize);
			vkUnmapMemory(device, stagingMemory);

			// Setup buffer copy regions for each mip level
			// 为每个 mip 级别配置对应的缓冲区拷贝区域   完整的内存块中存储了好几个mip等级的图片
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			uint32_t offset = 0;

			//这里就是告诉vulkan 所有miplevel中图片在gpu那边该怎么分布
			for (uint32_t i = 0; i < texture.mipLevels; i++) {
				// Calculate offset into staging buffer for the current mip level
				// 计算当前 mip 级别在 staging 缓冲区中的偏移位置
				ktx_size_t offset;
				KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
				assert(ret == KTX_SUCCESS);
				// Setup a buffer image copy structure for the current mip level
				// 为当前 mip 级别创建一个 VkBufferImageCopy 结构体
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> i;
				bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> i;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset; // 当前mip在Staging中的起始偏移
				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			// Create optimal tiled target image on the device
			// 在设备端创建一张 optimal tiled 的目标图像，用于接收纹理数据
			VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = texture.mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;    // 无多重采样
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// Set initial layout of the image to undefined
			//将图像的初始布局设置为未定义
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			//extent填texture.width，就是告诉 Vulkan：“我要创建一个以 1024×1024 为基础尺寸的纹理，后续会包含它的所有缩小版 mip 层级”。
			imageCreateInfo.extent = { texture.width, texture.height, 1 };   
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image));

			vkGetImageMemoryRequirements(device, texture.image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device, texture.image, texture.deviceMemory, 0));

			VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// Image memory barriers for the texture image
			// 为纹理图像设置图像内存屏障（Image Memory Barrier）管线屏障的作用：保证内存访问顺序，确保布局转换完成后再执行拷贝。

			// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			// subresourceRange 描述了后续将通过内存屏障进行布局转换的图像子资源范围
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			// 图像只包含颜色数据
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			// 从第 0 个 mip 级别开始
			subresourceRange.baseMipLevel = 0;
			// We will transition on all mip levels
			// 会对所有 mip 级别执行布局转换
			subresourceRange.levelCount = texture.mipLevels;
			// The 2D texture only has one layer
			// 该 2D 纹理只有一个图层（array layer）
			subresourceRange.layerCount = 1;

			// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
			// 先将纹理图像布局转换为传输目标布局，以便安全地从缓冲区向其拷贝数据
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();;
			imageMemoryBarrier.image = texture.image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			// 在合适的管线阶段插入内存依赖来执行布局转换
			// 源阶段为主机读写（VK_PIPELINE_STAGE_HOST_BIT）
			// 目标阶段为传输命令执行（VK_PIPELINE_STAGE_TRANSFER_BIT）
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			// Copy mip levels from staging buffer
			// 将各 mip 级别的数据从 staging 缓冲区拷贝到图像中
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				texture.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data());

			// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
			// 数据上传完毕后，将纹理图像的布局转换为着色器只读布局，以便在着色器中采样
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			// 同样通过内存依赖在合适的管线阶段执行布局切换
			// 源阶段为传输命令执行（VK_PIPELINE_STAGE_TRANSFER_BIT）
			// 目标阶段为片元着色器访问阶段（VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT）
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			// Store current layout for later reuse
			// 记录当前图像布局，以便后续使用
			texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

			// Clean up staging resources
			// 清理 staging 相关的临时资源
			vkFreeMemory(device, stagingMemory, nullptr);
			vkDestroyBuffer(device, stagingBuffer, nullptr);
		} else {
			// Copy data to a linear tiled image
			// 将数据拷贝到一张线性平铺的图像中

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			// Load mip map level 0 to linear tiling image
			// 仅将 mip 0 级数据加载到线性平铺图像中
			VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
			imageCreateInfo.extent = { texture.width, texture.height, 1 };
			VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage));

			// Get memory requirements for this image like size and alignment
			// 查询该图像的内存需求，例如大小与对齐方式
			vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			// 将分配大小设置为图像所需的内存大小
			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type that can be mapped to host memory
			// 选择一种可映射到主机内存的内存类型
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &mappableMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device, mappableImage, mappableMemory, 0));

			// Map image memory
			// 映射图像内存到主机地址空间
			void *data;
			VK_CHECK_RESULT(vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data));
			// Copy image data of the first mip level into memory
			// 将第一个 mip 级别的数据拷贝到映射内存中
			memcpy(data, ktxTextureData, memReqs.size);
			vkUnmapMemory(device, mappableMemory);

			// Linear tiled images don't need to be staged and can be directly used as textures
			// 线性平铺图像不需要中转缓冲，可以被直接当作纹理来使用
			texture.image = mappableImage;
			texture.deviceMemory = mappableMemory;
			texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Setup image memory barrier transfer image to shader read layout
			// 设置图像内存屏障以将图像布局转换为着色器可读布局
			VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// The sub resource range describes the regions of the image we will be transition
			// subresourceRange 描述了将进行布局转换的图像子资源范围
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			// Transition the texture image layout to shader read, so it can be sampled from
			// 将纹理图像布局转换到着色器只读布局，以便可以在着色器中进行采样
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();;
			imageMemoryBarrier.image = texture.image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			// 在适当的管线阶段插入内存依赖以执行布局转换
			// 源阶段为主机读写（VK_PIPELINE_STAGE_HOST_BIT）
			// 目标阶段为片元着色器访问（VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT）
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			vulkanDevice->flushCommandBuffer(copyCmd, queue, true);
		}

		ktxTexture_Destroy(ktxTexture);

		// Create a texture sampler
		// In Vulkan textures are accessed by samplers
		// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
		// Note: Similar to the samplers available with OpenGL 3.3
		// 创建纹理采样器
		// 在 Vulkan 中，纹理是通过 sampler 来访问的
		// 采样参数与纹理数据被解耦，你可以为同一张纹理创建多个 sampler，分别使用不同的采样配置
		// 注意：这一点与 OpenGL 3.3 中的独立 sampler 对象类似
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		// Set max level-of-detail to mip level count of the texture
		// 将最大 LOD 设置为纹理的 mip 级别数量
		sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device
		// 启用各向异性过滤
		// 该特性是可选的，因此需要先检查设备是否支持
		if (vulkanDevice->features.samplerAnisotropy) {
			// Use max. level of anisotropy for this example
			// 在本示例中直接使用设备支持的最大各向异性等级
			sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
			sampler.anisotropyEnable = VK_TRUE;
		} else {
			// The device does not support anisotropic filtering
			// 如果设备不支持各向异性过滤，就退化为 1.0（相当于关闭）
			sampler.maxAnisotropy = 1.0;
			sampler.anisotropyEnable = VK_FALSE;
		}
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		// 创建图像视图
		// 着色器并不会直接访问图像本体，而是通过 image view 来访问
		// image view 还包含额外的信息以及子资源范围
		VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		// subresourceRange 描述了通过该 image view 可访问的一组 mip 级别（以及数组层）
		// 对同一张图像可以创建多个 image view，分别引用图像的不同（甚至重叠）子范围
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		// 线性平铺通常不支持 mipmap
		// 因此只有在使用 optimal tiled 图像时才设置多级 mip 计数
		view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
		// The view will be based on the texture's image
		// 该 image view 基于上面创建好的纹理图像
		view.image = texture.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));
	}

	// Free all Vulkan resources used by a texture object
	// 释放纹理对象所使用的所有 Vulkan 资源
	void destroyTextureImage(Texture texture)
	{
		vkDestroyImageView(device, texture.view, nullptr);
		vkDestroyImage(device, texture.image, nullptr);
		vkDestroySampler(device, texture.sampler, nullptr);
		vkFreeMemory(device, texture.deviceMemory, nullptr);
	}
	
	// Creates a vertex and index buffer for a quad made of two triangles
	// This is used to display the texture on
	// 创建一个由两个三角形组成的矩形的顶点缓冲与索引缓冲
	// 我们会在这个矩形上绘制纹理
	void generateQuad()
	{
		// Setup vertices for a single uv-mapped quad made from two triangles
		// 为一个带 UV 映射的矩形（由两个三角形组成）设置顶点数据
		std::vector<Vertex> vertices =
		{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
			{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
		};

		// Setup indices
		// 设置索引缓冲
		std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
		indexCount = static_cast<uint32_t>(indices.size());

		// Create buffers and upload data to the GPU
		// 创建缓冲区并将数据上传到 GPU
		struct StagingBuffers {
			vks::Buffer vertices;
			vks::Buffer indices;
		} stagingBuffers;

		// Host visible source buffers (staging)
		// 主机可见的源缓冲区（staging 缓冲）
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffers.vertices, vertices.size() * sizeof(Vertex), vertices.data()));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffers.indices, indices.size() * sizeof(uint32_t), indices.data()));

		// Device local destination buffers
		// 设备本地目标缓冲区
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, vertices.size() * sizeof(Vertex)));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, indices.size() * sizeof(uint32_t)));

		// Copy from host do device
		// 将数据从主机侧拷贝到设备本地缓冲区
		vulkanDevice->copyBuffer(&stagingBuffers.vertices, &vertexBuffer, queue);
		vulkanDevice->copyBuffer(&stagingBuffers.indices, &indexBuffer, queue);

		// Clean up
		// 清理中转缓冲区资源
		stagingBuffers.vertices.destroy();
		stagingBuffers.indices.destroy();
	}

	void setupDescriptors()
	{
		// Pool
		// 描述符池配置
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames),
			// The sample uses a combined image + sampler descriptor to sample the texture in the fragment shader
			// We need multiple descriptors (NOT images) due to how we set up the descriptor bindings in this sample
			// 本示例在片元着色器中使用“图像 + 采样器”组合描述符（combined image sampler）来采样纹理
			// 由于本示例的绑定方式，我们需要多个描述符对象（而不是多份纹理图像）
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		// 管线布局（指定着色器可以访问哪些描述符集）
		// 描述符布局（描述每个绑定点使用何种资源）
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			// 绑定点 0：顶点着色器使用的 uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1 : Fragment shader image sampler
			// 绑定点 1：片元着色器使用的图像采样器
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Setup a descriptor image info for the current texture to be used as a descriptor for a combined image sampler
		// 为当前纹理构造 VkDescriptorImageInfo，以便作为 combined image sampler 描述符使用
		VkDescriptorImageInfo textureDescriptor{};
		// The image's view (images are never directly accessed by the shader, but rather through views defining subresources)
		// 图像视图（着色器不会直接访问图像本身，而是通过 image view 来访问图像的子资源）
		textureDescriptor.imageView = texture.view;
		// The sampler (Telling the pipeline how to sample the texture, including repeat, border, etc.)
		// 采样器（告诉管线如何采样纹理，包括寻址模式、边界颜色等）
		textureDescriptor.sampler = texture.sampler;
		// The current layout of the image(Note: Should always fit the actual use, e.g.shader read)
		// 图像当前的布局（注意：应始终与实际使用方式相匹配，例如用于着色器读取）
		textureDescriptor.imageLayout = texture.imageLayout;

		// Sets per frame, just like the buffers themselves
		// 描述符集与缓冲区一样，为每帧单独分配一份
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));

			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0 : Vertex shader uniform buffer
				// 绑定点 0：顶点着色器的 uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
				// Binding 1 : Fragment shader texture sampler
				//  Note that unlike the uniform buffer, which is written by the CPU and the GPU, the image (texture) is a static resource
				//  As such we can use the same image for every frame in flight
				//	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
				// 绑定点 1：片元着色器的纹理采样器
				//  与 CPU/GPU 会频繁更新的 uniform buffer 不同，纹理图像通常是静态资源
				//  因此多帧并行渲染时可以复用同一张纹理图像
				//  对应片元着色器中的声明：layout (binding = 1) uniform sampler2D samplerColor;
				vks::initializers::writeDescriptorSet(descriptorSets[i],
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		// The descriptor set will use a combined image sampler (as opposed to splitting image and sampler)
					1,												// Shader binding point 1
					&textureDescriptor)								// Pointer to the descriptor image for our texture
				// 这里的三个参数分别为：
				//  - 描述符类型：使用 combined image sampler（而不是分开提供 image 与 sampler）
				//  - 绑定点索引：1，对应片元着色器的 binding = 1
				//  - 指向纹理 VkDescriptorImageInfo 的指针
			};

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()
	{
		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Pipeline
		// 图形管线配置
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

		// Shaders
		// 着色器阶段
		shaderStages[0] = loadShader(getShadersPath() + "texture/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "texture/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		// Vertex input state
		// 顶点输入状态
		std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)),
			vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	// 准备并初始化用于存放着色器 uniform 数据的缓冲区
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.modelView = camera.matrices.view;
		uniformData.viewPos = camera.viewPos;
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTexture();
		generateQuad();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		// This will bind the descriptor set that contains our image (texture), so it can be accessed in the fragment shader
		// 绑定包含纹理图像的描述符集，以便片元着色器可以访问该纹理
		// 绑定包含纹理图像的描述符集，以便在片元着色器中访问该纹理
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared)
			return;
		VulkanExampleBase::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->sliderFloat("LOD bias", &uniformData.lodBias, 0.0f, (float)texture.mipLevels);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
