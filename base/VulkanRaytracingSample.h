/*
* Extended sample base class for ray tracing based samples
*
* Copyright (C) 2020-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "vulkan/vulkan.h"
#include "vulkanexamplebase.h"
#include "VulkanTools.h"
#include "VulkanDevice.h"

// Base sample class with added features specific to hardware ray traced samples
// 基础示例类，添加了特定于硬件光线追踪示例的功能
/**
 * @brief Vulkan 光线追踪示例基类
 * 扩展了 VulkanExampleBase，添加了光线追踪相关的功能
 */
class VulkanRaytracingSample : public VulkanExampleBase
{
protected:
	// Update the default render pass with different color attachment load ops
	// 使用不同的颜色附件加载操作更新默认渲染通道
	/**
	 * @brief 设置渲染通道（虚函数，可重写）
	 */
	virtual void setupRenderPass();
	/**
	 * @brief 设置帧缓冲区（虚函数，可重写）
	 */
	virtual void setupFrameBuffer();
public:
	// Function pointers for ray tracing related stuff
	// 光线追踪相关功能的函数指针
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{ nullptr };                    // 获取缓冲区设备地址
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR{ nullptr };          // 创建加速结构
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR{ nullptr };         // 销毁加速结构
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR{ nullptr };  // 获取加速结构构建大小
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR{ nullptr };  // 获取加速结构设备地址
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR{ nullptr };          // 构建加速结构
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR{ nullptr };    // 命令缓冲区构建加速结构
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR{ nullptr };                                         // 命令缓冲区追踪光线
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR{ nullptr }; // 获取光线追踪着色器组句柄
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR{ nullptr };              // 创建光线追踪管线

	// Available features and properties
	// 可用的功能和属性
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};  // 光线追踪管线属性
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};  // 加速结构功能
	
	// Enabled features and properties
	// 启用的功能和属性
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};           // 启用的缓冲区设备地址功能
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};         // 启用的光线追踪管线功能
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};    // 启用的加速结构功能

	// Holds information for a ray tracing scratch buffer that is used as a temporary storage
	// 保存用于临时存储的光线追踪暂存缓冲区信息
	/**
	 * @brief 暂存缓冲区结构（用于加速结构构建时的临时存储）
	 */
	struct ScratchBuffer
	{
		uint64_t deviceAddress{ 0 };      // 设备地址
		VkBuffer handle{ VK_NULL_HANDLE };  // 缓冲区句柄
		VkDeviceMemory memory{ VK_NULL_HANDLE };  // 内存句柄
	};

	// Holds information for a ray tracing acceleration structure
	// 保存光线追踪加速结构信息
	/**
	 * @brief 加速结构类（用于光线追踪的几何加速结构）
	 */
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle{ VK_NULL_HANDLE };  // 加速结构句柄
		uint64_t deviceAddress{ 0 };                          // 设备地址
		VkDeviceMemory memory{ VK_NULL_HANDLE };               // 内存句柄
		VkBuffer buffer{ VK_NULL_HANDLE };                    // 缓冲区句柄
	};

	// Holds information for a storage image that the ray tracing shaders output to
	// 保存光线追踪着色器输出到的存储图像信息
	/**
	 * @brief 存储图像结构（光线追踪着色器的输出目标）
	 */
	struct StorageImage {
		VkDeviceMemory memory{ VK_NULL_HANDLE };  // 内存句柄
		VkImage image{ VK_NULL_HANDLE };          // 图像句柄
		VkImageView view{ VK_NULL_HANDLE };       // 图像视图句柄
		VkFormat format;                          // 图像格式
	} storageImage;

	// Extends the buffer class and holds information for a shader binding table
	// 扩展缓冲区类并保存着色器绑定表信息
	/**
	 * @brief 着色器绑定表类（用于光线追踪的着色器绑定表）
	 */
	class ShaderBindingTable : public vks::Buffer {
	public:
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};  // 跨步设备地址区域
	};

	// Set to true, to denote that the sample only uses ray queries (changes extension and render pass handling)
	// 设置为 true，表示示例仅使用光线查询（更改扩展和渲染通道处理）
	bool rayQueryOnly = false;  // 是否仅使用光线查询

	/**
	 * @brief 启用光线追踪相关的扩展
	 */
	void enableExtensions();
	/**
	 * @brief 创建暂存缓冲区
	 * @param size 缓冲区大小
	 * @return 暂存缓冲区结构
	 */
	ScratchBuffer createScratchBuffer(VkDeviceSize size);
	/**
	 * @brief 删除暂存缓冲区
	 * @param scratchBuffer 要删除的暂存缓冲区引用
	 */
	void deleteScratchBuffer(ScratchBuffer& scratchBuffer);
	/**
	 * @brief 创建加速结构
	 * @param accelerationStructure 要填充的加速结构引用
	 * @param type 加速结构类型
	 * @param buildSizeInfo 构建大小信息
	 */
	void createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	/**
	 * @brief 删除加速结构
	 * @param accelerationStructure 要删除的加速结构引用
	 */
	void deleteAccelerationStructure(AccelerationStructure& accelerationStructure);
	/**
	 * @brief 获取缓冲区设备地址
	 * @param buffer 缓冲区句柄
	 * @return 设备地址
	 */
	uint64_t getBufferDeviceAddress(VkBuffer buffer);
	/**
	 * @brief 创建存储图像
	 * @param format 图像格式
	 * @param extent 图像范围
	 */
	void createStorageImage(VkFormat format, VkExtent3D extent);
	/**
	 * @brief 删除存储图像
	 */
	void deleteStorageImage();
	/**
	 * @brief 获取 SBT 条目的跨步设备地址区域
	 * @param buffer 缓冲区句柄
	 * @param handleCount 句柄数量
	 * @return 跨步设备地址区域
	 */
	VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount);
	/**
	 * @brief 创建着色器绑定表
	 * @param shaderBindingTable 要填充的着色器绑定表引用
	 * @param handleCount 句柄数量
	 */
	void createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount);
	// Draw the ImGUI UI overlay using a render pass
	// 使用渲染通道绘制 ImGui UI 叠加层
	/**
	 * @brief 绘制 ImGui UI 叠加层
	 * @param commandBuffer 命令缓冲区句柄
	 * @param framebuffer 帧缓冲区句柄
	 */
	void drawUI(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

	/**
	 * @brief 准备示例（虚函数，可重写）
	 */
	virtual void prepare();
};
