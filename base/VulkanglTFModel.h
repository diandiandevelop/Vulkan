/*
* Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
*
* Copyright (C) 2018-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * Note that this isn't a complete glTF loader and not all features of the glTF 2.0 spec are supported
 * For details on how glTF 2.0 works, see the official spec at https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * If you are looking for a complete glTF implementation, check out https://github.com/SaschaWillems/Vulkan-glTF-PBR/
 * 
 * 注意：这不是一个完整的 glTF 加载器，不支持 glTF 2.0 规范的所有功能
 * 有关 glTF 2.0 的详细信息，请参阅官方规范：https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 * 
 * 如果您正在寻找完整的 glTF 实现，请查看：https://github.com/SaschaWillems/Vulkan-glTF-PBR/
 */

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include <ktx.h>
#include <ktxvulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vkglTF
{
	/**
	 * @brief 描述符绑定标志
	 */
	enum DescriptorBindingFlags {
		ImageBaseColor = 0x00000001,  // 基础颜色纹理
		ImageNormalMap = 0x00000002  // 法线贴图纹理
	};

	extern VkDescriptorSetLayout descriptorSetLayoutImage;  // 图像描述符集布局
	extern VkDescriptorSetLayout descriptorSetLayoutUbo;   // Uniform 缓冲区描述符集布局
	extern VkMemoryPropertyFlags memoryPropertyFlags;       // 内存属性标志
	extern uint32_t descriptorBindingFlags;                 // 描述符绑定标志

	struct Node;

	/*
		glTF texture loading class
		glTF 纹理加载类
	*/
	/**
	 * @brief glTF 纹理类
	 */
	struct Texture {
		vks::VulkanDevice* device = nullptr;  // Vulkan 设备指针
		VkImage image;                        // 图像句柄
		VkImageLayout imageLayout;            // 图像布局
		VkDeviceMemory deviceMemory;          // 设备内存句柄
		VkImageView view;                     // 图像视图句柄
		uint32_t width, height;               // 纹理宽度和高度
		uint32_t mipLevels;                   // Mip 级别数量
		uint32_t layerCount;                  // 层数量
		VkDescriptorImageInfo descriptor;     // 描述符图像信息
		VkSampler sampler;                    // 采样器句柄
		uint32_t index;                       // 纹理索引
		/**
		 * @brief 更新描述符信息
		 */
		void updateDescriptor();
		/**
		 * @brief 销毁纹理资源
		 */
		void destroy();
		/**
		 * @brief 从 glTF 图像创建纹理
		 * @param gltfimage glTF 图像数据
		 * @param path 文件路径
		 * @param device Vulkan 设备指针
		 * @param copyQueue 复制队列
		 */
		void fromglTfImage(tinygltf::Image& gltfimage, std::string path, vks::VulkanDevice* device, VkQueue copyQueue);
	};

	/*
		glTF material class
		glTF 材质类
	*/
	/**
	 * @brief glTF 材质类
	 */
	struct Material {
		vks::VulkanDevice* device = nullptr;  // Vulkan 设备指针
		enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };  // Alpha 模式枚举
		AlphaMode alphaMode = ALPHAMODE_OPAQUE;  // Alpha 模式：不透明、遮罩、混合
		float alphaCutoff = 1.0f;              // Alpha 截止值
		float metallicFactor = 1.0f;           // 金属度因子
		float roughnessFactor = 1.0f;          // 粗糙度因子
		glm::vec4 baseColorFactor = glm::vec4(1.0f);  // 基础颜色因子
		vkglTF::Texture* baseColorTexture = nullptr;           // 基础颜色纹理
		vkglTF::Texture* metallicRoughnessTexture = nullptr;  // 金属度粗糙度纹理
		vkglTF::Texture* normalTexture = nullptr;             // 法线纹理
		vkglTF::Texture* occlusionTexture = nullptr;          // 遮挡纹理
		vkglTF::Texture* emissiveTexture = nullptr;           // 自发光纹理

		vkglTF::Texture* specularGlossinessTexture;  // 镜面光泽度纹理（KHR_materials_pbrSpecularGlossiness）
		vkglTF::Texture* diffuseTexture;             // 漫反射纹理（KHR_materials_pbrSpecularGlossiness）

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;  // 描述符集句柄

		/**
		 * @brief 构造函数
		 * @param device Vulkan 设备指针
		 */
		Material(vks::VulkanDevice* device) : device(device) {};
		/**
		 * @brief 创建描述符集
		 * @param descriptorPool 描述符池
		 * @param descriptorSetLayout 描述符集布局
		 * @param descriptorBindingFlags 描述符绑定标志
		 */
		void createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags);
	};

	/*
		glTF primitive
		glTF 图元
	*/
	/**
	 * @brief glTF 图元类
	 */
	struct Primitive {
		uint32_t firstIndex;    // 第一个索引
		uint32_t indexCount;    // 索引数量
		uint32_t firstVertex;   // 第一个顶点
		uint32_t vertexCount;   // 顶点数量
		Material& material;     // 材质引用

		/**
		 * @brief 图元尺寸信息
		 */
		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);      // 最小边界点
			glm::vec3 max = glm::vec3(-FLT_MAX);     // 最大边界点
			glm::vec3 size;                          // 尺寸
			glm::vec3 center;                        // 中心点
			float radius;                           // 包围球半径
		} dimensions;

		/**
		 * @brief 设置图元尺寸
		 * @param min 最小边界点
		 * @param max 最大边界点
		 */
		void setDimensions(glm::vec3 min, glm::vec3 max);
		/**
		 * @brief 构造函数
		 * @param firstIndex 第一个索引
		 * @param indexCount 索引数量
		 * @param material 材质引用
		 */
		Primitive(uint32_t firstIndex, uint32_t indexCount, Material& material) : firstIndex(firstIndex), indexCount(indexCount), material(material) {};
	};

	/*
		glTF mesh
		glTF 网格
	*/
	/**
	 * @brief glTF 网格类
	 */
	struct Mesh {
		vks::VulkanDevice* device;  // Vulkan 设备指针

		std::vector<Primitive*> primitives;  // 图元列表
		std::string name;                    // 网格名称

		/**
		 * @brief Uniform 缓冲区结构
		 */
		struct UniformBuffer {
			VkBuffer buffer;                    // 缓冲区句柄
			VkDeviceMemory memory;              // 内存句柄
			VkDescriptorBufferInfo descriptor;  // 描述符缓冲区信息
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;  // 描述符集句柄
			void* mapped;                       // 映射的内存指针
		} uniformBuffer;

		/**
		 * @brief Uniform 块数据
		 */
		struct UniformBlock {
			glm::mat4 matrix;              // 变换矩阵
			glm::mat4 jointMatrix[64]{};   // 关节矩阵数组（用于蒙皮）
			float jointcount{ 0 };         // 关节数量
		} uniformBlock;

		/**
		 * @brief 构造函数
		 * @param device Vulkan 设备指针
		 * @param matrix 初始变换矩阵
		 */
		Mesh(vks::VulkanDevice* device, glm::mat4 matrix);
		~Mesh();
	};

	/*
		glTF skin
		glTF 蒙皮
	*/
	/**
	 * @brief glTF 蒙皮类（用于顶点蒙皮动画）
	 */
	struct Skin {
		std::string name;                              // 蒙皮名称
		Node* skeletonRoot = nullptr;                  // 骨架根节点
		std::vector<glm::mat4> inverseBindMatrices;   // 逆绑定矩阵列表
		std::vector<Node*> joints;                    // 关节节点列表
	};

	/*
		glTF node
		glTF 节点
	*/
	/**
	 * @brief glTF 节点类（场景图节点）
	 */
	struct Node {
		Node* parent;                    // 父节点
		uint32_t index;                  // 节点索引
		std::vector<Node*> children;     // 子节点列表
		glm::mat4 matrix;                // 变换矩阵
		std::string name;                // 节点名称
		Mesh* mesh;                      // 关联的网格
		Skin* skin;                      // 关联的蒙皮
		int32_t skinIndex = -1;          // 蒙皮索引
		glm::vec3 translation{};         // 平移
		glm::vec3 scale{ 1.0f };         // 缩放
		glm::quat rotation{};            // 旋转（四元数）
		/**
		 * @brief 计算局部变换矩阵
		 * @return 局部变换矩阵
		 */
		glm::mat4 localMatrix();
		/**
		 * @brief 获取世界变换矩阵
		 * @return 世界变换矩阵
		 */
		glm::mat4 getMatrix();
		/**
		 * @brief 更新节点（计算变换矩阵）
		 */
		void update();
		~Node();
	};

	/*
		glTF animation channel
		glTF 动画通道
	*/
	/**
	 * @brief glTF 动画通道类
	 */
	struct AnimationChannel {
		enum PathType { TRANSLATION, ROTATION, SCALE };  // 路径类型：平移、旋转、缩放
		PathType path;          // 路径类型
		Node* node;             // 目标节点
		uint32_t samplerIndex; // 采样器索引
	};

	/*
		glTF animation sampler
		glTF 动画采样器
	*/
	/**
	 * @brief glTF 动画采样器类
	 */
	struct AnimationSampler {
		enum InterpolationType { LINEAR, STEP, CUBICSPLINE };  // 插值类型：线性、步进、三次样条
		InterpolationType interpolation;  // 插值类型
		std::vector<float> inputs;        // 输入时间值
		std::vector<glm::vec4> outputsVec4;  // 输出值（vec4）
	};

	/*
		glTF animation
		glTF 动画
	*/
	/**
	 * @brief glTF 动画类
	 */
	struct Animation {
		std::string name;                              // 动画名称
		std::vector<AnimationSampler> samplers;        // 采样器列表
		std::vector<AnimationChannel> channels;       // 通道列表
		float start = std::numeric_limits<float>::max();  // 开始时间
		float end = std::numeric_limits<float>::min();    // 结束时间
	};

	/*
		glTF default vertex layout with easy Vulkan mapping functions
		glTF 默认顶点布局，带有易于使用的 Vulkan 映射函数
	*/
	/**
	 * @brief 顶点组件枚举
	 */
	enum class VertexComponent { Position, Normal, UV, Color, Tangent, Joint0, Weight0 };

	/**
	 * @brief glTF 顶点结构
	 */
	struct Vertex {
		glm::vec3 pos;      // 位置
		glm::vec3 normal;   // 法线
		glm::vec2 uv;       // UV 坐标
		glm::vec4 color;    // 颜色
		glm::vec4 joint0;   // 关节索引（用于蒙皮）
		glm::vec4 weight0;  // 权重（用于蒙皮）
		glm::vec4 tangent;  // 切线
		static VkVertexInputBindingDescription vertexInputBindingDescription;  // 顶点输入绑定描述
		static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;  // 顶点输入属性描述列表
		static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;  // 管线顶点输入状态创建信息
		/**
		 * @brief 创建输入绑定描述
		 * @param binding 绑定索引
		 * @return 顶点输入绑定描述
		 */
		static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);
		/**
		 * @brief 创建输入属性描述
		 * @param binding 绑定索引
		 * @param location 位置索引
		 * @param component 顶点组件类型
		 * @return 顶点输入属性描述
		 */
		static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
		/**
		 * @brief 创建输入属性描述列表
		 * @param binding 绑定索引
		 * @param components 顶点组件列表
		 * @return 顶点输入属性描述列表
		 */
		static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components);
		/** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
		/** @brief 返回请求的顶点组件的默认管线顶点输入状态创建信息结构 */
		static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(const std::vector<VertexComponent> components);
	};

	enum FileLoadingFlags {
		None = 0x00000000,
		PreTransformVertices = 0x00000001,
		PreMultiplyVertexColors = 0x00000002,
		FlipY = 0x00000004,
		DontLoadImages = 0x00000008
	};

	enum RenderFlags {
		BindImages = 0x00000001,
		RenderOpaqueNodes = 0x00000002,
		RenderAlphaMaskedNodes = 0x00000004,
		RenderAlphaBlendedNodes = 0x00000008
	};

	/*
		glTF model loading and rendering class
	*/
	class Model {
	private:
		vkglTF::Texture* getTexture(uint32_t index);
		vkglTF::Texture emptyTexture;
		void createEmptyTexture(VkQueue transferQueue);
	public:
		vks::VulkanDevice* device;
		VkDescriptorPool descriptorPool;

		struct Vertices {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertices;
		struct Indices {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} indices;

		std::vector<Node*> nodes;
		std::vector<Node*> linearNodes;

		std::vector<Skin*> skins;

		std::vector<Texture> textures;
		std::vector<Material> materials;
		std::vector<Animation> animations;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		bool metallicRoughnessWorkflow = true;
		bool buffersBound = false;
		std::string path;

		Model() {};
		~Model();
		void loadNode(vkglTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale);
		void loadSkins(tinygltf::Model& gltfModel);
		void loadImages(tinygltf::Model& gltfModel, vks::VulkanDevice* device, VkQueue transferQueue);
		void loadMaterials(tinygltf::Model& gltfModel);
		void loadAnimations(tinygltf::Model& gltfModel);
		void loadFromFile(std::string filename, vks::VulkanDevice* device, VkQueue transferQueue, uint32_t fileLoadingFlags = vkglTF::FileLoadingFlags::None, float scale = 1.0f);
		void bindBuffers(VkCommandBuffer commandBuffer);
		void drawNode(Node* node, VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageSet = 1);
		void draw(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageSet = 1);
		void getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max);
		void getSceneDimensions();
		void updateAnimation(uint32_t index, float time);
		Node* findNode(Node* parent, uint32_t index);
		Node* nodeFromIndex(uint32_t index);
		void prepareNodeDescriptor(vkglTF::Node* node, VkDescriptorSetLayout descriptorSetLayout);
	};
}