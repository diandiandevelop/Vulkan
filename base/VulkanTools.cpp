/*
 * Assorted commonly used Vulkan helper functions
 *
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "VulkanTools.h"

#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
// iOS & macOS: getAssetPath() and getShaderBasePath() implemented externally for access to Obj-C++ path utilities
// iOS 和 macOS：getAssetPath() 和 getShaderBasePath() 在外部实现，以访问 Obj-C++ 路径工具
/**
 * @brief 获取资源文件路径
 * @return 资源文件目录路径
 */
const std::string getAssetPath()
{
if (vks::tools::resourcePath != "") {  // 如果设置了资源路径
	return vks::tools::resourcePath + "/assets/";  // 返回资源路径 + "/assets/"
}
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	return "";  // Android 平台：返回空字符串（资源在 APK 中）
#elif defined(VK_EXAMPLE_ASSETS_DIR)
	return VK_EXAMPLE_ASSETS_DIR;  // 如果定义了资源目录宏，使用该路径
#else
	return "./../assets/";  // 默认：返回相对路径
#endif
}

/**
 * @brief 获取着色器基础路径
 * @return 着色器目录路径
 */
const std::string getShaderBasePath()
{
if (vks::tools::resourcePath != "") {  // 如果设置了资源路径
	return vks::tools::resourcePath + "/shaders/";  // 返回资源路径 + "/shaders/"
}
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	return "shaders/";  // Android 平台：返回 "shaders/"（资源在 APK 中）
#elif defined(VK_EXAMPLE_SHADERS_DIR)
	return VK_EXAMPLE_SHADERS_DIR;  // 如果定义了着色器目录宏，使用该路径
#else
	return "./../shaders/";  // 默认：返回相对路径
#endif
}
#endif

namespace vks
{
	namespace tools
	{
		bool errorModeSilent = false;      // 错误模式静默标志
		std::string resourcePath = "";      // 资源路径

		/**
		 * @brief 将 Vulkan 错误代码转换为字符串
		 * @param errorCode Vulkan 结果错误代码
		 * @return 错误代码的字符串表示
		 */
		std::string errorString(VkResult errorCode)
		{
			switch (errorCode)  // 根据错误代码返回对应的字符串
			{
#define STR(r) case VK_ ##r: return #r  // 宏定义：将 VK_XXX 转换为字符串 "XXX"
				STR(NOT_READY);  // VK_NOT_READY -> "NOT_READY"
				STR(TIMEOUT);    // VK_TIMEOUT -> "TIMEOUT"
				STR(EVENT_SET);  // VK_EVENT_SET -> "EVENT_SET"
				STR(EVENT_RESET);  // VK_EVENT_RESET -> "EVENT_RESET"
				STR(INCOMPLETE);  // VK_INCOMPLETE -> "INCOMPLETE"
				STR(ERROR_OUT_OF_HOST_MEMORY);  // VK_ERROR_OUT_OF_HOST_MEMORY -> "ERROR_OUT_OF_HOST_MEMORY"
				STR(ERROR_OUT_OF_DEVICE_MEMORY);  // VK_ERROR_OUT_OF_DEVICE_MEMORY -> "ERROR_OUT_OF_DEVICE_MEMORY"
				STR(ERROR_INITIALIZATION_FAILED);  // VK_ERROR_INITIALIZATION_FAILED -> "ERROR_INITIALIZATION_FAILED"
				STR(ERROR_DEVICE_LOST);  // VK_ERROR_DEVICE_LOST -> "ERROR_DEVICE_LOST"
				STR(ERROR_MEMORY_MAP_FAILED);  // VK_ERROR_MEMORY_MAP_FAILED -> "ERROR_MEMORY_MAP_FAILED"
				STR(ERROR_LAYER_NOT_PRESENT);  // VK_ERROR_LAYER_NOT_PRESENT -> "ERROR_LAYER_NOT_PRESENT"
				STR(ERROR_EXTENSION_NOT_PRESENT);  // VK_ERROR_EXTENSION_NOT_PRESENT -> "ERROR_EXTENSION_NOT_PRESENT"
				STR(ERROR_FEATURE_NOT_PRESENT);  // VK_ERROR_FEATURE_NOT_PRESENT -> "ERROR_FEATURE_NOT_PRESENT"
				STR(ERROR_INCOMPATIBLE_DRIVER);  // VK_ERROR_INCOMPATIBLE_DRIVER -> "ERROR_INCOMPATIBLE_DRIVER"
				STR(ERROR_TOO_MANY_OBJECTS);  // VK_ERROR_TOO_MANY_OBJECTS -> "ERROR_TOO_MANY_OBJECTS"
				STR(ERROR_FORMAT_NOT_SUPPORTED);  // VK_ERROR_FORMAT_NOT_SUPPORTED -> "ERROR_FORMAT_NOT_SUPPORTED"
				STR(ERROR_SURFACE_LOST_KHR);  // VK_ERROR_SURFACE_LOST_KHR -> "ERROR_SURFACE_LOST_KHR"
				STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);  // VK_ERROR_NATIVE_WINDOW_IN_USE_KHR -> "ERROR_NATIVE_WINDOW_IN_USE_KHR"
				STR(SUBOPTIMAL_KHR);  // VK_SUBOPTIMAL_KHR -> "SUBOPTIMAL_KHR"
				STR(ERROR_OUT_OF_DATE_KHR);  // VK_ERROR_OUT_OF_DATE_KHR -> "ERROR_OUT_OF_DATE_KHR"
				STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);  // VK_ERROR_INCOMPATIBLE_DISPLAY_KHR -> "ERROR_INCOMPATIBLE_DISPLAY_KHR"
				STR(ERROR_VALIDATION_FAILED_EXT);  // VK_ERROR_VALIDATION_FAILED_EXT -> "ERROR_VALIDATION_FAILED_EXT"
				STR(ERROR_INVALID_SHADER_NV);  // VK_ERROR_INVALID_SHADER_NV -> "ERROR_INVALID_SHADER_NV"
				STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);  // VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT -> "ERROR_INCOMPATIBLE_SHADER_BINARY_EXT"
#undef STR  // 取消宏定义
			default:
				return "UNKNOWN_ERROR";  // 未知错误代码
			}
		}

		/**
		 * @brief 将物理设备类型转换为字符串
		 * @param type 物理设备类型
		 * @return 设备类型的字符串表示
		 */
		std::string physicalDeviceTypeString(VkPhysicalDeviceType type)
		{
			switch (type)  // 根据设备类型返回对应的字符串
			{
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r  // 宏定义：将 VK_PHYSICAL_DEVICE_TYPE_XXX 转换为字符串 "XXX"
				STR(OTHER);          // VK_PHYSICAL_DEVICE_TYPE_OTHER -> "OTHER"
				STR(INTEGRATED_GPU); // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU -> "INTEGRATED_GPU"
				STR(DISCRETE_GPU);   // VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU -> "DISCRETE_GPU"
				STR(VIRTUAL_GPU);    // VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU -> "VIRTUAL_GPU"
				STR(CPU);            // VK_PHYSICAL_DEVICE_TYPE_CPU -> "CPU"
#undef STR  // 取消宏定义
			default: return "UNKNOWN_DEVICE_TYPE";  // 未知设备类型
			}
		}

		/**
		 * @brief 获取支持的深度格式
		 * @param physicalDevice 物理设备句柄
		 * @param depthFormat 输出的深度格式
		 * @return 如果找到支持的格式返回 true
		 */
		VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
		{
			// Since all depth formats may be optional, we need to find a suitable depth format to use
			// Start with the highest precision packed format
			// 由于所有深度格式都是可选的，我们需要找到一个合适的深度格式
			// 从最高精度的打包格式开始
			std::vector<VkFormat> formatList = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,  // 32位浮点深度 + 8位无符号整数模板
				VK_FORMAT_D32_SFLOAT,          // 32位浮点深度
				VK_FORMAT_D24_UNORM_S8_UINT,   // 24位无符号归一化深度 + 8位无符号整数模板
				VK_FORMAT_D16_UNORM_S8_UINT,   // 16位无符号归一化深度 + 8位无符号整数模板
				VK_FORMAT_D16_UNORM            // 16位无符号归一化深度
			};

			for (auto& format : formatList)  // 遍历格式列表
			{
				VkFormatProperties formatProps;  // 格式属性
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);  // 获取格式属性
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)  // 检查是否支持作为深度模板附件
				{
					*depthFormat = format;  // 设置找到的格式
					return true;  // 返回成功
				}
			}

			return false;  // 未找到支持的格式
		}

		/**
		 * @brief 获取支持的深度模板格式
		 * @param physicalDevice 物理设备句柄
		 * @param depthStencilFormat 输出的深度模板格式
		 * @return 如果找到支持的格式返回 true
		 */
		VkBool32 getSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice, VkFormat* depthStencilFormat)
		{
			std::vector<VkFormat> formatList = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,  // 32位浮点深度 + 8位模板
				VK_FORMAT_D24_UNORM_S8_UINT,   // 24位深度 + 8位模板
				VK_FORMAT_D16_UNORM_S8_UINT,   // 16位深度 + 8位模板
			};

			for (auto& format : formatList)  // 遍历格式列表（只包含带模板的格式）
			{
				VkFormatProperties formatProps;  // 格式属性
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);  // 获取格式属性
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)  // 检查是否支持作为深度模板附件
				{
					*depthStencilFormat = format;  // 设置找到的格式
					return true;  // 返回成功
				}
			}

			return false;  // 未找到支持的格式
		}


		/**
		 * @brief 检查格式是否包含模板分量
		 * @param format 要检查的格式
		 * @return 如果格式有模板分量返回 true
		 */
		VkBool32 formatHasStencil(VkFormat format)
		{
			std::vector<VkFormat> stencilFormats = {
				VK_FORMAT_S8_UINT,              // 仅 8位模板
				VK_FORMAT_D16_UNORM_S8_UINT,    // 16位深度 + 8位模板
				VK_FORMAT_D24_UNORM_S8_UINT,    // 24位深度 + 8位模板
				VK_FORMAT_D32_SFLOAT_S8_UINT,   // 32位深度 + 8位模板
			};
			return std::find(stencilFormats.begin(), stencilFormats.end(), format) != std::end(stencilFormats);  // 在模板格式列表中查找
		}

		// Returns if a given format support LINEAR filtering
		// 返回给定格式是否支持线性过滤
		/**
		 * @brief 检查格式是否支持线性过滤
		 * @param physicalDevice 物理设备句柄
		 * @param format 要检查的格式
		 * @param tiling 图像平铺方式
		 * @return 如果支持线性过滤返回 true
		 */
		VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling)
		{
			VkFormatProperties formatProps;  // 格式属性
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);  // 获取格式属性

			if (tiling == VK_IMAGE_TILING_OPTIMAL)  // 如果是最优平铺
				return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;  // 检查最优平铺特性

			if (tiling == VK_IMAGE_TILING_LINEAR)  // 如果是线性平铺
				return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;  // 检查线性平铺特性

			return false;  // 其他情况不支持
		}

		// Create an image memory barrier for changing the layout of
		// an image and put it into an active command buffer
		// See chapter 11.4 "Image Layout" for details
		// 创建图像内存屏障以更改图像的布局，并将其放入活动命令缓冲区
		// 详见第 11.4 章"图像布局"
		/**
		 * @brief 设置图像布局（完整版本，带子资源范围）
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param image 图像句柄
		 * @param oldImageLayout 旧的图像布局
		 * @param newImageLayout 新的图像布局
		 * @param subresourceRange 子资源范围
		 * @param srcStageMask 源管线阶段掩码
		 * @param dstStageMask 目标管线阶段掩码
		 */
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			// Create an image barrier object
			// 创建图像屏障对象
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();  // 初始化图像内存屏障
			imageMemoryBarrier.oldLayout = oldImageLayout;        // 旧布局
			imageMemoryBarrier.newLayout = newImageLayout;        // 新布局
			imageMemoryBarrier.image = image;                     // 图像句柄
			imageMemoryBarrier.subresourceRange = subresourceRange;  // 子资源范围

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			// 源布局（旧布局）
			// 源访问掩码控制在旧布局上必须完成的操作，然后才能转换到新布局
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				// 图像布局未定义（或无关紧要）
				// 仅作为初始布局有效
				// 不需要标志，仅为了完整性列出
				imageMemoryBarrier.srcAccessMask = 0;  // 无访问要求
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				// 图像已预初始化
				// 仅对线性图像作为初始布局有效，保留内存内容
				// 确保主机写入已完成
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;  // 主机写入访问
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				// 图像是颜色附件
				// 确保对颜色缓冲区的所有写入已完成
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // 颜色附件写入访问
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				// 图像是深度/模板附件
				// 确保对深度/模板缓冲区的所有写入已完成
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // 深度模板附件写入访问
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				// 图像是传输源
				// 确保对图像的所有读取已完成
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;  // 传输读取访问
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				// 图像是传输目标
				// 确保对图像的所有写入已完成
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // 传输写入访问
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				// 图像被着色器读取
				// 确保着色器对图像的所有读取已完成
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;  // 着色器读取访问
				break;
			default:
				// Other source layouts aren't handled (yet)
				// 其他源布局尚未处理
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			// 目标布局（新布局）
			// 目标访问掩码控制新图像布局的依赖关系
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				// 图像将用作传输目标
				// 确保对图像的所有写入已完成
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // 传输写入访问
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				// 图像将用作传输源
				// 确保对图像的所有读取已完成
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;  // 传输读取访问
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				// 图像将用作颜色附件
				// 确保对颜色缓冲区的所有写入已完成
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // 颜色附件写入访问
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				// 图像布局将用作深度/模板附件
				// 确保对深度/模板缓冲区的所有写入已完成
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // 深度模板附件写入访问（或运算保留原有标志）
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				// 图像将在着色器中读取（采样器、输入附件）
				// 确保对图像的所有写入已完成
				if (imageMemoryBarrier.srcAccessMask == 0)  // 如果源访问掩码为 0（从未定义布局转换）
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;  // 设置主机写入和传输写入访问
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;  // 着色器读取访问
				break;
			default:
				// Other source layouts aren't handled (yet)
				// 其他源布局尚未处理
				break;
			}

			// Put barrier inside setup command buffer
			// 将屏障放入设置命令缓冲区
			vkCmdPipelineBarrier(
				cmdbuffer,              // 命令缓冲区
				srcStageMask,           // 源管线阶段掩码
				dstStageMask,          // 目标管线阶段掩码
				0,                      // 依赖标志
				0, nullptr,             // 内存屏障数量（无）
				0, nullptr,             // 缓冲区内存屏障数量（无）
				1, &imageMemoryBarrier);  // 图像内存屏障数量（1个）
		}

		// Fixed sub resource on first mip level and layer
		// 固定子资源在第一个 Mip 级别和层
		/**
		 * @brief 设置图像布局（简化版本，使用固定的子资源范围）
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param image 图像句柄
		 * @param aspectMask 图像方面掩码
		 * @param oldImageLayout 旧的图像布局
		 * @param newImageLayout 新的图像布局
		 * @param srcStageMask 源管线阶段掩码
		 * @param dstStageMask 目标管线阶段掩码
		 */
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = {};  // 子资源范围
			subresourceRange.aspectMask = aspectMask;        // 方面掩码
			subresourceRange.baseMipLevel = 0;               // 基础 Mip 级别（第一个）
			subresourceRange.levelCount = 1;                 // Mip 级别数量（1个）
			subresourceRange.layerCount = 1;                 // 层数量（1个）
			setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);  // 调用完整版本
		}

		/**
		 * @brief 插入图像内存屏障
		 * @param cmdbuffer 命令缓冲区句柄
		 * @param image 图像句柄
		 * @param srcAccessMask 源访问掩码
		 * @param dstAccessMask 目标访问掩码
		 * @param oldImageLayout 旧的图像布局
		 * @param newImageLayout 新的图像布局
		 * @param srcStageMask 源管线阶段掩码
		 * @param dstStageMask 目标管线阶段掩码
		 * @param subresourceRange 子资源范围
		 */
		void insertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();  // 初始化图像内存屏障
			imageMemoryBarrier.srcAccessMask = srcAccessMask;        // 源访问掩码
			imageMemoryBarrier.dstAccessMask = dstAccessMask;        // 目标访问掩码
			imageMemoryBarrier.oldLayout = oldImageLayout;          // 旧布局
			imageMemoryBarrier.newLayout = newImageLayout;          // 新布局
			imageMemoryBarrier.image = image;                       // 图像句柄
			imageMemoryBarrier.subresourceRange = subresourceRange;  // 子资源范围

			vkCmdPipelineBarrier(
				cmdbuffer,              // 命令缓冲区
				srcStageMask,           // 源管线阶段掩码
				dstStageMask,          // 目标管线阶段掩码
				0,                      // 依赖标志
				0, nullptr,             // 内存屏障数量（无）
				0, nullptr,             // 缓冲区内存屏障数量（无）
				1, &imageMemoryBarrier);  // 图像内存屏障数量（1个）
		}

		/**
		 * @brief 致命错误退出（使用退出代码）
		 * @param message 错误消息
		 * @param exitCode 退出代码
		 */
		void exitFatal(const std::string& message, int32_t exitCode)
		{
#if defined(_WIN32)
			if (!errorModeSilent) {  // 如果未启用静默模式
				MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);  // 显示 Windows 消息框
			}
#elif defined(__ANDROID__)
            LOGE("Fatal error: %s", message.c_str());  // 输出 Android 日志
			vks::android::showAlert(message.c_str());  // 显示 Android 警告对话框
#endif
			std::cerr << message << "\n";  // 输出错误消息到标准错误流
#if !defined(__ANDROID__)
			exit(exitCode);  // 退出程序（Android 平台不退出，让系统处理）
#endif
		}

		/**
		 * @brief 致命错误退出（使用 Vulkan 结果代码）
		 * @param message 错误消息
		 * @param resultCode Vulkan 结果代码
		 */
		void exitFatal(const std::string& message, VkResult resultCode)
		{
			exitFatal(message, (int32_t)resultCode);  // 将 Vulkan 结果代码转换为整数并调用退出函数
		}

#if defined(__ANDROID__)
		// Android shaders are stored as assets in the apk
		// So they need to be loaded via the asset manager
		// Android 着色器作为资源存储在 APK 中
		// 因此需要通过资源管理器加载
		/**
		 * @brief 从 Android 资源加载 SPIR-V 着色器（Android 平台）
		 * @param assetManager Android 资源管理器
		 * @param fileName 着色器文件名
		 * @param device Vulkan 设备句柄
		 * @return 着色器模块句柄
		 */
		VkShaderModule loadShader(AAssetManager* assetManager, const char *fileName, VkDevice device)
		{
			// Load shader from compressed asset
			// 从压缩资源加载着色器
			AAsset* asset = AAssetManager_open(assetManager, fileName, AASSET_MODE_STREAMING);  // 打开资源文件
			assert(asset);  // 确保资源打开成功
			size_t size = AAsset_getLength(asset);  // 获取资源大小
			assert(size > 0);  // 确保大小大于 0

			char *shaderCode = new char[size];  // 分配缓冲区
			AAsset_read(asset, shaderCode, size);  // 读取资源数据
			AAsset_close(asset);  // 关闭资源

			VkShaderModule shaderModule;  // 着色器模块句柄
			VkShaderModuleCreateInfo moduleCreateInfo;  // 着色器模块创建信息
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;  // 结构体类型
			moduleCreateInfo.pNext = NULL;  // 下一结构指针
			moduleCreateInfo.codeSize = size;  // 代码大小
			moduleCreateInfo.pCode = (uint32_t*)shaderCode;  // 代码指针
			moduleCreateInfo.flags = 0;  // 标志

			VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));  // 创建着色器模块

			delete[] shaderCode;  // 释放缓冲区

			return shaderModule;
		}
#else
		/**
		 * @brief 从文件加载 SPIR-V 着色器（非 Android 平台）
		 * @param fileName 着色器文件路径
		 * @param device Vulkan 设备句柄
		 * @return 着色器模块句柄
		 */
		VkShaderModule loadShader(const char *fileName, VkDevice device)
		{
			std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);  // 以二进制模式打开文件，定位到文件末尾

			if (is.is_open())  // 如果文件打开成功
			{
				size_t size = is.tellg();  // 获取文件大小（当前位置 = 文件末尾）
				is.seekg(0, std::ios::beg);  // 重新定位到文件开始
				char* shaderCode = new char[size];  // 分配缓冲区
				is.read(shaderCode, size);  // 读取文件内容
				is.close();  // 关闭文件

				assert(size > 0);  // 确保大小大于 0

				VkShaderModule shaderModule;  // 着色器模块句柄
				VkShaderModuleCreateInfo moduleCreateInfo{};  // 着色器模块创建信息
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;  // 结构体类型
				moduleCreateInfo.codeSize = size;  // 代码大小
				moduleCreateInfo.pCode = (uint32_t*)shaderCode;  // 代码指针

				VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));  // 创建着色器模块

				delete[] shaderCode;  // 释放缓冲区

				return shaderModule;
			}
			else  // 如果文件打开失败
			{
				std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";  // 输出错误信息
				return VK_NULL_HANDLE;  // 返回空句柄
			}
		}
#endif

		/**
		 * @brief 检查文件是否存在
		 * @param filename 文件名
		 * @return 如果文件存在返回 true
		 */
		bool fileExists(const std::string &filename)
		{
			std::ifstream f(filename.c_str());  // 尝试打开文件
			return !f.fail();  // 如果打开成功，文件存在
		}

		/**
		 * @brief 计算对齐后的大小（32位版本）
		 * @param value 原始值
		 * @param alignment 对齐值
		 * @return 对齐后的大小（向上对齐到 alignment 的倍数）
		 */
		uint32_t alignedSize(uint32_t value, uint32_t alignment)
        {
	        return (value + alignment - 1) & ~(alignment - 1);  // 向上对齐算法
		}

		/**
		 * @brief 计算对齐后的大小（size_t 版本）
		 * @param value 原始值
		 * @param alignment 对齐值
		 * @return 对齐后的大小（向上对齐到 alignment 的倍数）
		 */
		size_t alignedSize(size_t value, size_t alignment)
		{
			return (value + alignment - 1) & ~(alignment - 1);  // 向上对齐算法
		}


		/**
		 * @brief 计算对齐后的大小（VkDeviceSize 版本）
		 * @param value 原始值
		 * @param alignment 对齐值
		 * @return 对齐后的大小（向上对齐到 alignment 的倍数）
		 */
		VkDeviceSize alignedVkSize(VkDeviceSize value, VkDeviceSize alignment)
		{
			return (value + alignment - 1) & ~(alignment - 1);  // 向上对齐算法
		}

	}
}
