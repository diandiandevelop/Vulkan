/*
* Vulkan Example - Basic indexed triangle rendering using Vulkan 1.3
*
* Note:
* This is a variation of the the triangle sample that makes use of Vulkan 1.3 features
* This simplifies the api a bit, esp. with dynamic rendering replacing render passes (and with that framebuffers)
*
* Copyright (C) 2024-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

// We want to keep GPU and CPU busy. To do that we may start building a new command buffer while the previous one is still being executed
// This number defines how many frames may be worked on simultaneously at once
// Increasing this number may improve performance but will also introduce additional latency
// 我们希望保持 GPU 和 CPU 忙碌。为此，我们可以在前一个命令缓冲区仍在执行时开始构建新的命令缓冲区
// 此数字定义可以同时处理多少帧
// 增加此数字可能会提高性能，但也会引入额外的延迟
constexpr auto MAX_CONCURRENT_FRAMES = 2;  // 最大并发帧数（2 帧重叠）

class VulkanExample : public VulkanExampleBase
{
public:
	// Vertex layout used in this example
	// 本示例中使用的顶点布局
	struct Vertex {
		float position[3];  // 顶点位置（XYZ）
		float color[3];  // 顶点颜色（RGB）
	};

	/**
	 * @brief Vulkan 缓冲区结构体
	 * 封装缓冲区和其内存句柄
	 */
	struct VulkanBuffer {
		VkDeviceMemory memory{ VK_NULL_HANDLE };  // 设备内存句柄
		VkBuffer handle{ VK_NULL_HANDLE };  // 缓冲区句柄
	};

	VulkanBuffer vertexBuffer;  // 顶点缓冲区
	VulkanBuffer indexBuffer;  // 索引缓冲区
	uint32_t indexCount{ 0 };  // 索引数量

	// Uniform buffer block object
	// 统一缓冲区块对象
	/**
	 * @brief 统一缓冲区结构体
	 * 继承自 VulkanBuffer，包含描述符集和映射指针
	 */
	struct UniformBuffer : VulkanBuffer {
		// The descriptor set stores the resources bound to the binding points in a shader
		// It connects the binding points of the different shaders with the buffers and images used for those bindings
		// 描述符集存储绑定到着色器中绑定点的资源
		// 它将不同着色器的绑定点与用于这些绑定的缓冲区和图像连接起来
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };  // 描述符集句柄
		// We keep a pointer to the mapped buffer, so we can easily update it's contents via a memcpy
		// 我们保留映射缓冲区的指针，这样我们可以通过 memcpy 轻松更新其内容
		uint8_t* mapped{ nullptr };  // 映射内存指针
	};
	// We use one UBO per frame, so we can have a frame overlap and make sure that uniforms aren't updated while still in use
	// 我们每帧使用一个 UBO，这样我们可以有帧重叠，并确保统一变量在使用时不会被更新
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;  // 每帧的统一缓冲区数组

	// For simplicity we use the same uniform block layout as in the shader
	// This way we can just memcpy the data to the ubo
	// Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
	// 为简单起见，我们使用与着色器中相同的统一块布局
	// 这样我们可以直接将数据 memcpy 到 UBO
	// 注意：您应该使用与 GPU 对齐的数据类型，以避免手动填充（vec4、mat4）
	/**
	 * @brief 着色器数据结构体
	 * 包含投影、模型和视图矩阵，与着色器中的统一块布局匹配
	 */
	struct ShaderData {
		glm::mat4 projectionMatrix;  // 投影矩阵
		glm::mat4 modelMatrix;  // 模型矩阵
		glm::mat4 viewMatrix;  // 视图矩阵
	};

	// The pipeline layout is used by a pipeline to access the descriptor sets
	// It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
	// A pipeline layout can be shared among multiple pipelines as long as their interfaces match
	// 管道布局由管道用于访问描述符集
	// 它定义管道使用的着色器阶段与着色器资源之间的接口（不绑定任何实际数据）
	// 只要接口匹配，管道布局可以在多个管道之间共享
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };  // 管道布局句柄

	// Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
	// While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
	// So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
	// Even though this adds a new dimension of planning ahead, it's a great opportunity for performance optimizations by the driver
	// 管道（通常称为"管道状态对象"）用于烘焙影响管道的所有状态
	// 虽然在 OpenGL 中每个状态都可以（几乎）随时更改，但 Vulkan 需要提前布局图形（和计算）管道状态
	// 因此，对于每个非动态管道状态的组合，您都需要一个新管道（这里不讨论一些例外情况）
	// 尽管这增加了提前规划的新维度，但这是驱动程序进行性能优化的绝佳机会
	VkPipeline pipeline{ VK_NULL_HANDLE };  // 图形管道句柄

	// The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
	// Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
	// 描述符集布局描述着色器绑定布局（不实际引用描述符）
	// 与管道布局一样，它基本上是一个蓝图，只要布局匹配，就可以与不同的描述符集一起使用
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };  // 描述符集布局句柄

	// Synchronization primitives
	// Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.
	// Semaphores are used to coordinate operations within the graphics queue and ensure correct command ordering
	// 同步原语
	// 同步是 Vulkan 的一个重要概念，OpenGL 大多隐藏了这一点。正确理解这一点对于使用 Vulkan 至关重要
	// 信号量用于协调图形队列内的操作并确保正确的命令顺序
	std::vector<VkSemaphore> presentCompleteSemaphores{};  // 呈现完成信号量（每帧一个）
	std::vector<VkSemaphore> renderCompleteSemaphores{};  // 渲染完成信号量（每个交换链图像一个）
	// Fences are used to make sure command buffers aren't rerecorded until they've finished executing
	// 栅栏用于确保命令缓冲区在执行完成之前不会被重新记录
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};  // 等待栅栏（每帧一个）

	VkCommandPool commandPool{ VK_NULL_HANDLE };  // 命令池句柄
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};  // 命令缓冲区数组（每帧一个）

	// To select the correct sync and command objects, we need to keep track of the current frame
	// 为了选择正确的同步和命令对象，我们需要跟踪当前帧
	uint32_t currentFrame{ 0 };  // 当前帧索引

	// Vulkan 1.3 特性启用标志
	// 用于启用动态渲染和同步 2 特性
	VkPhysicalDeviceVulkan13Features enabledFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };  // Vulkan 1.3 特性结构

	/**
	 * @brief 构造函数
	 * 初始化示例标题、相机和 Vulkan 1.3 特性
	 */
	VulkanExample() : VulkanExampleBase()
	{
		title = "Basic indexed triangle using Vulkan 1.3";  // 设置窗口标题
		// To keep things simple, we don't use the UI overlay from the framework
		// 为保持简单，我们不使用框架的 UI 叠加层
		settings.overlay = false;  // 禁用 UI 叠加层
		// Setup a default look-at camera
		// 设置默认的观察相机
		camera.type = Camera::CameraType::lookat;  // 相机类型（观察相机）
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));  // 设置相机位置（Z = -2.5）
		camera.setRotation(glm::vec3(0.0f));  // 设置相机旋转（无旋转）
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);  // 设置透视投影（FOV 60°，近平面 1.0，远平面 256.0）
		// We want to use Vulkan 1.3 with the dynamic rendering and sync 2 features
		// 我们希望使用 Vulkan 1.3 的动态渲染和同步 2 特性
		apiVersion = VK_API_VERSION_1_3;  // 设置 API 版本为 1.3
		enabledFeatures.dynamicRendering = VK_TRUE;  // 启用动态渲染特性
		enabledFeatures.synchronization2 = VK_TRUE;  // 启用同步 2 特性
		deviceCreatepNextChain = &enabledFeatures;  // 将特性结构添加到设备创建链中
	}

	/**
	 * @brief 析构函数
	 * 清理所有使用的 Vulkan 资源
	 * 注意：继承的析构函数会清理基类中存储的资源
	 */
	~VulkanExample() override
	{
		// Clean up used Vulkan resources
		// Note: Inherited destructor cleans up resources stored in base class
		// 清理使用的 Vulkan 资源
		// 注意：继承的析构函数会清理基类中存储的资源
		if (device) {  // 如果设备已创建
			vkDestroyPipeline(device, pipeline, nullptr);  // 销毁图形管道
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);  // 销毁管道布局
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);  // 销毁描述符集布局
			vkDestroyBuffer(device, vertexBuffer.handle, nullptr);  // 销毁顶点缓冲区
			vkFreeMemory(device, vertexBuffer.memory, nullptr);  // 释放顶点缓冲区内存
			vkDestroyBuffer(device, indexBuffer.handle, nullptr);  // 销毁索引缓冲区
			vkFreeMemory(device, indexBuffer.memory, nullptr);  // 释放索引缓冲区内存
			vkDestroyCommandPool(device, commandPool, nullptr);  // 销毁命令池
			// 销毁所有呈现完成信号量
			for (size_t i = 0; i < presentCompleteSemaphores.size(); i++) {
				vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			}
			// 销毁所有渲染完成信号量
			for (size_t i = 0; i < renderCompleteSemaphores.size(); i++) {
				vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			}
			// 销毁每帧的栅栏和统一缓冲区
			for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
				vkDestroyFence(device, waitFences[i], nullptr);  // 销毁栅栏
				vkDestroyBuffer(device, uniformBuffers[i].handle, nullptr);  // 销毁统一缓冲区
				vkFreeMemory(device, uniformBuffers[i].memory, nullptr);  // 释放统一缓冲区内存
			}
		}
	}

	/**
	 * @brief 获取启用的特性
	 * 检查设备是否支持 Vulkan 1.3
	 * 注意：这是基类中虚函数的重写
	 */
	virtual void getEnabledFeatures() override
	{
		// Vulkan 1.3 device support is required for this example
		// 本示例需要 Vulkan 1.3 设备支持
		if (deviceProperties.apiVersion < VK_API_VERSION_1_3) {  // 如果设备 API 版本低于 1.3
			vks::tools::exitFatal("Selected GPU does not support support Vulkan 1.3", VK_ERROR_INCOMPATIBLE_DRIVER);  // 退出并显示错误
		}
	}

	// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
	// Upon success it will return the index of the memory type that fits our requested memory properties
	// This is necessary as implementations can offer an arbitrary number of memory types with different memory properties
	// You can check https://vulkan.gpuinfo.org/ for details on different memory configurations
	/**
	 * @brief 获取内存类型索引
	 * 请求支持所有请求属性标志的设备内存类型（例如设备本地、主机可见）
	 * 成功时返回符合请求内存属性的内存类型索引
	 * 这是必要的，因为实现可以提供任意数量的具有不同内存属性的内存类型
	 * 您可以在 https://vulkan.gpuinfo.org/ 查看不同内存配置的详细信息
	 * @param typeBits 内存类型位掩码
	 * @param properties 请求的内存属性标志
	 * @return 内存类型索引
	 */
	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		// Iterate over all memory types available for the device used in this example
		// 遍历本示例使用的设备的所有可用内存类型
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {  // 遍历所有内存类型
			if ((typeBits & 1) == 1) {  // 如果当前位为 1（此内存类型可用）
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {  // 如果属性标志匹配
					return i;  // 返回内存类型索引
				}
			}
			typeBits >>= 1;  // 右移一位，检查下一个内存类型
		}
		throw "Could not find a suitable memory type!";  // 如果未找到合适的内存类型，抛出异常
	}

	// Create the per-frame (in flight) and per (swapchain image) Vulkan synchronization primitives used in this example
	/**
	 * @brief 创建同步原语
	 * 创建每帧（飞行中）和每个交换链图像的 Vulkan 同步原语
	 * 包括栅栏（每帧）和信号量（呈现完成和渲染完成）
	 */
	void createSynchronizationPrimitives()
	{
		// Fences are per frame in flight
		// 栅栏是每帧飞行中的
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {		
			// Fence used to ensure that command buffer has completed exection before using it again
			// 栅栏用于确保命令缓冲区在执行完成之前不会被再次使用
			VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };  // 栅栏创建信息
			// Create the fences in signaled state (so we don't wait on first render of each command buffer)
			// 以已发出信号的状态创建栅栏（这样我们就不必等待每个命令缓冲区的第一次渲染）
			fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // 创建标志（已发出信号）
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));  // 创建栅栏
		}
		// Semaphores are used for correct command ordering within a queue
		// Used to ensure that image presentation is complete before starting to submit again
		// 信号量用于队列内正确的命令顺序
		// 用于确保图像呈现完成后再开始提交
		presentCompleteSemaphores.resize(MAX_CONCURRENT_FRAMES);  // 调整呈现完成信号量大小
		for (auto& semaphore : presentCompleteSemaphores) {  // 为每帧创建呈现完成信号量
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };  // 信号量创建信息
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));  // 创建信号量
		}
		// Render completion
		// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
		// 渲染完成
		// 信号量用于确保所有提交的命令在将图像提交到队列之前已完成
		renderCompleteSemaphores.resize(swapChain.images.size());  // 调整渲染完成信号量大小（每个交换链图像一个）
		for (auto& semaphore : renderCompleteSemaphores) {  // 为每个交换链图像创建渲染完成信号量
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };  // 信号量创建信息
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));  // 创建信号量
		}
	}

	// Command buffers are used to record commands to and are submitted to a queue for execution ("rendering")
	/**
	 * @brief 创建命令缓冲区
	 * 创建命令池并为每帧分配命令缓冲区
	 * 命令缓冲区用于记录命令并提交到队列执行（"渲染"）
	 */
	void createCommandBuffers()
	{
		// All command buffers are allocated from the same command pool
		// 所有命令缓冲区都从同一个命令池分配
		VkCommandPoolCreateInfo commandPoolCI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };  // 命令池创建信息
		commandPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;  // 队列族索引（交换链队列节点索引）
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // 创建标志（允许重置命令缓冲区）
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));  // 创建命令池
		// Allocate one command buffer per max. concurrent frame from above pool
		// 从上面的池中为每个最大并发帧分配一个命令缓冲区
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);  // 命令缓冲区分配信息
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));  // 分配命令缓冲区
	}

	// Prepare vertex and index buffers for an indexed triangle
	// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
	/**
	 * @brief 创建顶点缓冲区
	 * 为索引三角形准备顶点和索引缓冲区
	 * 还使用暂存缓冲区将它们上传到设备本地内存，并初始化顶点输入和属性绑定以匹配顶点着色器
	 */
	void createVertexBuffer()
	{
		// A note on memory management in Vulkan in general:
		//	This is a complex topic and while it's fine for an example application to small individual memory allocations that is not
		//	what should be done a real-world application, where you should allocate large chunks of memory at once instead.
		// 关于 Vulkan 中内存管理的一般说明：
		//	这是一个复杂的话题，虽然对于示例应用程序来说，小的单独内存分配是可以的，但这不是
		//	真实世界应用程序应该做的，您应该一次分配大块内存

		// Setup vertices
		// 设置顶点（三个顶点：右上角红色、左上角绿色、底部蓝色）
		const std::vector<Vertex> vertices{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },  // 右上角顶点（红色）
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // 左上角顶点（绿色）
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }   // 底部顶点（蓝色）
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);  // 顶点缓冲区大小

		// Setup indices
		// We do this for demonstration purposes, a triangle doesn't require indices to be rendered (because of the 1:1 mapping), but more complex shapes usually make use of indices
		// 设置索引
		// 我们这样做是为了演示目的，三角形不需要索引来渲染（因为 1:1 映射），但更复杂的形状通常使用索引
		std::vector<uint32_t> indices{ 0, 1, 2 };  // 索引数组（三个顶点按顺序索引）
		indexCount = static_cast<uint32_t>(indices.size());  // 索引数量
		uint32_t indexBufferSize = indexCount * sizeof(uint32_t);  // 索引缓冲区大小

		VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };  // 内存分配信息
		VkMemoryRequirements memReqs;  // 内存需求

		// Static data like vertex and index buffer should be stored on the device memory for optimal (and fastest) access by the GPU
		//
		// To achieve this we use so-called "staging buffers" :
		// - Create a buffer that's visible to the host (and can be mapped)
		// - Copy the data to this buffer
		// - Create another buffer that's local on the device (VRAM) with the same size
		// - Copy the data from the host to the device using a command buffer
		// - Delete the host visible (staging) buffer
		// - Use the device local buffers for rendering
		//
		// Note: On unified memory architectures where host (CPU) and GPU share the same memory, staging is not necessary
		// To keep this sample easy to follow, there is no check for that in place
		// 静态数据（如顶点和索引缓冲区）应存储在设备内存中，以便 GPU 以最佳（最快）方式访问
		//
		// 为了实现这一点，我们使用所谓的"暂存缓冲区"：
		// - 创建一个对主机可见（可以映射）的缓冲区
		// - 将数据复制到此缓冲区
		// - 创建另一个设备本地（VRAM）的相同大小的缓冲区
		// - 使用命令缓冲区将数据从主机复制到设备
		// - 删除主机可见（暂存）缓冲区
		// - 使用设备本地缓冲区进行渲染
		//
		// 注意：在主机（CPU）和 GPU 共享相同内存的统一内存架构上，暂存不是必需的
		// 为保持此示例易于理解，此处没有检查

		// Create the host visible staging buffer that we copy vertices and indices too, and from which we copy to the device
		// 创建主机可见的暂存缓冲区，我们将顶点和索引复制到其中，然后从其中复制到设备
		VulkanBuffer stagingBuffer;  // 暂存缓冲区
		VkBufferCreateInfo stagingBufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };  // 暂存缓冲区创建信息
		stagingBufferCI.size = vertexBufferSize + indexBufferSize;  // 缓冲区大小（顶点 + 索引）
		// Buffer is used as the copy source
		// 缓冲区用作复制源
		stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  // 用途：传输源
		// Create a host-visible buffer to copy the vertex data to (staging buffer)
		// 创建主机可见缓冲区以复制顶点数据（暂存缓冲区）
		VK_CHECK_RESULT(vkCreateBuffer(device, &stagingBufferCI, nullptr, &stagingBuffer.handle));  // 创建暂存缓冲区
		vkGetBufferMemoryRequirements(device, stagingBuffer.handle, &memReqs);  // 获取内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		// Request a host visible memory type that can be used to copy our data to
		// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
		// 请求可用于复制数据的主机可见内存类型
		// 还请求它是一致性的，以便在取消映射缓冲区后写入对 GPU 立即可见
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);  // 获取主机可见和一致性内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffer.memory));  // 分配内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer.handle, stagingBuffer.memory, 0));  // 绑定内存
		// Map the buffer and copy vertices and indices into it, this way we can use a single buffer as the source for both vertex and index GPU buffers
		// 映射缓冲区并将顶点和索引复制到其中，这样我们可以使用单个缓冲区作为顶点和索引 GPU 缓冲区的源
		uint8_t* data{ nullptr };  // 映射数据指针
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffer.memory, 0, memAlloc.allocationSize, 0, (void**)&data));  // 映射内存
		memcpy(data, vertices.data(), vertexBufferSize);  // 复制顶点数据
		memcpy(((char*)data) + vertexBufferSize, indices.data(), indexBufferSize);  // 复制索引数据（在顶点数据之后）

		// Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
		// 创建设备本地缓冲区，将（主机本地）顶点数据复制到其中，并用于渲染
		VkBufferCreateInfo vertexbufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };  // 顶点缓冲区创建信息
		vertexbufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // 用途：顶点缓冲区和传输目标
		vertexbufferCI.size = vertexBufferSize;  // 缓冲区大小
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexbufferCI, nullptr, &vertexBuffer.handle));  // 创建顶点缓冲区
		vkGetBufferMemoryRequirements(device, vertexBuffer.handle, &memReqs);  // 获取内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertexBuffer.memory));  // 分配内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertexBuffer.handle, vertexBuffer.memory, 0));  // 绑定内存

		// Create a device local buffer to which the (host local) index data will be copied and which will be used for rendering
		// 创建设备本地缓冲区，将（主机本地）索引数据复制到其中，并用于渲染
		VkBufferCreateInfo indexbufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };  // 索引缓冲区创建信息
		indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // 用途：索引缓冲区和传输目标
		indexbufferCI.size = indexBufferSize;  // 缓冲区大小
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indexBuffer.handle));  // 创建索引缓冲区
		vkGetBufferMemoryRequirements(device, indexBuffer.handle, &memReqs);  // 获取内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indexBuffer.memory));  // 分配内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, indexBuffer.handle, indexBuffer.memory, 0));  // 绑定内存

		// Buffer copies have to be submitted to a queue, so we need a command buffer for them
		// 缓冲区复制必须提交到队列，因此我们需要一个命令缓冲区
		VkCommandBuffer copyCmd;  // 复制命令缓冲区

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };  // 命令缓冲区分配信息
		cmdBufAllocateInfo.commandPool = commandPool;  // 命令池
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // 命令缓冲区级别（主命令缓冲区）
		cmdBufAllocateInfo.commandBufferCount = 1;  // 命令缓冲区数量
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));  // 分配命令缓冲区

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();  // 命令缓冲区开始信息
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));  // 开始记录命令缓冲区
		// Copy vertex and index buffers to the device
		// 将顶点和索引缓冲区复制到设备
		VkBufferCopy copyRegion{};  // 复制区域
		copyRegion.size = vertexBufferSize;  // 复制大小（顶点缓冲区）
		vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, vertexBuffer.handle, 1, &copyRegion);  // 复制顶点缓冲区
		copyRegion.size = indexBufferSize;  // 复制大小（索引缓冲区）
		// Indices are stored after the vertices in the source buffer, so we need to add an offset
		// 索引存储在源缓冲区中的顶点之后，因此我们需要添加偏移量
		copyRegion.srcOffset = vertexBufferSize;  // 源偏移量（顶点缓冲区大小）
		vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, indexBuffer.handle,	1, &copyRegion);  // 复制索引缓冲区
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));  // 结束记录命令缓冲区

		// Submit the command buffer to the queue to finish the copy
		// 将命令缓冲区提交到队列以完成复制
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };  // 提交信息
		submitInfo.commandBufferCount = 1;  // 命令缓冲区数量
		submitInfo.pCommandBuffers = &copyCmd;  // 命令缓冲区数组

		// Create fence to ensure that the command buffer has finished executing
		// 创建栅栏以确保命令缓冲区已完成执行
		VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };  // 栅栏创建信息
		VkFence fence;  // 栅栏句柄
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));  // 创建栅栏
		// Submit copies to the queue
		// 将复制提交到队列
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));  // 提交命令缓冲区
		// Wait for the fence to signal that command buffer has finished executing
		// 等待栅栏发出信号，表示命令缓冲区已完成执行
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));  // 等待栅栏
		vkDestroyFence(device, fence, nullptr);  // 销毁栅栏
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);  // 释放命令缓冲区

		// The fence made sure copies are finished, so we can safely delete the staging buffer
		// 栅栏确保复制已完成，因此我们可以安全地删除暂存缓冲区
		vkDestroyBuffer(device, stagingBuffer.handle, nullptr);  // 销毁暂存缓冲区
		vkFreeMemory(device, stagingBuffer.memory, nullptr);  // 释放暂存缓冲区内存
	}

	// Decriptors are used to pass data to shaders, for our sample we use a descriptor to pass parameters like matrices to the shader
	/**
	 * @brief 创建描述符
	 * 描述符用于将数据传递给着色器，本示例使用描述符将矩阵等参数传递给着色器
	 * 包括描述符池、描述符集布局和描述符集的创建
	 */
	void createDescriptors()
	{
		// Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
		// 描述符从池中分配，池告诉实现我们将使用多少和什么类型的描述符（最多）
		VkDescriptorPoolSize descriptorTypeCounts[1]{};  // 描述符类型计数数组
		// This example only one descriptor type (uniform buffer)
		// 本示例只有一种描述符类型（统一缓冲区）
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 描述符类型（统一缓冲区）
		// We have one buffer (and as such descriptor) per frame
		// 我们每帧有一个缓冲区（因此有一个描述符）
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;  // 描述符数量（最大并发帧数）
		// For additional types you need to add new entries in the type count list
		// E.g. for two combined image samplers :
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;
		// 对于其他类型，您需要在类型计数列表中添加新条目
		// 例如，对于两个组合图像采样器：
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;

		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		// 创建全局描述符池
		// 本示例中使用的所有描述符都从此池分配
		VkDescriptorPoolCreateInfo descriptorPoolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };  // 描述符池创建信息
		descriptorPoolCI.poolSizeCount = 1;  // 池大小数量
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;  // 池大小数组
		// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
		// Our sample will create one set per uniform buffer per frame
		// 设置可以从此池请求的最大描述符集数量（超出此限制的请求将导致错误）
		// 我们的示例将为每帧的每个统一缓冲区创建一个集
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;  // 最大描述符集数量
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));  // 创建描述符池

		// Descriptor set layouts define the interface between our application and the shader
		// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
		// So every shader binding should map to one descriptor set layout binding
		// Binding 0: Uniform buffer (Vertex shader)
		// 描述符集布局定义应用程序和着色器之间的接口
		// 基本上将不同的着色器阶段连接到用于绑定统一缓冲区、图像采样器等的描述符
		// 因此每个着色器绑定应该映射到一个描述符集布局绑定
		// 绑定 0：统一缓冲区（顶点着色器）
		VkDescriptorSetLayoutBinding layoutBinding{};  // 描述符集布局绑定
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 描述符类型（统一缓冲区）
		layoutBinding.descriptorCount = 1;  // 描述符数量
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // 着色器阶段标志（顶点着色器）

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };  // 描述符集布局创建信息
		descriptorLayoutCI.bindingCount = 1;  // 绑定数量
		descriptorLayoutCI.pBindings = &layoutBinding;  // 绑定数组
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));  // 创建描述符集布局

		// Where the descriptor set layout is the interface, the descriptor set points to actual data
		// Descriptors that are changed per frame need to be multiplied, so we can update descriptor n+1 while n is still used by the GPU, so we create one per max frame in flight
		// 描述符集布局是接口，描述符集指向实际数据
		// 每帧更改的描述符需要成倍增加，这样我们可以在 GPU 仍在使用 n 时更新描述符 n+1，因此我们为每个最大飞行帧创建一个
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {  // 为每帧创建描述符集
			VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };  // 描述符集分配信息
			allocInfo.descriptorPool = descriptorPool;  // 描述符池
			allocInfo.descriptorSetCount = 1;  // 描述符集数量
			allocInfo.pSetLayouts = &descriptorSetLayout;  // 描述符集布局数组
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));  // 分配描述符集

			// Update the descriptor set determining the shader binding points
			// For every binding point used in a shader there needs to be one
			// descriptor set matching that binding point
			// 更新描述符集，确定着色器绑定点
			// 对于着色器中使用的每个绑定点，需要有一个匹配该绑定点的描述符集
			VkWriteDescriptorSet writeDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };  // 写入描述符集

			// The buffer's information is passed using a descriptor info structure
			// 使用描述符信息结构传递缓冲区的信息
			VkDescriptorBufferInfo bufferInfo{};  // 描述符缓冲区信息
			bufferInfo.buffer = uniformBuffers[i].handle;  // 缓冲区句柄
			bufferInfo.range = sizeof(ShaderData);  // 缓冲区范围（着色器数据大小）

			// Binding 0 : Uniform buffer
			// 绑定 0：统一缓冲区
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;  // 目标描述符集
			writeDescriptorSet.descriptorCount = 1;  // 描述符数量
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 描述符类型（统一缓冲区）
			writeDescriptorSet.pBufferInfo = &bufferInfo;  // 缓冲区信息数组
			writeDescriptorSet.dstBinding = 0;  // 目标绑定（绑定 0）
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);  // 更新描述符集
		}
	}

	// Create the depth (and stencil) buffer attachments
	// While we don't do any depth testing in this sample, having depth testing is very common so it's a good idea to learn it from the very start
	/**
	 * @brief 设置深度模板缓冲区
	 * 创建深度（和模板）缓冲区附件
	 * 虽然本示例不进行任何深度测试，但深度测试非常常见，因此从一开始学习它是一个好主意
	 * 注意：这是基类中虚函数的重写
	 */
	void setupDepthStencil() override
	{
		// Create an optimal tiled image used as the depth stencil attachment
		// 创建用作深度模板附件的最优平铺图像
		VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };  // 图像创建信息
		imageCI.imageType = VK_IMAGE_TYPE_2D;  // 图像类型（2D）
		imageCI.format = depthFormat;  // 图像格式（深度格式）
		imageCI.extent = { width, height, 1 };  // 图像范围（宽度、高度、深度）
		imageCI.mipLevels = 1;  // Mip 级别数量（1 级）
		imageCI.arrayLayers = 1;  // 数组层数（1 层）
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;  // 采样数（单采样）
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;  // 平铺模式（最优）
		imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;  // 用途（深度模板附件）
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // 初始布局（未定义）
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));  // 创建图像

		// Allocate memory for the image (device local) and bind it to our image
		// 为图像分配内存（设备本地）并将其绑定到我们的图像
		VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };  // 内存分配信息
		VkMemoryRequirements memReqs;  // 内存需求
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);  // 获取图像内存需求
		memAlloc.allocationSize = memReqs.size;  // 分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));  // 分配内存
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));  // 绑定内存到图像

		// Create a view for the depth stencil image
		// Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
		// This allows for multiple views of one image with differing ranges (e.g. for different layers)
		// 为深度模板图像创建视图
		// 在 Vulkan 中，图像不是直接访问的，而是通过由子资源范围描述的视图访问
		// 这允许一个图像有多个具有不同范围的视图（例如，用于不同的层）
		VkImageViewCreateInfo depthStencilViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };  // 图像视图创建信息
		depthStencilViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;  // 视图类型（2D）
		depthStencilViewCI.format = depthFormat;  // 视图格式（深度格式）
		depthStencilViewCI.subresourceRange = {};  // 子资源范围
		depthStencilViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;  // 方面掩码（深度方面）
		// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT)
		// 模板方面应该只在深度 + 模板格式上设置（VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT）
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {  // 如果深度格式支持模板
			depthStencilViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;  // 添加模板方面掩码
		}
		depthStencilViewCI.subresourceRange.baseMipLevel = 0;  // 基础 Mip 级别（0）
		depthStencilViewCI.subresourceRange.levelCount = 1;  // Mip 级别数量（1）
		depthStencilViewCI.subresourceRange.baseArrayLayer = 0;  // 基础数组层（0）
		depthStencilViewCI.subresourceRange.layerCount = 1;  // 数组层数（1）
		depthStencilViewCI.image = depthStencil.image;  // 图像句柄
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilViewCI, nullptr, &depthStencil.view));  // 创建图像视图
	}

	// Vulkan loads its shaders from an immediate binary representation called SPIR-V
	// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
	// This function loads such a shader from a binary file and returns a shader module structure
	/**
	 * @brief 加载 SPIR-V 着色器
	 * Vulkan 从称为 SPIR-V 的即时二进制表示加载着色器
	 * 着色器从例如 GLSL 离线编译，使用参考 glslang 编译器
	 * 此函数从二进制文件加载此类着色器并返回着色器模块结构
	 * @param filename 着色器文件路径（SPIR-V 二进制文件）
	 * @return 着色器模块句柄，失败时返回 VK_NULL_HANDLE
	 */
	VkShaderModule loadSPIRVShader(const std::string& filename)
	{
		size_t shaderSize;  // 着色器代码大小
		char* shaderCode{ nullptr };  // 着色器代码缓冲区

#if defined(__ANDROID__)
		// Load shader from compressed asset
		// 从压缩资源加载着色器（Android 平台）
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);  // 打开资源文件
		assert(asset);  // 确保资源打开成功
		shaderSize = AAsset_getLength(asset);  // 获取资源文件长度
		assert(shaderSize > 0);  // 确保文件大小大于 0

		shaderCode = new char[shaderSize];  // 分配缓冲区
		AAsset_read(asset, shaderCode, shaderSize);  // 读取资源文件内容
		AAsset_close(asset);  // 关闭资源文件
#else
		// Load shader from file system (非 Android 平台)
		// 从文件系统加载着色器
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);  // 以二进制模式打开文件，定位到文件末尾

		if (is.is_open()) {
			shaderSize = is.tellg();  // 获取文件大小（当前位置即文件末尾）
			is.seekg(0, std::ios::beg);  // 定位到文件开头
			// Copy file contents into a buffer
			// 将文件内容复制到缓冲区
			shaderCode = new char[shaderSize];  // 分配缓冲区
			is.read(shaderCode, shaderSize);  // 读取文件内容
			is.close();  // 关闭文件
			assert(shaderSize > 0);  // 确保文件大小大于 0
		}
#endif
		if (shaderCode) {
			// Create a new shader module that will be used for pipeline creation
			// 创建将用于管道创建的新着色器模块
			VkShaderModuleCreateInfo shaderModuleCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };  // 着色器模块创建信息
			shaderModuleCI.codeSize = shaderSize;  // 着色器代码大小（字节数）
			shaderModuleCI.pCode = (uint32_t*)shaderCode;  // 着色器代码指针（SPIR-V 代码，按 32 位字对齐）

			VkShaderModule shaderModule;  // 着色器模块句柄
			VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));  // 创建着色器模块

			delete[] shaderCode;  // 释放着色器代码缓冲区

			return shaderModule;  // 返回着色器模块句柄
		} else {
			std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;  // 输出错误信息
			return VK_NULL_HANDLE;  // 返回空句柄
		}
	}

	/**
	 * @brief 创建图形管道
	 * 创建用于渲染的图形管道，包括所有管道状态（输入装配、光栅化、混合、视口、深度模板等）
	 * 注意：此管道使用 Vulkan 1.3 的动态渲染特性，不需要渲染通道和帧缓冲区
	 */
	void createPipeline()
	{
		// The pipeline layout is the interface telling the pipeline what type of descriptors will later be bound
		// 管道布局是接口，告诉管道稍后将绑定什么类型的描述符
		VkPipelineLayoutCreateInfo pipelineLayoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };  // 管道布局创建信息
		pipelineLayoutCI.setLayoutCount = 1;  // 描述符集布局数量（1 个）
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;  // 描述符集布局数组
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));  // 创建管道布局

		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// 创建本示例中使用的图形管道
		// Vulkan 使用渲染管道的概念来封装固定状态，取代 OpenGL 的复杂状态机
		// 管道随后在 GPU 上存储和哈希，使管道更改非常快

		VkGraphicsPipelineCreateInfo pipelineCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };  // 图形管道创建信息
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		// 此管道使用的布局（可以在使用相同布局的多个管道之间共享）
		pipelineCI.layout = pipelineLayout;  // 管道布局

		// Construct the different states making up the pipeline
		// 构建组成管道的不同状态

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		// 输入装配状态描述如何装配图元
		// 此管道将顶点数据装配为三角形列表（尽管我们只使用一个三角形）
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };  // 输入装配状态创建信息
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  // 图元拓扑（三角形列表）

		// Rasterization state
		// 光栅化状态
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };  // 光栅化状态创建信息
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;  // 多边形模式（填充）
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;  // 剔除模式（不剔除）
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // 正面朝向（逆时针）
		rasterizationStateCI.depthClampEnable = VK_FALSE;  // 深度夹紧（禁用）
		rasterizationStateCI.rasterizerDiscardEnable = VK_FALSE;  // 光栅化丢弃（禁用）
		rasterizationStateCI.depthBiasEnable = VK_FALSE;  // 深度偏移（禁用）
		rasterizationStateCI.lineWidth = 1.0f;  // 线宽（1.0）

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used)
		// 颜色混合状态描述如何计算混合因子（如果使用）
		// 每个颜色附件需要一个混合附件状态（即使不使用混合）
		VkPipelineColorBlendAttachmentState blendAttachmentState{};  // 颜色混合附件状态
		blendAttachmentState.colorWriteMask = 0xf;  // 颜色写入掩码（RGBA 全部启用，0xf = 1111）
		blendAttachmentState.blendEnable = VK_FALSE;  // 混合启用（禁用）
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };  // 颜色混合状态创建信息
		colorBlendStateCI.attachmentCount = 1;  // 附件数量（1 个颜色附件）
		colorBlendStateCI.pAttachments = &blendAttachmentState;  // 混合附件状态数组

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		// 视口状态设置此管道中使用的视口和裁剪矩形数量
		// 注意：这实际上被动态状态覆盖（见下文）
		VkPipelineViewportStateCreateInfo viewportStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };  // 视口状态创建信息
		viewportStateCI.viewportCount = 1;  // 视口数量（1 个）
		viewportStateCI.scissorCount = 1;  // 裁剪矩形数量（1 个）

		// Enable dynamic states
		// Most states are baked into the pipeline, but there is somee state that can be dynamically changed within the command buffer to mak e things easuer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer
		// 启用动态状态
		// 大多数状态都烘焙到管道中，但有一些状态可以在命令缓冲区中动态更改以使事情更容易
		// 为了能够更改这些，我们需要指定将使用此管道更改哪些动态状态。它们的实际状态稍后在命令缓冲区中设置
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };  // 动态状态向量
		VkPipelineDynamicStateCreateInfo dynamicStateCI{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };  // 动态状态创建信息
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();  // 动态状态数组
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());  // 动态状态数量

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		// 深度和模板状态，包含深度和模板比较和测试操作
		// 我们只使用深度测试，并希望启用深度测试和写入，并使用小于或等于进行比较
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };  // 深度模板状态创建信息
		depthStencilStateCI.depthTestEnable = VK_TRUE;  // 深度测试启用
		depthStencilStateCI.depthWriteEnable = VK_TRUE;  // 深度写入启用
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;  // 深度比较操作（小于或等于）
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;  // 深度边界测试（禁用）
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;  // 背面模板失败操作（保持）
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;  // 背面模板通过操作（保持）
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;  // 背面模板比较操作（始终通过）
		depthStencilStateCI.stencilTestEnable = VK_FALSE;  // 模板测试（禁用）
		depthStencilStateCI.front = depthStencilStateCI.back;  // 正面模板状态（与背面相同）

		// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		// 本示例不使用多重采样（用于抗锯齿），但仍必须设置状态并传递给管道
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };  // 多重采样状态创建信息
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // 光栅化采样数（单采样）

		// Vertex input descriptions
		// Specifies the vertex input parameters for a pipeline
		// 顶点输入描述
		// 指定管道的顶点输入参数

		// Vertex input binding
		// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
		// 顶点输入绑定
		// 本示例在绑定点 0 使用单个顶点输入绑定（参见 vkCmdBindVertexBuffers）
		VkVertexInputBindingDescription vertexInputBinding{};  // 顶点输入绑定描述
		vertexInputBinding.binding = 0;  // 绑定点（0）
		vertexInputBinding.stride = sizeof(Vertex);  // 步长（顶点结构体大小）
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // 输入速率（每个顶点）

		// Input attribute bindings describe shader attribute locations and memory layouts
		// 输入属性绑定描述着色器属性位置和内存布局
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs{};  // 顶点输入属性描述数组（2 个属性）
		// These match the following shader layout (see triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		// 这些匹配以下着色器布局（参见 triangle.vert）：
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		// Attribute location 0: Position
		// 属性位置 0：位置
		vertexInputAttributs[0].binding = 0;  // 绑定点（0）
		vertexInputAttributs[0].location = 0;  // 着色器中的位置（location = 0）
		// Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
		// 位置属性是三个 32 位有符号（SFLOAT）浮点数（R32 G32 B32）
		vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // 格式（R32G32B32 有符号浮点）
		vertexInputAttributs[0].offset = offsetof(Vertex, position);  // 偏移量（顶点结构体中位置字段的偏移）
		// Attribute location 1: Color
		// 属性位置 1：颜色
		vertexInputAttributs[1].binding = 0;  // 绑定点（0）
		vertexInputAttributs[1].location = 1;  // 着色器中的位置（location = 1）
		// Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
		// 颜色属性是三个 32 位有符号（SFLOAT）浮点数（R32 G32 B32）
		vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // 格式（R32G32B32 有符号浮点）
		vertexInputAttributs[1].offset = offsetof(Vertex, color);  // 偏移量（顶点结构体中颜色字段的偏移）

		// Vertex input state used for pipeline creation
		// 用于管道创建的顶点输入状态
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };  // 顶点输入状态创建信息
		vertexInputStateCI.vertexBindingDescriptionCount = 1;  // 顶点绑定描述数量（1 个）
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;  // 顶点绑定描述数组
		vertexInputStateCI.vertexAttributeDescriptionCount = 2;  // 顶点属性描述数量（2 个）
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributs.data();  // 顶点属性描述数组

		// Shaders
		// 着色器
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};  // 着色器阶段创建信息数组（顶点和片段）

		// Vertex shader
		// 顶点着色器
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;  // 结构体类型
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;  // 着色器阶段（顶点着色器）
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv");  // 加载顶点着色器模块
		shaderStages[0].pName = "main";  // 入口点函数名
		assert(shaderStages[0].module != VK_NULL_HANDLE);  // 确保着色器模块加载成功

		// Fragment shader
		// 片段着色器
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;  // 结构体类型
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;  // 着色器阶段（片段着色器）
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv");  // 加载片段着色器模块
		shaderStages[1].pName = "main";  // 入口点函数名
		assert(shaderStages[1].module != VK_NULL_HANDLE);  // 确保着色器模块加载成功

		// Set pipeline shader stage info
		// 设置管道着色器阶段信息
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());  // 着色器阶段数量（2 个）
		pipelineCI.pStages = shaderStages.data();  // 着色器阶段数组

		// Attachment information for dynamic rendering
		// 动态渲染的附件信息（Vulkan 1.3 特性）
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };  // 管道渲染创建信息
		pipelineRenderingCI.colorAttachmentCount = 1;  // 颜色附件数量（1 个）
		pipelineRenderingCI.pColorAttachmentFormats = &swapChain.colorFormat;  // 颜色附件格式数组
		pipelineRenderingCI.depthAttachmentFormat = depthFormat;  // 深度附件格式
		pipelineRenderingCI.stencilAttachmentFormat = depthFormat;  // 模板附件格式

		// Assign the pipeline states to the pipeline creation info structure
		// 将管道状态分配给管道创建信息结构
		pipelineCI.pVertexInputState = &vertexInputStateCI;  // 顶点输入状态
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;  // 输入装配状态
		pipelineCI.pRasterizationState = &rasterizationStateCI;  // 光栅化状态
		pipelineCI.pColorBlendState = &colorBlendStateCI;  // 颜色混合状态
		pipelineCI.pMultisampleState = &multisampleStateCI;  // 多重采样状态
		pipelineCI.pViewportState = &viewportStateCI;  // 视口状态
		pipelineCI.pDepthStencilState = &depthStencilStateCI;  // 深度模板状态
		pipelineCI.pDynamicState = &dynamicStateCI;  // 动态状态
		pipelineCI.pNext = &pipelineRenderingCI;  // 扩展指针（动态渲染信息）

		// Create rendering pipeline using the specified states
		// 使用指定的状态创建渲染管道
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));  // 创建图形管道

		// Shader modules can safely be destroyed when the pipeline has been created
		// 一旦管道创建完成，着色器模块就可以安全地销毁
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);  // 销毁顶点着色器模块
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);  // 销毁片段着色器模块
	}

	/**
	 * @brief 创建统一缓冲区
	 * 准备并初始化每帧的统一缓冲区块，包含着色器统一变量
	 * 注意：Vulkan 中不再存在像 OpenGL 中的单个统一变量。所有着色器统一变量都通过统一缓冲区块传递
	 */
	void createUniformBuffers()
	{
		// Prepare and initialize the per-frame uniform buffer blocks containing shader uniforms
		// Single uniforms like in OpenGL are no longer present in Vulkan. All shader uniforms are passed via uniform buffer blocks
		// 准备并初始化每帧的统一缓冲区块，包含着色器统一变量
		// Vulkan 中不再存在像 OpenGL 中的单个统一变量。所有着色器统一变量都通过统一缓冲区块传递
		VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };  // 缓冲区创建信息
		bufferInfo.size = sizeof(ShaderData);  // 缓冲区大小（着色器数据大小）
		// This buffer will be used as a uniform buffer
		// 此缓冲区将用作统一缓冲区
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;  // 用途：统一缓冲区

		// Create the buffers
		// 创建缓冲区（每帧一个）
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].handle));  // 创建统一缓冲区
			// Get memory requirements including size, alignment and memory type based on the buffer type we request (uniform buffer)
			// 获取内存需求，包括大小、对齐和内存类型，基于我们请求的缓冲区类型（统一缓冲区）
			VkMemoryRequirements memReqs;  // 内存需求
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].handle, &memReqs);  // 查询缓冲区内存需求
			VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };  // 内存分配信息
			// Note that we use the size we got from the memory requirements and not the actual buffer size, as the former may be larger due to alignment requirements of the device
			// 注意：我们使用从内存需求获得的大小，而不是实际缓冲区大小，因为前者可能由于设备的对齐要求而更大
			allocInfo.allocationSize = memReqs.size;  // 设置分配大小
			// Get the memory type index that supports host visible memory access
			// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
			// We also want the buffer to be host coherent so we don't have to flush (or sync after every update).
			// 获取支持主机可见内存访问的内存类型索引
			// 大多数实现提供多种内存类型，选择正确的内存类型进行分配至关重要
			// 我们还希望缓冲区是主机一致的，这样我们就不必在每次更新后刷新（或同步）
			allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);  // 获取主机可见和一致性内存类型
			// Allocate memory for the uniform buffer
			// 为统一缓冲区分配内存
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));  // 分配内存
			// Bind memory to buffer
			// 将内存绑定到缓冲区
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].handle, uniformBuffers[i].memory, 0));  // 绑定内存
			// We map the buffer once, so we can update it without having to map it again
			// 我们映射缓冲区一次，这样我们就可以更新它而不必再次映射
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));  // 映射内存，保存映射指针
		}

	}

	/**
	 * @brief 准备示例
	 * 初始化所有 Vulkan 资源，包括同步原语、命令缓冲区、顶点缓冲区、统一缓冲区、描述符和管道
	 * 注意：这是基类中虚函数的重写
	 */
	void prepare() override
	{
		VulkanExampleBase::prepare();  // 调用基类准备方法（初始化 Vulkan 实例、设备、交换链等）
		createSynchronizationPrimitives();  // 创建同步原语（栅栏和信号量）
		createCommandBuffers();  // 创建命令缓冲区和命令池
		createVertexBuffer();  // 创建顶点和索引缓冲区
		createUniformBuffers();  // 创建统一缓冲区
		createDescriptors();  // 创建描述符（池、布局、集）
		createPipeline();  // 创建图形管道（使用动态渲染）
		prepared = true;  // 标记为已准备完成
	}

	/**
	 * @brief 渲染一帧
	 * 执行完整的渲染循环，使用 Vulkan 1.3 的动态渲染特性
	 * 注意：这是基类中虚函数的重写
	 * 与传统的渲染通道不同，动态渲染不需要预先创建渲染通道和帧缓冲区
	 */
	void render() override
	{
		// Use a fence to wait until the command buffer has finished execution before using it again
		// 使用栅栏等待命令缓冲区完成执行，然后才能再次使用它
		//渲染当前帧之前，先等待该帧对应的栅栏被 GPU 信号标记（即上一次使用该栅栏的 GPU 命令已执行完成）。VK_TRUE 表示等待栅栏状态变为「已触发」，UINT64_MAX 表示无限等待，确保 GPU 资源就绪后再进行后
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);  // 等待当前帧的栅栏
		//栅栏是「一次性触发」的（触发后状态保持为「已完成」，无法再次用于等待），因此在等待完成后、重新使用该栅栏之前，必须调用 vkResetFences 将其状态重置为「未触发」，为当前帧的 GPU 命令提交做准备。
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));  // 重置栅栏，准备下一帧使用

		// Get the next swap chain image from the implementation
		// Note that the implementation is free to return the images in any order, so we must use the acquire function and can't just cycle through the images/imageIndex on our own
		// 从实现获取下一个交换链图像
		// 注意：实现可以按任何顺序返回图像，因此我们必须使用获取函数，不能自己简单地循环图像/图像索引
		uint32_t imageIndex{ 0 };  // 交换链图像索引
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);  // 获取下一个交换链图像
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {  // 如果交换链已过期（例如窗口大小改变）
			windowResize();  // 调整窗口大小（重新创建交换链）
			return;
		} else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {  // 如果获取失败且不是次优状态
			throw "Could not acquire the next swap chain image!";  // 抛出异常
		}

		// Update the uniform buffer for the next frame
		// 更新下一帧的统一缓冲区
		ShaderData shaderData{};  // 着色器数据
		shaderData.projectionMatrix = camera.matrices.perspective;  // 投影矩阵（从相机获取）
		shaderData.viewMatrix = camera.matrices.view;  // 视图矩阵（从相机获取）
		shaderData.modelMatrix = glm::mat4(1.0f);  // 模型矩阵（单位矩阵，无变换）
		// Copy the current matrices to the current frame's uniform buffer. As we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU.
		// 将当前矩阵复制到当前帧的统一缓冲区。由于我们为统一缓冲区请求了主机一致性内存类型，写入对 GPU 立即可见
		memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));  // 复制着色器数据到映射内存

		// Build the command buffer for the next frame to render
		// 构建下一帧要渲染的命令缓冲区
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);  // 重置命令缓冲区，准备记录新命令
		VkCommandBufferBeginInfo cmdBufInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };  // 命令缓冲区开始信息
		const VkCommandBuffer commandBuffer = commandBuffers[currentFrame];  // 当前帧的命令缓冲区
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));  // 开始记录命令缓冲区

		/**
		**变化点 2：layout transition 从“隐式”变“显式”（最容易卡住）**
		- 传统路线里，你在 RenderPass 里写 `initialLayout/finalLayout` + subpass dependency，驱动会在 renderpass 边界隐式完成很多转换
		- 动态渲染没有 RenderPass 边界，所以你必须在命令缓冲区里显式写 barrier：
		  - **渲染前**：swapchain image / depth image → `ATTACHMENT_OPTIMAL`
		  - **渲染后呈现前**：swapchain image → `PRESENT_SRC_KHR`

		> 小提示：你看到 `trianglevulkan13` 的 `render()` 一开始就插入两次 barrier（颜色/深度），结束渲染后又插一次 barrier（颜色转 present），这就是动态渲染“流程变化”的核心体现。
		*/
		// With dynamic rendering we need to explicitly add layout transitions by using barriers, this set of barriers prepares the color and depth images for output
		// 使用动态渲染时，我们需要使用屏障显式添加布局转换，这组屏障准备颜色和深度图像以进行输出
		// 颜色图像：从未定义布局转换到附件最优布局
		vks::tools::insertImageMemoryBarrier(commandBuffer, swapChain.images[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		// 深度模板图像：从未定义布局转换到附件最优布局
		vks::tools::insertImageMemoryBarrier(commandBuffer, depthStencil.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

		// New structures are used to define the attachments used in dynamic rendering
		// 使用新结构定义动态渲染中使用的附件
		// Color attachment
		// 颜色附件
		VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };  // 渲染附件信息（颜色）
		colorAttachment.imageView = swapChain.imageViews[imageIndex];  // 图像视图（交换链图像视图）
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // 图像布局（颜色附件最优）
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // 加载操作（清除）
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // 存储操作（存储）
		colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.2f, 0.0f };  // 清除值（深蓝色，RGBA）
		// Depth/stencil attachment
		// 深度/模板附件
		VkRenderingAttachmentInfo depthStencilAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };  // 渲染附件信息（深度模板）
		depthStencilAttachment.imageView = depthStencil.view;  // 图像视图（深度模板视图）
		depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // 图像布局（深度模板附件最优）
		depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // 加载操作（清除）
		depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // 存储操作（不关心）
		depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };  // 清除值（深度 = 1.0，模板 = 0）

		VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };  // 渲染信息（Vulkan 1.3 动态渲染）
		renderingInfo.renderArea = { 0, 0, width, height };  // 渲染区域（整个窗口）
		renderingInfo.layerCount = 1;  // 层数（1 层）
		renderingInfo.colorAttachmentCount = 1;  // 颜色附件数量（1 个）
		renderingInfo.pColorAttachments = &colorAttachment;  // 颜色附件数组
		renderingInfo.pDepthAttachment = &depthStencilAttachment;  // 深度附件
		renderingInfo.pStencilAttachment = &depthStencilAttachment;  // 模板附件

		// Start a dynamic rendering section
		// 开始动态渲染部分（Vulkan 1.3 特性，替代传统的 vkCmdBeginRenderPass）
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		// Update dynamic viewport state
		// 更新动态视口状态
		VkViewport viewport{ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };  // 视口（X, Y, 宽度, 高度, 最小深度, 最大深度）
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);  // 设置视口（动态状态）
		// Update dynamic scissor state
		// 更新动态裁剪矩形状态
		VkRect2D scissor{ 0, 0, width, height };  // 裁剪矩形（偏移 X, 偏移 Y, 宽度, 高度）
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);  // 设置裁剪矩形（动态状态）
		// Bind descriptor set for the current frame's uniform buffer, so the shader uses the data from that buffer for this draw
		// 绑定当前帧统一缓冲区的描述符集，以便着色器在此绘制中使用该缓冲区的数据
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);  // 绑定描述符集
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		// 管道（状态对象）包含渲染管道的所有状态，绑定它将设置管道创建时指定的所有状态
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);  // 绑定图形管道
		// Bind triangle vertex buffer (contains position and colors)
		// 绑定三角形顶点缓冲区（包含位置和颜色）
		VkDeviceSize offsets[1]{ 0 };  // 顶点缓冲区偏移数组
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.handle, offsets);  // 绑定顶点缓冲区（绑定点 0）
		// Bind triangle index buffer
		// 绑定三角形索引缓冲区
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);  // 绑定索引缓冲区（32 位无符号整数类型）
		// Draw indexed triangle
		// 绘制索引三角形
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);  // 绘制索引图元（索引数量，实例数量，第一个索引，顶点偏移，第一个实例）
		// Finish the current dynamic rendering section
		// 结束当前动态渲染部分（Vulkan 1.3 特性，替代传统的 vkCmdEndRenderPass）
		vkCmdEndRendering(commandBuffer);

		// This barrier prepares the color image for presentation, we don't need to care for the depth image
		// 此屏障准备颜色图像以进行呈现，我们不需要关心深度图像
		// 颜色图像：从附件最优布局转换到呈现源布局
		vks::tools::insertImageMemoryBarrier(commandBuffer, swapChain.images[imageIndex], VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));  // 结束记录命令缓冲区

		// Submit the command buffer to the graphics queue
		// 将命令缓冲区提交到图形队列

		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		// 队列提交将等待的管道阶段（通过 pWaitSemaphores）
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 等待阶段掩码（颜色附件输出阶段）
		// The submit info structure specifies a command buffer queue submission batch
		// 提交信息结构指定命令缓冲区队列提交批次
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };  // 提交信息
		submitInfo.pWaitDstStageMask = &waitStageMask;      // Pointer to the list of pipeline stages that the semaphore waits will occur at
		                                                    // 信号量等待将发生的管道阶段列表的指针
		submitInfo.pCommandBuffers = &commandBuffer;		// Command buffers(s) to execute in this batch (submission)
		                                                    // 在此批次（提交）中执行的命令缓冲区
		submitInfo.commandBufferCount = 1;                  // We submit a single command buffer
		                                                    // 我们提交单个命令缓冲区

		// Semaphore to wait upon before the submitted command buffer starts executing
		// 在提交的命令缓冲区开始执行之前等待的信号量
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];  // 等待信号量（呈现完成信号量）
		submitInfo.waitSemaphoreCount = 1;  // 等待信号量数量
		// Semaphore to be signaled when command buffers have completed
		// 命令缓冲区完成时发出信号的信号量
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[imageIndex];  // 信号信号量（渲染完成信号量）
		submitInfo.signalSemaphoreCount = 1;  // 信号信号量数量

		// Submit to the graphics queue passing a wait fence
		// 提交到图形队列，传递等待栅栏
		//将当前帧对应的栅栏传递给 vkQueueSubmit 的最后一个参数，当该提交中的所有 GPU 命令（命令缓冲区中的渲染指令）全部执行完成后，GPU 会自动将该栅栏标记为「已触发」，供下一循环（该帧再次被选中时）的 vkWaitForFences 等待使用。
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));  // 提交命令缓冲区到队列

		// Present the current frame buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted
		// 将当前帧缓冲区呈现到交换链
		// 将提交信息中命令缓冲区提交发出的信号量作为交换链呈现的等待信号量传递
		// 这确保在所有命令都已提交之前，图像不会呈现到窗口系统
		VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };  // 呈现信息
		presentInfo.waitSemaphoreCount = 1;  // 等待信号量数量
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[imageIndex];  // 等待信号量（渲染完成信号量）
		presentInfo.swapchainCount = 1;  // 交换链数量
		presentInfo.pSwapchains = &swapChain.swapChain;  // 交换链数组
		presentInfo.pImageIndices = &imageIndex;  // 图像索引数组
		result = vkQueuePresentKHR(queue, &presentInfo);  // 呈现图像到交换链
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {  // 如果交换链已过期或次优
			windowResize();  // 调整窗口大小（重新创建交换链）
		} else if (result != VK_SUCCESS) {  // 如果呈现失败
			throw "Could not present the image to the swap chain!";  // 抛出异常
		}

		// Select the next frame to render to, based on the max. no. of concurrent frames
		// 根据最大并发帧数选择下一个要渲染的帧
		currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;  // 循环选择下一帧（帧重叠）


		/**
		第 1 次进入 render 循环：
		初始 currentFrame=0，此时操作的是 waitFences[0]；
		调用 vkWaitForFences(device, 1, &waitFences[0], VK_TRUE, UINT64_MAX)，由于是首次使用该栅栏，它在程序初始化时被创建并处于「未触发（unsignaled）」状态；
		但对于首次执行，vkWaitForFences 传入 waitFences[0] 会直接通过，不产生任何阻塞（因为该栅栏从未被绑定到任何 GPU 任务，没有需要等待的 GPU 执行结果）；
		后续完成栅栏重置、命令提交、呈现请求，最后执行 currentFrame = (0 + 1) % 2 = 1，切换为 1。
		
		第 2 次进入 render 循环：
		此时 currentFrame=1，操作的是 waitFences[1]；
		同样，waitFences[1] 也是首次使用，处于初始化后的「未触发」状态，vkWaitForFences 直接通过，不阻塞；
		最后执行 currentFrame = (1 + 1) % 2 = 0，切换为 0。

		第 3 次进入 render 循环：
		此时 currentFrame=0，操作的是 waitFences[0]； 等待第一次渲染结束 这样就实现了帧重叠。
	
		*/
	}

	// Override these as otherwise the base class would generate frame buffers and render passes
	// 重写这些方法，否则基类会生成帧缓冲区和渲染通道
	// 注意：使用动态渲染时不需要帧缓冲区和渲染通道
	/**
	 * @brief 设置帧缓冲区（空实现）
	 * 使用动态渲染时不需要帧缓冲区，因此此方法为空
	 */
	void setupFrameBuffer() override {}
	/**
	 * @brief 设置渲染通道（空实现）
	 * 使用动态渲染时不需要渲染通道，因此此方法为空
	 */
	void setupRenderPass() override {}
};

// OS specific main entry points
// Most of the code base is shared for the different supported operating systems, but stuff like message handling differs
// 操作系统特定的主入口点
// 大部分代码库在不同支持的操作系统之间共享，但消息处理等内容有所不同

#if defined(_WIN32)
// Windows entry point
// Windows 入口点
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
/**
 * @brief Windows 窗口过程
 * 处理 Windows 窗口消息
 * @param hWnd 窗口句柄
 * @param uMsg 消息类型
 * @param wParam 消息参数 1
 * @param lParam 消息参数 2
 * @return 消息处理结果
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)  // 如果示例已创建
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);  // 处理窗口消息
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));  // 调用默认窗口过程
}
/**
 * @brief Windows 主入口点
 * 初始化 Vulkan 示例并启动渲染循环
 * @param hInstance 应用程序实例句柄
 * @param hPrevInstance 前一个实例句柄（已弃用）
 * @param lpCmdLine 命令行字符串
 * @param nCmdShow 窗口显示方式
 * @return 程序退出代码
 */
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int)
{
	for (size_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };  // 保存命令行参数
	vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
	vulkanExample->initVulkan();  // 初始化 Vulkan（实例、设备、交换链等）
	vulkanExample->setupWindow(hInstance, WndProc);  // 设置窗口（创建窗口并注册窗口过程）
	vulkanExample->prepare();  // 准备示例（创建资源）
	vulkanExample->renderLoop();  // 启动渲染循环
	delete(vulkanExample);  // 清理示例实例
	return 0;  // 返回成功
}

#elif defined(__ANDROID__)
// Android entry point
// Android 入口点
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
/**
 * @brief Android 主入口点
 * 初始化 Vulkan 示例并启动渲染循环
 * @param state Android 应用程序状态
 */
void android_main(android_app* state)
{
	vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
	state->userData = vulkanExample;  // 将示例指针保存到用户数据
	state->onAppCmd = VulkanExample::handleAppCommand;  // 设置应用程序命令回调
	state->onInputEvent = VulkanExample::handleAppInput;  // 设置输入事件回调
	androidApp = state;  // 保存全局 Android 应用程序指针
	vulkanExample->renderLoop();  // 启动渲染循环（在循环中初始化 Vulkan）
	delete(vulkanExample);  // 清理示例实例
}
#elif defined(_DIRECT2DISPLAY)

// Linux entry point with direct to display wsi
// Direct to Displays (D2D) is used on embedded platforms
// Linux 入口点（直接到显示 WSI）
// 直接到显示（D2D）用于嵌入式平台
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
/**
 * @brief 事件处理函数（空实现，直接显示不需要事件处理）
 */
static void handleEvent()
{
}
/**
 * @brief Linux 直接显示主入口点
 * 初始化 Vulkan 示例并启动渲染循环（直接输出到显示器，无窗口系统）
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出代码
 */
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  // 保存命令行参数
	vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
	vulkanExample->initVulkan();  // 初始化 Vulkan（实例、设备、交换链等）
	vulkanExample->prepare();  // 准备示例（创建资源）
	vulkanExample->renderLoop();  // 启动渲染循环
	delete(vulkanExample);  // 清理示例实例
	return 0;  // 返回成功
}
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
// DirectFB 入口点
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
/**
 * @brief DirectFB 事件处理函数
 * 处理 DirectFB 窗口事件
 * @param event DirectFB 窗口事件
 */
static void handleEvent(const DFBWindowEvent *event)
{
	if (vulkanExample != NULL)  // 如果示例已创建
	{
		vulkanExample->handleEvent(event);  // 处理 DirectFB 事件
	}
}
/**
 * @brief DirectFB 主入口点
 * 初始化 Vulkan 示例并启动渲染循环
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出代码
 */
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  // 保存命令行参数
	vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
	vulkanExample->initVulkan();  // 初始化 Vulkan（实例、设备、交换链等）
	vulkanExample->setupWindow();  // 设置窗口（创建 DirectFB 窗口）
	vulkanExample->prepare();  // 准备示例（创建资源）
	vulkanExample->renderLoop();  // 启动渲染循环
	delete(vulkanExample);  // 清理示例实例
	return 0;  // 返回成功
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
// Wayland 入口点
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
/**
 * @brief Wayland 主入口点
 * 初始化 Vulkan 示例并启动渲染循环
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出代码
 */
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  // 保存命令行参数
	vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
	vulkanExample->initVulkan();  // 初始化 Vulkan（实例、设备、交换链等）
	vulkanExample->setupWindow();  // 设置窗口（创建 Wayland 窗口）
	vulkanExample->prepare();  // 准备示例（创建资源）
	vulkanExample->renderLoop();  // 启动渲染循环
	delete(vulkanExample);  // 清理示例实例
	return 0;  // 返回成功
}
#elif defined(__linux__) || defined(__FreeBSD__)

// Linux entry point
// Linux 入口点
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
#if defined(VK_USE_PLATFORM_XCB_KHR)
/**
 * @brief XCB 事件处理函数
 * 处理 XCB 窗口事件
 * @param event XCB 通用事件
 */
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)  // 如果示例已创建
	{
		vulkanExample->handleEvent(event);  // 处理 XCB 事件
	}
}
#else
/**
 * @brief 事件处理函数（空实现，无头模式不需要事件处理）
 */
static void handleEvent()
{
}
#endif
/**
 * @brief Linux/FreeBSD 主入口点
 * 初始化 Vulkan 示例并启动渲染循环
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出代码
 */
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  // 保存命令行参数
	vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
	vulkanExample->initVulkan();  // 初始化 Vulkan（实例、设备、交换链等）
	vulkanExample->setupWindow();  // 设置窗口（创建 XCB 窗口或无头窗口）
	vulkanExample->prepare();  // 准备示例（创建资源）
	vulkanExample->renderLoop();  // 启动渲染循环
	delete(vulkanExample);  // 清理示例实例
	return 0;  // 返回成功
}
#elif (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_EXAMPLE_XCODE_GENERATED)
// macOS/iOS 入口点（使用 MoltenVK 或 Metal）
VulkanExample *vulkanExample;  // Vulkan 示例全局指针
/**
 * @brief macOS/iOS 主入口点
 * 初始化 Vulkan 示例并启动渲染循环
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出代码
 */
int main(const int argc, const char *argv[])
{
	@autoreleasepool  // Objective-C 自动释放池
	{
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  // 保存命令行参数
		vulkanExample = new VulkanExample();  // 创建 Vulkan 示例实例
		vulkanExample->initVulkan();  // 初始化 Vulkan（实例、设备、交换链等）
		vulkanExample->setupWindow(nullptr);  // 设置窗口（创建 macOS/iOS 窗口）
		vulkanExample->prepare();  // 准备示例（创建资源）
		vulkanExample->renderLoop();  // 启动渲染循环
		delete(vulkanExample);  // 清理示例实例
	}
	return 0;  // 返回成功
}
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
// QNX Screen 入口点（使用宏定义）
VULKAN_EXAMPLE_MAIN()
#endif
