/*
* Vulkan texture loader for KTX files
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"

#include <ktx.h>
#include <ktxvulkan.h>

#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTools.h"

#if defined(__ANDROID__)
#	include <android/asset_manager.h>
#endif

namespace vks
{
class Texture
{
  public:
	vks::VulkanDevice *   device;        // Vulkan 设备指针
	VkImage               image;         // 图像句柄
	VkImageLayout         imageLayout;   // 图像布局
	VkDeviceMemory        deviceMemory;  // 设备内存句柄
	VkImageView           view;          // 图像视图句柄
	uint32_t              width, height;  // 图像宽度和高度
	uint32_t              mipLevels;       // Mip 级别数量
	uint32_t              layerCount;     // 层数量
	VkDescriptorImageInfo descriptor;    // 描述符图像信息
	VkSampler             sampler;       // 采样器句柄

	/**
	 * @brief 更新描述符信息
	 */
	void      updateDescriptor();
	/**
	 * @brief 销毁纹理资源
	 */
	void      destroy();
	/**
	 * @brief 加载 KTX 文件
	 * @param filename 文件名
	 * @param target 输出的 KTX 纹理指针
	 * @return KTX 结果
	 */
	ktxResult loadKTXFile(std::string filename, ktxTexture **target);
};

class Texture2D : public Texture
{
  public:
	/**
	 * @brief 从文件加载 2D 纹理
	 * @param filename 文件名
	 * @param format 图像格式
	 * @param device Vulkan 设备指针
	 * @param copyQueue 复制队列
	 * @param imageUsageFlags 图像使用标志
	 * @param imageLayout 图像布局
	 */
	void loadFromFile(
	    std::string        filename,
	    VkFormat           format,
	    vks::VulkanDevice *device,
	    VkQueue            copyQueue,
	    VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
	    VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	/**
	 * @brief 从缓冲区创建 2D 纹理
	 * @param buffer 缓冲区指针
	 * @param bufferSize 缓冲区大小
	 * @param format 图像格式
	 * @param texWidth 纹理宽度
	 * @param texHeight 纹理高度
	 * @param device Vulkan 设备指针
	 * @param copyQueue 复制队列
	 * @param filter 过滤模式
	 * @param imageUsageFlags 图像使用标志
	 * @param imageLayout 图像布局
	 */
	void fromBuffer(
	    void *             buffer,
	    VkDeviceSize       bufferSize,
	    VkFormat           format,
	    uint32_t           texWidth,
	    uint32_t           texHeight,
	    vks::VulkanDevice *device,
	    VkQueue            copyQueue,
	    VkFilter           filter          = VK_FILTER_LINEAR,
	    VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
	    VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};

class Texture2DArray : public Texture
{
  public:
	void loadFromFile(
	    std::string        filename,
	    VkFormat           format,
	    vks::VulkanDevice *device,
	    VkQueue            copyQueue,
	    VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
	    VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};

class TextureCubeMap : public Texture
{
  public:
	void loadFromFile(
	    std::string        filename,
	    VkFormat           format,
	    vks::VulkanDevice *device,
	    VkQueue            copyQueue,
	    VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
	    VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
};
}        // namespace vks
