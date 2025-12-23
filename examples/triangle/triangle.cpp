/*
* Vulkan Example - Basic indexed triangle rendering
*
* Note:
*	This is a "pedal to the metal" example to show off how to get Vulkan up and displaying something
*	Contrary to the other examples, this one won't make use of helper functions or initializers
*	Except in a few cases (swap chain setup e.g.)
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
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
constexpr auto MAX_CONCURRENT_FRAMES = 2;

class VulkanExample : public VulkanExampleBase
{
public:
	// Vertex layout used in this example
	// 本示例中使用的顶点布局
	struct Vertex {
		float position[3];  // 顶点位置（3D 坐标）
		float color[3];      // 顶点颜色（RGB）
	};

	// Vertex buffer and attributes
	// 顶点缓冲区和属性
	struct {
		VkDeviceMemory memory{ VK_NULL_HANDLE }; // Handle to the device memory for this buffer
		                                          // 此缓冲区的设备内存句柄
		VkBuffer buffer{ VK_NULL_HANDLE };		 // Handle to the Vulkan buffer object that the memory is bound to
		                                          // 内存绑定到的 Vulkan 缓冲区对象句柄
	} vertices;

	// Index buffer
	// 索引缓冲区
	struct {
		VkDeviceMemory memory{ VK_NULL_HANDLE };  // 索引缓冲区的设备内存句柄
		VkBuffer buffer{ VK_NULL_HANDLE };        // 索引缓冲区对象句柄
		uint32_t count{ 0 };                       // 索引数量
	} indices;

	// Uniform buffer block object
	// 统一缓冲区块对象
	struct UniformBuffer {
		VkDeviceMemory memory{ VK_NULL_HANDLE };  // 统一缓冲区的设备内存句柄
		VkBuffer buffer{ VK_NULL_HANDLE };        // 统一缓冲区对象句柄
		// The descriptor set stores the resources bound to the binding points in a shader
		// It connects the binding points of the different shaders with the buffers and images used for those bindings
		// 描述符集存储绑定到着色器中绑定点的资源
		// 它将不同着色器的绑定点与用于这些绑定的缓冲区和图像连接起来
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };  // 描述符集句柄
		// We keep a pointer to the mapped buffer, so we can easily update it's contents via a memcpy
		// 我们保留映射缓冲区的指针，以便可以通过 memcpy 轻松更新其内容
		uint8_t* mapped{ nullptr };  // 映射内存的指针（用于更新统一缓冲区数据）
	};
	// We use one UBO per frame, so we can have a frame overlap and make sure that uniforms aren't updated while still in use
	// 我们每帧使用一个统一缓冲区，这样可以实现帧重叠，并确保统一缓冲区在使用时不会被更新
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;  // 每帧的统一缓冲区数组

	// For simplicity we use the same uniform block layout as in the shader:
	//
	//	layout(set = 0, binding = 0) uniform UBO
	//	{
	//		mat4 projectionMatrix;
	//		mat4 modelMatrix;
	//		mat4 viewMatrix;
	//	} ubo;
	//
	// This way we can just memcopy the ubo data to the ubo
	// Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
	// 为简单起见，我们使用与着色器中相同的统一块布局：
	// 这样我们可以直接将统一缓冲区数据 memcpy 到统一缓冲区
	// 注意：应使用与 GPU 对齐的数据类型，以避免手动填充（vec4、mat4）
	struct ShaderData {
		glm::mat4 projectionMatrix;  // 投影矩阵（将 3D 坐标投影到 2D 屏幕空间）
		glm::mat4 modelMatrix;        // 模型矩阵（模型空间到世界空间的变换）
		glm::mat4 viewMatrix;         // 视图矩阵（世界空间到视图空间的变换）
	};

	// The pipeline layout is used by a pipeline to access the descriptor sets
	// It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
	// A pipeline layout can be shared among multiple pipelines as long as their interfaces match
	// 管道布局用于管道访问描述符集
	// 它定义管道使用的着色器阶段与着色器资源之间的接口（不绑定任何实际数据）
	// 只要接口匹配，管道布局可以在多个管道之间共享
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };  // 管道布局句柄

	// Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
	// While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
	// So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
	// Even though this adds a new dimension of planning ahead, it's a great opportunity for performance optimizations by the driver
	// 管道（通常称为"管道状态对象"）用于预编译影响管道的所有状态
	// 在 OpenGL 中，几乎可以随时更改任何状态，而 Vulkan 要求预先布局图形（和计算）管道状态
	// 因此，对于非动态管道状态的每种组合，都需要一个新的管道（这里不讨论一些例外情况）
	// 尽管这增加了提前规划的新维度，但这是驱动程序进行性能优化的绝佳机会
	VkPipeline pipeline{ VK_NULL_HANDLE };  // 图形管道句柄

	// The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
	// Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
	// 描述符集布局描述着色器绑定布局（不实际引用描述符）
	// 与管道布局类似，它基本上是一个蓝图，只要布局匹配，就可以与不同的描述符集一起使用
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };  // 描述符集布局句柄

	// Synchronization primitives
	// Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.
	// 同步原语
	// 同步是 Vulkan 的一个重要概念，OpenGL 大多隐藏了这一点。正确使用同步对于使用 Vulkan 至关重要

	// Semaphores are used to coordinate operations within the graphics queue and ensure correct command ordering
	// 信号量用于协调图形队列内的操作并确保正确的命令顺序
	std::vector<VkSemaphore> presentCompleteSemaphores{};  // 呈现完成信号量（图像可用于呈现时发出信号）
	std::vector<VkSemaphore> renderCompleteSemaphores{};   // 渲染完成信号量（渲染完成时发出信号）

	VkCommandPool commandPool{ VK_NULL_HANDLE };                                    // 命令池句柄（用于分配命令缓冲区）
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};            // 命令缓冲区数组（每帧一个）
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};                        // 等待栅栏数组（用于等待命令缓冲区完成）

	// To select the correct sync and command objects, we need to keep track of the current frame
	// 为了选择正确的同步和命令对象，我们需要跟踪当前帧
	uint32_t currentFrame{ 0 };  // 当前帧索引（用于帧重叠）

	VulkanExample() : VulkanExampleBase()
	{
		title = "Basic indexed triangle";  // 设置示例标题
		// To keep things simple, we don't use the UI overlay from the framework
		// 为保持简单，我们不使用框架的 UI 叠加层
		settings.overlay = false;  // 禁用 UI 叠加层
		// Setup a default look-at camera
		// 设置默认的观察相机
		camera.type = Camera::CameraType::lookat;  // 相机类型：观察相机
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));  // 设置相机位置（在 Z 轴负方向 2.5 单位处）
		camera.setRotation(glm::vec3(0.0f));  // 设置相机旋转（无旋转）
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);  // 设置透视投影（60 度视野，当前宽高比，近平面 1.0，远平面 256.0）
		// Values not set here are initialized in the base class constructor
		// 未在此处设置的值在基类构造函数中初始化
	}

	~VulkanExample() override
	{
		// Clean up used Vulkan resources
		// Note: Inherited destructor cleans up resources stored in base class
		// 清理使用的 Vulkan 资源
		// 注意：继承的析构函数会清理基类中存储的资源
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);  // 销毁图形管道
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);  // 销毁管道布局
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);  // 销毁描述符集布局
			vkDestroyBuffer(device, vertices.buffer, nullptr);  // 销毁顶点缓冲区
			vkFreeMemory(device, vertices.memory, nullptr);  // 释放顶点缓冲区内存
			vkDestroyBuffer(device, indices.buffer, nullptr);  // 销毁索引缓冲区
			vkFreeMemory(device, indices.memory, nullptr);  // 释放索引缓冲区内存
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
				vkDestroyFence(device, waitFences[i], nullptr);  // 销毁等待栅栏
				vkDestroyBuffer(device, uniformBuffers[i].buffer, nullptr);  // 销毁统一缓冲区
				vkFreeMemory(device, uniformBuffers[i].memory, nullptr);  // 释放统一缓冲区内存
			}
		}
	}

	// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
	// Upon success it will return the index of the memory type that fits our requested memory properties
	// This is necessary as implementations can offer an arbitrary number of memory types with different
	// memory properties.
	// You can check https://vulkan.gpuinfo.org/ for details on different memory configurations
	/**
	 * @brief 获取支持指定内存属性的内存类型索引
	 * 此函数用于请求支持我们请求的所有属性标志（例如设备本地、主机可见）的设备内存类型
	 * 成功时，它将返回符合我们请求的内存属性的内存类型索引
	 * 这是必要的，因为实现可以提供任意数量的具有不同内存属性的内存类型
	 * 可以在 https://vulkan.gpuinfo.org/ 上查看不同内存配置的详细信息
	 * @param typeBits 内存类型位掩码（从缓冲区/图像内存需求中获取）
	 * @param properties 请求的内存属性标志（例如 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT）
	 * @return 匹配的内存类型索引
	 */
	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		// Iterate over all memory types available for the device used in this example
		// 遍历本示例使用的设备的所有可用内存类型
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)  // 检查当前内存类型是否在类型位掩码中
			{
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)  // 检查内存类型是否支持所有请求的属性
				{
					return i;  // 返回匹配的内存类型索引
				}
			}
			typeBits >>= 1;  // 右移类型位掩码，检查下一个内存类型
		}

		throw "Could not find a suitable memory type!";  // 如果找不到合适的内存类型，抛出异常
	}

	// Create the per-frame (in flight) Vulkan synchronization primitives used in this example
	/**
	 * @brief 创建每帧（进行中）的 Vulkan 同步原语
	 * 创建本示例中使用的每帧同步原语，包括栅栏和信号量
	 */
	void createSynchronizationPrimitives()
	{
		// Fences are used to check draw command buffer completion on the host
		// 栅栏用于在主机端检查绘制命令缓冲区的完成情况
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {		
			VkFenceCreateInfo fenceCI{};  // 栅栏创建信息
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;  // 结构体类型
			// Create the fences in signaled state (so we don't wait on first render of each command buffer)
			// 以已发出信号的状态创建栅栏（这样我们就不需要在每个命令缓冲区的第一次渲染时等待）
			fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // 创建标志：已发出信号
			// Fence used to ensure that command buffer has completed exection before using it again
			// 栅栏用于确保命令缓冲区在再次使用之前已完成执行
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));  // 创建栅栏
		}
		// Semaphores are used for correct command ordering within a queue
		// Used to ensure that image presentation is complete before starting to submit again
		// 信号量用于队列内正确的命令排序
		// 用于确保在再次开始提交之前图像呈现已完成
		presentCompleteSemaphores.resize(MAX_CONCURRENT_FRAMES);  // 调整呈现完成信号量数组大小
		for (auto& semaphore : presentCompleteSemaphores) {
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };  // 信号量创建信息
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));  // 创建呈现完成信号量
		}
		// Render completion
		// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
		// 渲染完成
		// 信号量用于确保在将图像提交到队列之前，所有已提交的命令都已完成
		renderCompleteSemaphores.resize(swapChain.images.size());  // 调整渲染完成信号量数组大小（每个交换链图像一个）
		for (auto& semaphore : renderCompleteSemaphores) {
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };  // 信号量创建信息
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));  // 创建渲染完成信号量
		}
	}

	/**
	 * @brief 创建命令缓冲区和命令池
	 * 创建命令池并从中分配命令缓冲区，用于记录渲染命令
	 */
	void createCommandBuffers()
	{
		// All command buffers are allocated from a command pool
		// 所有命令缓冲区都从命令池分配
		VkCommandPoolCreateInfo commandPoolCI{};  // 命令池创建信息
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;  // 结构体类型
		commandPoolCI.queueFamilyIndex = swapChain.queueNodeIndex;  // 队列族索引（与交换链使用的队列族相同）
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // 创建标志：允许重置命令缓冲区
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));  // 创建命令池

		// Allocate one command buffer per max. concurrent frame from above pool
		// 从上面的池中为每个最大并发帧分配一个命令缓冲区
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);  // 命令缓冲区分配信息（主级别，MAX_CONCURRENT_FRAMES 个）
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));  // 分配命令缓冲区
	}

	// Prepare vertex and index buffers for an indexed triangle
	// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
	/**
	 * @brief 为索引三角形准备顶点和索引缓冲区
	 * 使用暂存缓冲区将它们上传到设备本地内存，并初始化顶点输入和属性绑定以匹配顶点着色器
	 * 注意：在真实应用程序中，应该一次性分配大块内存，而不是进行小的单独内存分配
	 */
	void createVertexBuffer()
	{
		// A note on memory management in Vulkan in general:
		//	This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
		//	what should be done a real-world application, where you should allocate large chunks of memory at once instead.
		// 关于 Vulkan 中内存管理的一般说明：
		//	这是一个非常复杂的主题，虽然对于示例应用程序来说，小的单独内存分配是可以的，但这不应该是真实应用程序的做法，
		//	在真实应用程序中，应该一次性分配大块内存

		// Setup vertices
		// 设置顶点数据（三个顶点形成一个三角形，每个顶点包含位置和颜色）
		std::vector<Vertex> vertexBuffer{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },  // 顶点 0：右上角，红色
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // 顶点 1：左上角，绿色
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }   // 顶点 2：底部中心，蓝色
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);  // 计算顶点缓冲区大小

		// Setup indices
		// 设置索引数据（定义三角形的顶点顺序）
		std::vector<uint32_t> indexBuffer{ 0, 1, 2 };  // 索引：使用顶点 0、1、2 形成三角形
		indices.count = static_cast<uint32_t>(indexBuffer.size());  // 保存索引数量
		uint32_t indexBufferSize = indices.count * sizeof(uint32_t);  // 计算索引缓冲区大小

		VkMemoryAllocateInfo memAlloc{};  // 内存分配信息
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;  // 结构体类型
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
		// 静态数据（如顶点和索引缓冲区）应存储在设备内存中，以便 GPU 以最优（最快）方式访问
		//
		// 为此，我们使用所谓的"暂存缓冲区"：
		// - 创建一个对主机可见（可以映射）的缓冲区
		// - 将数据复制到此缓冲区
		// - 创建另一个设备本地（VRAM）的相同大小的缓冲区
		// - 使用命令缓冲区将数据从主机复制到设备
		// - 删除主机可见（暂存）缓冲区
		// - 使用设备本地缓冲区进行渲染
		//
		// 注意：在统一内存架构中（主机（CPU）和 GPU 共享相同内存），暂存是不必要的
		// 为保持此示例易于理解，此处没有检查这种情况

		struct StagingBuffer {  // 暂存缓冲区结构体
			VkDeviceMemory memory;  // 暂存缓冲区内存句柄
			VkBuffer buffer;        // 暂存缓冲区对象句柄
		};

		struct {
			StagingBuffer vertices;  // 顶点暂存缓冲区
			StagingBuffer indices;   // 索引暂存缓冲区
		} stagingBuffers{};  // 暂存缓冲区集合

		void* data;  // 映射内存的指针

		// Vertex buffer
		// 顶点缓冲区（暂存缓冲区）
		VkBufferCreateInfo vertexBufferInfoCI{};  // 顶点缓冲区创建信息
		vertexBufferInfoCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  // 结构体类型
		vertexBufferInfoCI.size = vertexBufferSize;  // 缓冲区大小
		// Buffer is used as the copy source
		// 缓冲区用作复制源
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  // 用途：传输源
		// Create a host-visible buffer to copy the vertex data to (staging buffer)
		// 创建主机可见缓冲区以复制顶点数据（暂存缓冲区）
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &stagingBuffers.vertices.buffer));  // 创建顶点暂存缓冲区
		vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);  // 查询缓冲区内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		// Request a host visible memory type that can be used to copy our data to
		// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
		// 请求可用于复制数据的主机可见内存类型
		// 同时请求一致性，以便在取消映射缓冲区后，写入对 GPU 立即可见
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);  // 获取主机可见和一致性内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));  // 分配暂存内存
		// Map and copy
		// 映射并复制顶点数据
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));  // 映射内存
		memcpy(data, vertexBuffer.data(), vertexBufferSize);  // 复制顶点数据到映射内存
		vkUnmapMemory(device, stagingBuffers.vertices.memory);  // 取消映射内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));  // 将内存绑定到缓冲区

		// Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
		// 创建设备本地缓冲区，将（主机本地）顶点数据复制到其中，并用于渲染
		vertexBufferInfoCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // 用途：顶点缓冲区和传输目标
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfoCI, nullptr, &vertices.buffer));  // 创建设备本地顶点缓冲区
		vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);  // 查询缓冲区内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));  // 分配设备本地内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));  // 将内存绑定到缓冲区

		// Index buffer
		// 索引缓冲区（暂存缓冲区）
		VkBufferCreateInfo indexbufferCI{};  // 索引缓冲区创建信息
		indexbufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  // 结构体类型
		indexbufferCI.size = indexBufferSize;  // 缓冲区大小
		indexbufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;  // 用途：传输源
		// Copy index data to a buffer visible to the host (staging buffer)
		// 将索引数据复制到主机可见缓冲区（暂存缓冲区）
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &stagingBuffers.indices.buffer));  // 创建索引暂存缓冲区
		vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);  // 查询缓冲区内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);  // 获取主机可见和一致性内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));  // 分配暂存内存
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));  // 映射内存
		memcpy(data, indexBuffer.data(), indexBufferSize);  // 复制索引数据到映射内存
		vkUnmapMemory(device, stagingBuffers.indices.memory);  // 取消映射内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));  // 将内存绑定到缓冲区

		// Create destination buffer with device only visibility
		// 创建仅设备可见的目标缓冲区
		indexbufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // 用途：索引缓冲区和传输目标
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indices.buffer));  // 创建设备本地索引缓冲区
		vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);  // 查询缓冲区内存需求
		memAlloc.allocationSize = memReqs.size;  // 设置分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));  // 分配设备本地内存
		VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));  // 将内存绑定到缓冲区

		// Buffer copies have to be submitted to a queue, so we need a command buffer for them
		// Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
		// 缓冲区复制必须提交到队列，因此我们需要一个命令缓冲区
		// 注意：某些设备提供专用传输队列（仅设置传输位），在进行大量复制时可能更快
		VkCommandBuffer copyCmd;  // 复制命令缓冲区

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{};  // 命令缓冲区分配信息
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;  // 结构体类型
		cmdBufAllocateInfo.commandPool = commandPool;  // 命令池
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // 命令缓冲区级别（主级别）
		cmdBufAllocateInfo.commandBufferCount = 1;  // 命令缓冲区数量
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));  // 分配命令缓冲区

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();  // 命令缓冲区开始信息
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));  // 开始记录命令缓冲区
		// Put buffer region copies into command buffer
		// 将缓冲区区域复制命令放入命令缓冲区
		VkBufferCopy copyRegion{};  // 缓冲区复制区域
		// Vertex buffer
		// 顶点缓冲区复制
		copyRegion.size = vertexBufferSize;  // 复制大小（顶点缓冲区大小）
		vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);  // 记录复制命令：从暂存缓冲区复制到设备本地缓冲区
		// Index buffer
		// 索引缓冲区复制
		copyRegion.size = indexBufferSize;  // 复制大小（索引缓冲区大小）
		vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer,	1, &copyRegion);  // 记录复制命令：从暂存缓冲区复制到设备本地缓冲区
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));  // 结束记录命令缓冲区

		// Submit the command buffer to the queue to finish the copy
		// 将命令缓冲区提交到队列以完成复制
		VkSubmitInfo submitInfo{};  // 提交信息
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;  // 结构体类型
		submitInfo.commandBufferCount = 1;  // 命令缓冲区数量
		submitInfo.pCommandBuffers = &copyCmd;  // 命令缓冲区指针

		// Create fence to ensure that the command buffer has finished executing
		// 创建栅栏以确保命令缓冲区已完成执行
		VkFenceCreateInfo fenceCI{};  // 栅栏创建信息
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;  // 结构体类型
		fenceCI.flags = 0;  // 创建标志（无特殊标志）
		VkFence fence;  // 栅栏句柄
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));  // 创建栅栏

		// Submit to the queue
		// 提交到队列
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));  // 提交命令缓冲区到队列
		// Wait for the fence to signal that command buffer has finished executing
		// 等待栅栏发出信号，表示命令缓冲区已完成执行
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));  // 等待栅栏

		vkDestroyFence(device, fence, nullptr);  // 销毁栅栏
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);  // 释放命令缓冲区

		// Destroy staging buffers
		// Note: Staging buffer must not be deleted before the copies have been submitted and executed
		// 销毁暂存缓冲区
		// 注意：在复制已提交并执行之前，不得删除暂存缓冲区
		vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);  // 销毁顶点暂存缓冲区
		vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);  // 释放顶点暂存内存
		vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);  // 销毁索引暂存缓冲区
		vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);  // 释放索引暂存内存
	}

	// Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
	/**
	 * @brief 创建描述符池
	 * 描述符从池中分配，告诉实现我们将使用多少和什么类型的描述符（最大值）
	 */
	void createDescriptorPool()
	{
		// We need to tell the API the number of max. requested descriptors per type
		// 我们需要告诉 API 每种类型请求的最大描述符数量
		VkDescriptorPoolSize descriptorTypeCounts[1]{};  // 描述符类型计数数组
		// This example only one descriptor type (uniform buffer)
		// 本示例只有一种描述符类型（统一缓冲区）
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 描述符类型：统一缓冲区
		// We have one buffer (and as such descriptor) per frame
		// 我们每帧有一个缓冲区（因此每帧有一个描述符）
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;  // 描述符数量（每帧一个）
		// For additional types you need to add new entries in the type count list
		// E.g. for two combined image samplers :
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;
		// 对于其他类型，需要在类型计数列表中添加新条目
		// 例如，对于两个组合图像采样器：
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;

		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		// 创建全局描述符池
		// 本示例中使用的所有描述符都从此池中分配
		VkDescriptorPoolCreateInfo descriptorPoolCI{};  // 描述符池创建信息
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;  // 结构体类型
		descriptorPoolCI.pNext = nullptr;  // 扩展指针
		descriptorPoolCI.poolSizeCount = 1;  // 池大小计数（一种描述符类型）
		descriptorPoolCI.pPoolSizes = descriptorTypeCounts;  // 池大小数组
		// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
		// Our sample will create one set per uniform buffer per frame
		// 设置可以从此池请求的最大描述符集数量（超出此限制的请求将导致错误）
		// 我们的示例将为每帧的每个统一缓冲区创建一个集
		descriptorPoolCI.maxSets = MAX_CONCURRENT_FRAMES;  // 最大描述符集数量
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));  // 创建描述符池
	}

	// Descriptor set layouts define the interface between our application and the shader
	// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout binding
	/**
	 * @brief 创建描述符集布局
	 * 描述符集布局定义应用程序和着色器之间的接口
	 * 基本上将不同的着色器阶段连接到描述符，用于绑定统一缓冲区、图像采样器等
	 * 因此，每个着色器绑定应映射到一个描述符集布局绑定
	 */
	void createDescriptorSetLayout()
	{
		// Binding 0: Uniform buffer (Vertex shader)
		// 绑定 0：统一缓冲区（顶点着色器）
		VkDescriptorSetLayoutBinding layoutBinding{};  // 描述符集布局绑定
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 描述符类型：统一缓冲区
		layoutBinding.descriptorCount = 1;  // 描述符数量（1 个）
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // 着色器阶段标志（顶点着色器）
		layoutBinding.pImmutableSamplers = nullptr;  // 不可变采样器（不使用）

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};  // 描述符集布局创建信息
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;  // 结构体类型
		descriptorLayoutCI.pNext = nullptr;  // 扩展指针
		descriptorLayoutCI.bindingCount = 1;  // 绑定数量（1 个绑定）
		descriptorLayoutCI.pBindings = &layoutBinding;  // 绑定数组
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));  // 创建描述符集布局
	}

	// Shaders access data using descriptor sets that "point" at our uniform buffers
	// The descriptor sets make use of the descriptor set layouts created above 
	/**
	 * @brief 创建描述符集
	 * 着色器使用描述符集访问数据，描述符集"指向"我们的统一缓冲区
	 * 描述符集使用上面创建的描述符集布局
	 */
	void createDescriptorSets()
	{
		// Allocate one descriptor set per frame from the global descriptor pool
		// 从全局描述符池中为每帧分配一个描述符集
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VkDescriptorSetAllocateInfo allocInfo{};  // 描述符集分配信息
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;  // 结构体类型
			allocInfo.descriptorPool = descriptorPool;  // 描述符池
			allocInfo.descriptorSetCount = 1;  // 描述符集数量（1 个）
			allocInfo.pSetLayouts = &descriptorSetLayout;  // 描述符集布局数组
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));  // 分配描述符集

			// Update the descriptor set determining the shader binding points
			// For every binding point used in a shader there needs to be one
			// descriptor set matching that binding point
			// 更新描述符集，确定着色器绑定点
			// 对于着色器中使用的每个绑定点，需要有一个匹配该绑定点的描述符集
			VkWriteDescriptorSet writeDescriptorSet{};  // 写入描述符集
			
			// The buffer's information is passed using a descriptor info structure
			// 使用描述符信息结构传递缓冲区的信息
			VkDescriptorBufferInfo bufferInfo{};  // 描述符缓冲区信息
			bufferInfo.buffer = uniformBuffers[i].buffer;  // 统一缓冲区句柄
			bufferInfo.range = sizeof(ShaderData);  // 缓冲区范围（着色器数据大小）

			// Binding 0 : Uniform buffer
			// 绑定 0：统一缓冲区
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;  // 结构体类型
			writeDescriptorSet.dstSet = uniformBuffers[i].descriptorSet;  // 目标描述符集
			writeDescriptorSet.descriptorCount = 1;  // 描述符数量
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // 描述符类型：统一缓冲区
			writeDescriptorSet.pBufferInfo = &bufferInfo;  // 缓冲区信息指针
			writeDescriptorSet.dstBinding = 0;  // 目标绑定点（绑定 0）
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);  // 更新描述符集
		}
	}

	// Create the depth (and stencil) buffer attachments used by our framebuffers
	// Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
	/**
	 * @brief 设置深度（和模板）缓冲区附件
	 * 创建帧缓冲区使用的深度（和模板）缓冲区附件
	 * 注意：这是基类中虚函数的重写，从 VulkanExampleBase::prepare 中调用
	 */
	void setupDepthStencil() override
	{
		// Create an optimal image used as the depth stencil attachment
		// 创建用作深度模板附件的最优图像
		VkImageCreateInfo imageCI{};  // 图像创建信息
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;  // 结构体类型
		imageCI.imageType = VK_IMAGE_TYPE_2D;  // 图像类型（2D）
		imageCI.format = depthFormat;  // 图像格式（深度格式）
		// Use example's height and width
		// 使用示例的高度和宽度
		imageCI.extent = { width, height, 1 };  // 图像尺寸
		imageCI.mipLevels = 1;  // Mip 级别数量（单级别）
		imageCI.arrayLayers = 1;  // 数组层数量（单层）
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;  // 采样数（非多重采样）
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;  // 平铺模式（最优平铺）
		imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;  // 图像用途（深度模板附件）
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // 初始布局（未定义）
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));  // 创建深度模板图像

		// Allocate memory for the image (device local) and bind it to our image
		// 为图像分配内存（设备本地）并将其绑定到我们的图像
		VkMemoryAllocateInfo memAlloc{};  // 内存分配信息
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;  // 结构体类型
		VkMemoryRequirements memReqs;  // 内存需求
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);  // 查询图像内存需求
		memAlloc.allocationSize = memReqs.size;  // 分配大小
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);  // 获取设备本地内存类型
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));  // 分配设备本地内存
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));  // 将内存绑定到图像

		// Create a view for the depth stencil image
		// Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
		// This allows for multiple views of one image with differing ranges (e.g. for different layers)
		// 为深度模板图像创建视图
		// 在 Vulkan 中，图像不能直接访问，而是通过由子资源范围描述的视图访问
		// 这允许一个图像有多个具有不同范围的视图（例如，用于不同的层）
		VkImageViewCreateInfo depthStencilViewCI{};  // 深度模板视图创建信息
		depthStencilViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;  // 结构体类型
		depthStencilViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;  // 视图类型（2D）
		depthStencilViewCI.format = depthFormat;  // 视图格式
		depthStencilViewCI.subresourceRange = {};  // 子资源范围
		depthStencilViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;  // 图像方面（深度）
		// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT)
		// 模板方面应仅在深度 + 模板格式上设置（VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT）
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {  // 如果格式支持模板
			depthStencilViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;  // 添加模板方面
		}
		depthStencilViewCI.subresourceRange.baseMipLevel = 0;  // 基础 Mip 级别
		depthStencilViewCI.subresourceRange.levelCount = 1;  // Mip 级别数量
		depthStencilViewCI.subresourceRange.baseArrayLayer = 0;  // 基础数组层
		depthStencilViewCI.subresourceRange.layerCount = 1;  // 层数量
		depthStencilViewCI.image = depthStencil.image;  // 要创建视图的图像
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilViewCI, nullptr, &depthStencil.view));  // 创建深度模板图像视图
	}

	// Create a frame buffer for each swap chain image
	// Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
	/**
	 * @brief 为每个交换链图像创建帧缓冲区
	 * 注意：这是基类中虚函数的重写，从 VulkanExampleBase::prepare 中调用
	 */
	void setupFrameBuffer() override
	{
		// Create a frame buffer for every image in the swapchain
		// 为交换链中的每个图像创建帧缓冲区
		frameBuffers.resize(swapChain.images.size());  // 调整帧缓冲区数组大小
		for (size_t i = 0; i < frameBuffers.size(); i++)
		{
			std::array<VkImageView, 2> attachments{};  // 附件数组（颜色和深度模板）
			// Color attachment is the view of the swapchain image
			// 颜色附件是交换链图像的视图
			attachments[0] = swapChain.imageViews[i];  // 颜色附件（交换链图像视图）
			// Depth/Stencil attachment is the same for all frame buffers due to how depth works with current GPUs
			// 深度/模板附件对所有帧缓冲区都相同，因为深度在当前 GPU 上的工作方式
			attachments[1] = depthStencil.view;  // 深度模板附件（所有帧缓冲区共享）

			VkFramebufferCreateInfo frameBufferCI{};  // 帧缓冲区创建信息
			frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;  // 结构体类型
			// All frame buffers use the same renderpass setup
			// 所有帧缓冲区使用相同的渲染通道设置
			frameBufferCI.renderPass = renderPass;  // 渲染通道
			frameBufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());  // 附件数量（2 个：颜色和深度模板）
			frameBufferCI.pAttachments = attachments.data();  // 附件数组
			frameBufferCI.width = width;  // 帧缓冲区宽度
			frameBufferCI.height = height;  // 帧缓冲区高度
			frameBufferCI.layers = 1;  // 层数量（单层）
			// Create the framebuffer
			// 创建帧缓冲区
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &frameBuffers[i]));  // 创建帧缓冲区
		}
	}

	// Render pass setup
	// Render passes are a new concept in Vulkan. They describe the attachments used during rendering and may contain multiple subpasses with attachment dependencies
	// This allows the driver to know up-front what the rendering will look like and is a good opportunity to optimize especially on tile-based renderers (with multiple subpasses)
	// Using sub pass dependencies also adds implicit layout transitions for the attachment used, so we don't need to add explicit image memory barriers to transform them
	// Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
	/**
	 * @brief 设置渲染通道
	 * 渲染通道是 Vulkan 中的一个新概念。它们描述渲染期间使用的附件，可能包含多个具有附件依赖关系的子通道
	 * 这允许驱动程序提前了解渲染的外观，并且是优化的好机会，特别是在基于瓦片的渲染器上（具有多个子通道）
	 * 使用子通道依赖关系还会为使用的附件添加隐式布局转换，因此我们不需要添加显式图像内存屏障来转换它们
	 * 注意：这是基类中虚函数的重写，从 VulkanExampleBase::prepare 中调用
	 */
	void setupRenderPass() override
	{
		// This example will use a single render pass with one subpass
		// 本示例将使用具有一个子通道的单个渲染通道

		// Descriptors for the attachments used by this renderpass
		// 此渲染通道使用的附件描述符
		std::array<VkAttachmentDescription, 2> attachments{};  // 附件描述数组（颜色和深度模板）

		// Color attachment
		// 颜色附件
		attachments[0].format = swapChain.colorFormat;                                  // Use the color format selected by the swapchain
		                                                                                // 使用交换链选择的颜色格式
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;                                 // We don't use multi sampling in this example
		                                                                                // 本示例不使用多重采样
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                            // Clear this attachment at the start of the render pass
		                                                                                // 在渲染通道开始时清除此附件
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                          // Keep its contents after the render pass is finished (for displaying it)
		                                                                                // 在渲染通道完成后保留其内容（用于显示）
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                 // We don't use stencil, so don't care for load
		                                                                                // 我们不使用模板，因此不关心加载
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;               // Same for store
		                                                                                // 存储同样不关心
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                       // Layout at render pass start. Initial doesn't matter, so we use undefined
		                                                                                // 渲染通道开始时的布局。初始布局不重要，因此我们使用未定义
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                   // Layout to which the attachment is transitioned when the render pass is finished
		                                                                                // 渲染通道完成时附件转换到的布局
		                                                                                // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR
		                                                                                // 由于我们希望将颜色缓冲区呈现到交换链，我们转换到 PRESENT_KHR
		// Depth attachment
		// 深度附件
		attachments[1].format = depthFormat;                                           // A proper depth format is selected in the example base
		                                                                               // 在示例基类中选择适当的深度格式
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;  // 采样数（非多重采样）
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                           // Clear depth at start of first subpass
		                                                                               // 在第一个子通道开始时清除深度
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                     // We don't need depth after render pass has finished (DONT_CARE may result in better performance)
		                                                                               // 渲染通道完成后我们不需要深度（DONT_CARE 可能会带来更好的性能）
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                // No stencil
		                                                                              // 无模板
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;              // No Stencil
		                                                                              // 无模板
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                      // Layout at render pass start. Initial doesn't matter, so we use undefined
		                                                                             // 渲染通道开始时的布局。初始布局不重要，因此我们使用未定义
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Transition to depth/stencil attachment
		                                                                              // 转换到深度/模板附件布局

		// Setup attachment references
		// 设置附件引用
		VkAttachmentReference colorReference{};  // 颜色附件引用
		colorReference.attachment = 0;                                    // Attachment 0 is color
		                                                                  // 附件 0 是颜色
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass
		                                                                  // 子通道期间用作颜色的附件布局

		VkAttachmentReference depthReference{};  // 深度附件引用
		depthReference.attachment = 1;                                            // Attachment 1 is depth
		                                                                          // 附件 1 是深度
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Attachment used as depth/stencil used during the subpass
		                                                                          // 子通道期间用作深度/模板的附件

		// Setup a single subpass reference
		// 设置单个子通道引用
		VkSubpassDescription subpassDescription{};  // 子通道描述
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;  // 管道绑定点（图形）
		subpassDescription.colorAttachmentCount = 1;                            // Subpass uses one color attachment
		                                                                        // 子通道使用一个颜色附件
		subpassDescription.pColorAttachments = &colorReference;                 // Reference to the color attachment in slot 0
		                                                                        // 插槽 0 中颜色附件的引用
		subpassDescription.pDepthStencilAttachment = &depthReference;           // Reference to the depth attachment in slot 1
		                                                                        // 插槽 1 中深度附件的引用
		subpassDescription.inputAttachmentCount = 0;                            // Input attachments can be used to sample from contents of a previous subpass
		                                                                        // 输入附件可用于从前一个子通道的内容中采样
		subpassDescription.pInputAttachments = nullptr;                         // (Input attachments not used by this example)
		                                                                        // （本示例不使用输入附件）
		subpassDescription.preserveAttachmentCount = 0;                         // Preserved attachments can be used to loop (and preserve) attachments through subpasses
		                                                                        // 保留的附件可用于在子通道之间循环（和保留）附件
		subpassDescription.pPreserveAttachments = nullptr;                      // (Preserve attachments not used by this example)
		                                                                        // （本示例不使用保留附件）
		subpassDescription.pResolveAttachments = nullptr;                       // Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling
		                                                                        // 解析附件在子通道结束时解析，可用于例如多重采样

		// Setup subpass dependencies
		// These will add the implicit attachment layout transitions specified by the attachment descriptions
		// The actual usage layout is preserved through the layout specified in the attachment reference
		// Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
		// srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
		// Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
		// 设置子通道依赖关系
		// 这些将添加附件描述指定的隐式附件布局转换
		// 实际使用布局通过附件引用中指定的布局保留
		// 每个子通道依赖关系将在源和目标子通道之间引入内存和执行依赖关系，由以下描述：
		// srcStageMask、dstStageMask、srcAccessMask、dstAccessMask（并设置 dependencyFlags）
		// 注意：VK_SUBPASS_EXTERNAL 是一个特殊常量，指在实际渲染通道之外执行的所有命令
		std::array<VkSubpassDependency, 2> dependencies{};  // 子通道依赖关系数组

		// Does the transition from final to initial layout for the depth an color attachments
		// Depth attachment
		// 为深度和颜色附件执行从最终到初始布局的转换
		// 深度附件
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;  // 源子通道（外部）
		dependencies[0].dstSubpass = 0;  // 目标子通道（子通道 0）
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;  // 源阶段掩码（早期和晚期片段测试）
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;  // 目标阶段掩码（早期和晚期片段测试）
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;  // 源访问掩码（深度模板附件写入）
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;  // 目标访问掩码（深度模板附件写入和读取）
		dependencies[0].dependencyFlags = 0;  // 依赖关系标志（无特殊标志）
		// Color attachment
		// 颜色附件
		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;  // 源子通道（外部）
		dependencies[1].dstSubpass = 0;  // 目标子通道（子通道 0）
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 源阶段掩码（颜色附件输出）
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 目标阶段掩码（颜色附件输出）
		dependencies[1].srcAccessMask = 0;  // 源访问掩码（无）
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;  // 目标访问掩码（颜色附件写入和读取）
		dependencies[1].dependencyFlags = 0;  // 依赖关系标志（无特殊标志）

		// Create the actual renderpass
		// 创建实际的渲染通道
		VkRenderPassCreateInfo renderPassCI{};  // 渲染通道创建信息
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;  // 结构体类型
		renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());  // Number of attachments used by this render pass
		                                                                           // 此渲染通道使用的附件数量
		renderPassCI.pAttachments = attachments.data();                            // Descriptions of the attachments used by the render pass
		                                                                          // 渲染通道使用的附件描述
		renderPassCI.subpassCount = 1;                                             // We only use one subpass in this example
		                                                                          // 本示例中我们只使用一个子通道
		renderPassCI.pSubpasses = &subpassDescription;                             // Description of that subpass
		                                                                          // 该子通道的描述
		renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size()); // Number of subpass dependencies
		                                                                          // 子通道依赖关系数量
		renderPassCI.pDependencies = dependencies.data();                          // Subpass dependencies used by the render pass
		                                                                         // 渲染通道使用的子通道依赖关系
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));  // 创建渲染通道
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

		if (is.is_open())
		{
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
		if (shaderCode)
		{
			// Create a new shader module that will be used for pipeline creation
			// 创建将用于管道创建的新着色器模块
			VkShaderModuleCreateInfo shaderModuleCI{};  // 着色器模块创建信息
			shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;  // 结构体类型
			shaderModuleCI.codeSize = shaderSize;  // 着色器代码大小（字节数）
			shaderModuleCI.pCode = (uint32_t*)shaderCode;  // 着色器代码指针（SPIR-V 代码，按 32 位字对齐）

			VkShaderModule shaderModule;  // 着色器模块句柄
			VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));  // 创建着色器模块

			delete[] shaderCode;  // 释放着色器代码缓冲区

			return shaderModule;  // 返回着色器模块句柄
		}
		else
		{
			std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;  // 输出错误信息
			return VK_NULL_HANDLE;  // 返回空句柄
		}
	}

	/**
	 * @brief 创建图形管道
	 * 创建用于渲染的图形管道，包括所有管道状态（输入装配、光栅化、混合、视口、深度模板等）
	 */
	void createPipelines()
	{
		// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
		// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
		// 创建用于生成基于此描述符集布局的渲染管道的管道布局
		// 在更复杂的场景中，您可以为不同的描述符集布局使用不同的管道布局，这些布局可以重用
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};  // 管道布局创建信息
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;  // 结构体类型
		pipelineLayoutCI.pNext = nullptr;  // 扩展指针
		pipelineLayoutCI.setLayoutCount = 1;  // 描述符集布局数量（1 个）
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;  // 描述符集布局数组
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));  // 创建管道布局

		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)
		// 创建本示例中使用的图形管道
		// Vulkan 使用渲染管道的概念来封装固定状态，取代 OpenGL 的复杂状态机
		// 管道随后在 GPU 上存储和哈希，使管道更改非常快
		// 注意：仍然有一些动态状态不是管道的直接部分（但它们被使用的信息是）

		VkGraphicsPipelineCreateInfo pipelineCI{};  // 图形管道创建信息
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;  // 结构体类型
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		// 此管道使用的布局（可以在使用相同布局的多个管道之间共享）
		pipelineCI.layout = pipelineLayout;  // 管道布局
		// Renderpass this pipeline is attached to
		// 此管道附加到的渲染通道
		pipelineCI.renderPass = renderPass;  // 渲染通道

		// Construct the different states making up the pipeline
		// 构建组成管道的不同状态

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		// 输入装配状态描述如何装配图元
		// 此管道将顶点数据装配为三角形列表（尽管我们只使用一个三角形）
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};  // 输入装配状态创建信息
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;  // 结构体类型
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  // 图元拓扑（三角形列表）

		// Rasterization state
		// 光栅化状态
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};  // 光栅化状态创建信息
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;  // 结构体类型
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
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};  // 颜色混合状态创建信息
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;  // 结构体类型
		colorBlendStateCI.attachmentCount = 1;  // 附件数量（1 个颜色附件）
		colorBlendStateCI.pAttachments = &blendAttachmentState;  // 混合附件状态数组

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		// 视口状态设置此管道中使用的视口和裁剪矩形数量
		// 注意：这实际上被动态状态覆盖（见下文）
		VkPipelineViewportStateCreateInfo viewportStateCI{};  // 视口状态创建信息
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;  // 结构体类型
		viewportStateCI.viewportCount = 1;  // 视口数量（1 个）
		viewportStateCI.scissorCount = 1;  // 裁剪矩形数量（1 个）

		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
		// 启用动态状态
		// 大多数状态都烘焙到管道中，但仍有几个动态状态可以在命令缓冲区中更改
		// 为了能够更改这些，我们需要指定将使用此管道更改哪些动态状态。它们的实际状态稍后在命令缓冲区中设置
		// 对于此示例，我们将使用动态状态设置视口和裁剪矩形
		std::vector<VkDynamicState> dynamicStateEnables;  // 动态状态向量
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);  // 添加动态视口状态
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);  // 添加动态裁剪矩形状态
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};  // 动态状态创建信息
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;  // 结构体类型
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();  // 动态状态数组
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());  // 动态状态数量

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		// 深度和模板状态，包含深度和模板比较和测试操作
		// 我们只使用深度测试，并希望启用深度测试和写入，并使用小于或等于进行比较
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};  // 深度模板状态创建信息
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;  // 结构体类型
		depthStencilStateCI.depthTestEnable = VK_TRUE;  // 深度测试启用
		depthStencilStateCI.depthWriteEnable = VK_TRUE;  // 深度写入启用
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;  // 深度比较操作（小于或等于）
		depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;  // 深度边界测试（禁用）
		depthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;  // 背面模板失败操作（保持）
		depthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;  // 背面模板通过操作（保持）
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;  // 背面模板比较操作（始终通过）
		depthStencilStateCI.stencilTestEnable = VK_FALSE;  // 模板测试（禁用）
		depthStencilStateCI.front = depthStencilStateCI.back;  // 正面模板状态（与背面相同）

		// Multi sampling state
		// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		// 多重采样状态
		// 本示例不使用多重采样（用于抗锯齿），但仍必须设置状态并传递给管道
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};  // 多重采样状态创建信息
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;  // 结构体类型
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // 光栅化采样数（单采样）
		multisampleStateCI.pSampleMask = nullptr;  // 采样掩码（不使用）

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
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};  // 顶点输入状态创建信息
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;  // 结构体类型
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
		// Set pipeline stage for this shader
		// 设置此着色器的管道阶段
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;  // 着色器阶段（顶点着色器）
		// Load binary SPIR-V shader
		// 加载二进制 SPIR-V 着色器
		shaderStages[0].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv");  // 加载顶点着色器模块
		// Main entry point for the shader
		// 着色器的主入口点
		shaderStages[0].pName = "main";  // 入口点函数名
		assert(shaderStages[0].module != VK_NULL_HANDLE);  // 确保着色器模块加载成功

		// Fragment shader
		// 片段着色器
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;  // 结构体类型
		// Set pipeline stage for this shader
		// 设置此着色器的管道阶段
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;  // 着色器阶段（片段着色器）
		// Load binary SPIR-V shader
		// 加载二进制 SPIR-V 着色器
		shaderStages[1].module = loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv");  // 加载片段着色器模块
		// Main entry point for the shader
		// 着色器的主入口点
		shaderStages[1].pName = "main";  // 入口点函数名
		assert(shaderStages[1].module != VK_NULL_HANDLE);  // 确保着色器模块加载成功

		// Set pipeline shader stage info
		// 设置管道着色器阶段信息
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());  // 着色器阶段数量（2 个）
		pipelineCI.pStages = shaderStages.data();  // 着色器阶段数组

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

		// Create rendering pipeline using the specified states
		// 使用指定的状态创建渲染管道
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));  // 创建图形管道

		// Shader modules are no longer needed once the graphics pipeline has been created
		// 一旦图形管道创建完成，着色器模块就不再需要
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
		// Single uniforms like in OpenGL are no longer present in Vulkan. All hader uniforms are passed via uniform buffer blocks
		// 准备并初始化每帧的统一缓冲区块，包含着色器统一变量
		// Vulkan 中不再存在像 OpenGL 中的单个统一变量。所有着色器统一变量都通过统一缓冲区块传递
		VkMemoryRequirements memReqs;  // 内存需求

		// Vertex shader uniform buffer block
		// 顶点着色器统一缓冲区块
		VkBufferCreateInfo bufferInfo{};  // 缓冲区创建信息
		VkMemoryAllocateInfo allocInfo{};  // 内存分配信息
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;  // 结构体类型
		allocInfo.pNext = nullptr;  // 扩展指针
		allocInfo.allocationSize = 0;  // 分配大小（稍后设置）
		allocInfo.memoryTypeIndex = 0;  // 内存类型索引（稍后设置）

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  // 结构体类型
		bufferInfo.size = sizeof(ShaderData);  // 缓冲区大小（着色器数据大小）
		// This buffer will be used as a uniform buffer
		// 此缓冲区将用作统一缓冲区
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;  // 用途：统一缓冲区

		// Create the buffers
		// 创建缓冲区（每帧一个）
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].buffer));  // 创建统一缓冲区
			// Get memory requirements including size, alignment and memory type
			// 获取内存需求，包括大小、对齐和内存类型
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].buffer, &memReqs);  // 查询缓冲区内存需求
			allocInfo.allocationSize = memReqs.size;  // 设置分配大小
			// Get the memory type index that supports host visible memory access
			// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
			// We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
			// Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
			// 获取支持主机可见内存访问的内存类型索引
			// 大多数实现提供多种内存类型，选择正确的内存类型进行分配至关重要
			// 我们还希望缓冲区是主机一致的，这样我们就不必在每次更新后刷新（或同步）
			// 注意：这可能会影响性能，因此在定期更新缓冲区的真实应用程序中，您可能不想这样做
			allocInfo.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);  // 获取主机可见和一致性内存类型
			// Allocate memory for the uniform buffer
			// 为统一缓冲区分配内存
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));  // 分配内存
			// Bind memory to buffer
			// 将内存绑定到缓冲区
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].buffer, uniformBuffers[i].memory, 0));  // 绑定内存
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
		createDescriptorSetLayout();  // 创建描述符集布局
		createDescriptorPool();  // 创建描述符池
		createDescriptorSets();  // 创建描述符集
		createPipelines();  // 创建图形管道
		prepared = true;  // 标记为已准备完成
	}

	/**
	 * @brief 渲染一帧
	 * 执行完整的渲染循环：等待栅栏、获取交换链图像、更新统一缓冲区、记录命令、提交到队列、呈现图像
	 * 注意：这是基类中虚函数的重写
	 */
	void render() override
	{
		if (!prepared)  // 如果未准备完成，直接返回
			return;

		// Use a fence to wait until the command buffer has finished execution before using it again
		// 使用栅栏等待命令缓冲区完成执行，然后才能再次使用它
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);  // 等待当前帧的栅栏
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));  // 重置栅栏，准备下一帧使用

		// Get the next swap chain image from the implementation
		// Note that the implementation is free to return the images in any order, so we must use the acquire function and can't just cycle through the images/imageIndex on our own
		// 从实现获取下一个交换链图像
		// 注意：实现可以按任何顺序返回图像，因此我们必须使用获取函数，不能自己简单地循环图像/图像索引
		uint32_t imageIndex;  // 交换链图像索引
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);  // 获取下一个交换链图像
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {  // 如果交换链已过期（例如窗口大小改变）
			windowResize();  // 调整窗口大小（重新创建交换链）
			return;
		}
		else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {  // 如果获取失败且不是次优状态
			throw "Could not acquire the next swap chain image!";  // 抛出异常
		}

		// Update the uniform buffer for the next frame
		// 更新下一帧的统一缓冲区
		ShaderData shaderData{};  // 着色器数据
		shaderData.projectionMatrix = camera.matrices.perspective;  // 投影矩阵（从相机获取）
		shaderData.viewMatrix = camera.matrices.view;  // 视图矩阵（从相机获取）
		shaderData.modelMatrix = glm::mat4(1.0f);  // 模型矩阵（单位矩阵，无变换）

		// Copy the current matrices to the current frame's uniform buffer
		// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
		// 将当前矩阵复制到当前帧的统一缓冲区
		// 注意：由于我们为统一缓冲区请求了主机一致性内存类型，写入对 GPU 立即可见
		memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));  // 复制着色器数据到映射内存

		// Build the command buffer
		// Unlike in OpenGL all rendering commands are recorded into command buffers that are then submitted to the queue
		// This allows to generate work upfront in a separate thread
		// For basic command buffers (like in this sample), recording is so fast that there is no need to offload this
		// 构建命令缓冲区
		// 与 OpenGL 不同，所有渲染命令都记录到命令缓冲区中，然后提交到队列
		// 这允许在单独的线程中提前生成工作
		// 对于基本命令缓冲区（如本示例），记录速度非常快，无需卸载此操作

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);  // 重置命令缓冲区，准备记录新命令

		VkCommandBufferBeginInfo cmdBufInfo{};  // 命令缓冲区开始信息
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;  // 结构体类型

		// Set clear values for all framebuffer attachments with loadOp set to clear
		// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
		// 为所有具有 loadOp 设置为清除的帧缓冲区附件设置清除值
		// 我们使用两个附件（颜色和深度），它们在子通道开始时被清除，因此我们需要为两者设置清除值
		VkClearValue clearValues[2]{};  // 清除值数组
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };  // 颜色清除值（深蓝色，RGBA）
		clearValues[1].depthStencil = { 1.0f, 0 };  // 深度模板清除值（深度 = 1.0，模板 = 0）

		VkRenderPassBeginInfo renderPassBeginInfo{};  // 渲染通道开始信息
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;  // 结构体类型
		renderPassBeginInfo.pNext = nullptr;  // 扩展指针
		renderPassBeginInfo.renderPass = renderPass;  // 渲染通道
		renderPassBeginInfo.renderArea.offset.x = 0;  // 渲染区域偏移 X
		renderPassBeginInfo.renderArea.offset.y = 0;  // 渲染区域偏移 Y
		renderPassBeginInfo.renderArea.extent.width = width;  // 渲染区域宽度
		renderPassBeginInfo.renderArea.extent.height = height;  // 渲染区域高度
		renderPassBeginInfo.clearValueCount = 2;  // 清除值数量（2 个）
		renderPassBeginInfo.pClearValues = clearValues;  // 清除值数组
		renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];  // 帧缓冲区（使用获取的图像索引）

		const VkCommandBuffer commandBuffer = commandBuffers[currentFrame];  // 当前帧的命令缓冲区
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));  // 开始记录命令缓冲区

		// Start the first sub pass specified in our default render pass setup by the base class
		// This will clear the color and depth attachment
		// 开始基类在默认渲染通道设置中指定的第一个子通道
		// 这将清除颜色和深度附件
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);  // 开始渲染通道（内联命令）
		// Update dynamic viewport state
		// 更新动态视口状态
		VkViewport viewport{};  // 视口
		viewport.height = (float)height;  // 视口高度
		viewport.width = (float)width;  // 视口宽度
		viewport.minDepth = (float)0.0f;  // 最小深度值
		viewport.maxDepth = (float)1.0f;  // 最大深度值
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);  // 设置视口（动态状态）
		// Update dynamic scissor state
		// 更新动态裁剪矩形状态
		VkRect2D scissor{};  // 裁剪矩形
		scissor.extent.width = width;  // 裁剪矩形宽度
		scissor.extent.height = height;  // 裁剪矩形高度
		scissor.offset.x = 0;  // 裁剪矩形偏移 X
		scissor.offset.y = 0;  // 裁剪矩形偏移 Y
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);  // 设置裁剪矩形（动态状态）
		// Bind descriptor set for the current frame's uniform buffer, so the shader uses the data from that buffer for this draw
		// 绑定当前帧统一缓冲区的描述符集，以便着色器在此绘制中使用该缓冲区的数据
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);  // 绑定描述符集
		// Bind the rendering pipeline
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		// 绑定渲染管道
		// 管道（状态对象）包含渲染管道的所有状态，绑定它将设置管道创建时指定的所有状态
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);  // 绑定图形管道
		// Bind triangle vertex buffer (contains position and colors)
		// 绑定三角形顶点缓冲区（包含位置和颜色）
		VkDeviceSize offsets[1]{ 0 };  // 顶点缓冲区偏移数组
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);  // 绑定顶点缓冲区（绑定点 0）
		// Bind triangle index buffer
		// 绑定三角形索引缓冲区
		vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);  // 绑定索引缓冲区（32 位无符号整数类型）
		// Draw indexed triangle
		// 绘制索引三角形
		vkCmdDrawIndexed(commandBuffer, indices.count, 1, 0, 0, 0);  // 绘制索引图元（索引数量，实例数量，第一个索引，顶点偏移，第一个实例）
		vkCmdEndRenderPass(commandBuffer);  // 结束渲染通道
		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
		// 结束渲染通道将添加隐式屏障，将帧缓冲区颜色附件转换到
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 以将其呈现到窗口系统
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));  // 结束记录命令缓冲区

		// Submit the command buffer to the graphics queue
		// 将命令缓冲区提交到图形队列

		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		// 队列提交将等待的管道阶段（通过 pWaitSemaphores）
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 等待阶段掩码（颜色附件输出阶段）
		// The submit info structure specifies a command buffer queue submission batch
		// 提交信息结构指定命令缓冲区队列提交批次
		VkSubmitInfo submitInfo{};  // 提交信息
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;  // 结构体类型
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
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));  // 提交命令缓冲区到队列

		// Present the current frame buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted
		// 将当前帧缓冲区呈现到交换链
		// 将提交信息中命令缓冲区提交发出的信号量作为交换链呈现的等待信号量传递
		// 这确保在所有命令都已提交之前，图像不会呈现到窗口系统

		VkPresentInfoKHR presentInfo{};  // 呈现信息
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;  // 结构体类型
		presentInfo.waitSemaphoreCount = 1;  // 等待信号量数量
		presentInfo.pWaitSemaphores = &renderCompleteSemaphores[imageIndex];  // 等待信号量（渲染完成信号量）
		presentInfo.swapchainCount = 1;  // 交换链数量
		presentInfo.pSwapchains = &swapChain.swapChain;  // 交换链数组
		presentInfo.pImageIndices = &imageIndex;  // 图像索引数组
		result = vkQueuePresentKHR(queue, &presentInfo);  // 呈现图像到交换链

		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {  // 如果交换链已过期或次优
			windowResize();  // 调整窗口大小（重新创建交换链）
		}
		else if (result != VK_SUCCESS) {  // 如果呈现失败
			throw "Could not present the image to the swap chain!";  // 抛出异常
		}

		// Select the next frame to render to, based on the max. no. of concurrent frames
		// 根据最大并发帧数选择下一个要渲染的帧
		currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;  // 循环选择下一帧（帧重叠）
	}
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
