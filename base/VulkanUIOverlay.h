/*
* UI overlay class using ImGui
*
* Copyright (C) 2017-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

#include "../external/imgui/imgui.h"

#if defined(__ANDROID__)
#include "VulkanAndroid.h"
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace vks
{
	/**
	 * @brief 使用 ImGui 的 UI 叠加层类
	 */
	class UIOverlay
	{
	public:
		vks::VulkanDevice* device{ nullptr };  // Vulkan 设备指针
		VkQueue queue{ VK_NULL_HANDLE };       // 提交队列句柄

		VkSampleCountFlagBits rasterizationSamples{ VK_SAMPLE_COUNT_1_BIT };  // 光栅化采样数
		uint32_t subpass{ 0 };                                                 // 子通道索引

		/**
		 * @brief 缓冲区结构，包含顶点和索引缓冲区
		 */
		struct Buffers {
			vks::Buffer vertexBuffer;  // 顶点缓冲区
			vks::Buffer indexBuffer;    // 索引缓冲区
			int32_t vertexCount{ 0 };   // 顶点数量
			int32_t indexCount{ 0 };   // 索引数量
		};
		std::vector<Buffers> buffers;        // 每帧的缓冲区列表
		uint32_t maxConcurrentFrames{ 0 };   // 最大并发帧数
		uint32_t currentBuffer{ 0 };         // 当前缓冲区索引

		std::vector<VkPipelineShaderStageCreateInfo> shaders;  // 着色器阶段列表

		VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };        // 描述符池句柄
		VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };  // 描述符集布局句柄
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };          // 描述符集句柄
		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };       // 管线布局句柄
		VkPipeline pipeline{ VK_NULL_HANDLE };                  // 图形管线句柄

		VkDeviceMemory fontMemory{ VK_NULL_HANDLE };  // 字体图像内存句柄
		VkImage fontImage{ VK_NULL_HANDLE };           // 字体图像句柄
		VkImageView fontView{ VK_NULL_HANDLE };       // 字体图像视图句柄
		VkSampler sampler{ VK_NULL_HANDLE };          // 采样器句柄

		/**
		 * @brief 推送常量块，包含缩放和平移
		 */
		struct PushConstBlock {
			glm::vec2 scale;      // 缩放因子
			glm::vec2 translate;  // 平移偏移
		} pushConstBlock;

		bool visible{ true };    // UI 是否可见
		float scale{ 1.0f };     // UI 缩放因子

		/**
		 * @brief 构造函数，初始化 ImGui 上下文
		 */
		UIOverlay();
		/**
		 * @brief 析构函数，清理 ImGui 上下文
		 */
		~UIOverlay();

		/**
		 * @brief 准备渲染管线
		 * @param pipelineCache 管线缓存句柄
		 * @param renderPass 渲染通道句柄
		 * @param colorFormat 颜色格式
		 * @param depthFormat 深度格式
		 */
		void preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat);
		/**
		 * @brief 准备渲染资源（字体纹理、描述符等）
		 */
		void prepareResources();

		/**
		 * @brief 更新 UI 数据（每帧调用）
		 * @param currentBuffer 当前缓冲区索引
		 */
		void update(uint32_t currentBuffer);
		/**
		 * @brief 绘制 UI（记录到命令缓冲区）
		 * @param commandBuffer 命令缓冲区句柄
		 * @param currentBuffer 当前缓冲区索引
		 */
		void draw(const VkCommandBuffer commandBuffer, uint32_t currentBuffer);
		/**
		 * @brief 调整 UI 大小
		 * @param width 新宽度
		 * @param height 新高度
		 */
		void resize(uint32_t width, uint32_t height);

		/**
		 * @brief 释放所有资源
		 */
		void freeResources();

		/**
		 * @brief 创建可折叠的标题
		 * @param caption 标题文本
		 * @return 如果展开返回 true
		 */
		bool header(const char* caption);
		/**
		 * @brief 创建复选框（布尔值）
		 * @param caption 标签文本
		 * @param value 指向布尔值的指针
		 * @return 如果值改变返回 true
		 */
		bool checkBox(const char* caption, bool* value);
		/**
		 * @brief 创建复选框（整数值）
		 * @param caption 标签文本
		 * @param value 指向整数值的指针
		 * @return 如果值改变返回 true
		 */
		bool checkBox(const char* caption, int32_t* value);
		/**
		 * @brief 创建单选按钮
		 * @param caption 标签文本
		 * @param value 当前值
		 * @return 如果被选中返回 true
		 */
		bool radioButton(const char* caption, bool value);
		/**
		 * @brief 创建浮点数输入框
		 * @param caption 标签文本
		 * @param value 指向浮点数的指针
		 * @param step 步进值
		 * @param precision 精度（小数位数）
		 * @return 如果值改变返回 true
		 */
		bool inputFloat(const char* caption, float* value, float step, uint32_t precision);
		/**
		 * @brief 创建浮点数滑块
		 * @param caption 标签文本
		 * @param value 指向浮点数的指针
		 * @param min 最小值
		 * @param max 最大值
		 * @return 如果值改变返回 true
		 */
		bool sliderFloat(const char* caption, float* value, float min, float max);
		/**
		 * @brief 创建整数滑块
		 * @param caption 标签文本
		 * @param value 指向整数的指针
		 * @param min 最小值
		 * @param max 最大值
		 * @return 如果值改变返回 true
		 */
		bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
		/**
		 * @brief 创建下拉框
		 * @param caption 标签文本
		 * @param itemindex 指向当前选中索引的指针
		 * @param items 选项列表
		 * @return 如果选择改变返回 true
		 */
		bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
		/**
		 * @brief 创建按钮
		 * @param caption 按钮文本
		 * @return 如果被点击返回 true
		 */
		bool button(const char* caption);
		/**
		 * @brief 创建颜色选择器
		 * @param caption 标签文本
		 * @param color 指向颜色数组的指针（RGBA，4个浮点数）
		 * @return 如果颜色改变返回 true
		 */
		bool colorPicker(const char* caption, float* color);
		/**
		 * @brief 显示文本（支持格式化字符串）
		 * @param formatstr 格式化字符串
		 * @param ... 可变参数
		 */
		void text(const char* formatstr, ...);
	};
}
