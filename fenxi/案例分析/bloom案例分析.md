# Bloom 案例分析文档

## 目录

1. [概述](#概述)
2. [系统架构](#系统架构)
3. [核心数据结构](#核心数据结构)
4. [渲染流程](#渲染流程)
   - [渲染流程图](#渲染流程图)
   - [阶段一：光晕通道渲染](#阶段一光晕通道渲染offscreen-pass-1)
   - [阶段二：垂直模糊](#阶段二垂直模糊offscreen-pass-2)
   - [阶段三：主场景渲染 + 水平模糊](#阶段三主场景渲染--水平模糊)
5. [着色器详解](#着色器详解)
6. [离屏渲染设置](#离屏渲染设置)
7. [描述符集布局](#描述符集布局)
8. [数据流图](#数据流图)
9. [性能优化要点](#性能优化要点)
10. [可调参数](#可调参数)
11. [关键代码路径](#关键代码路径)
12. [技术要点总结](#技术要点总结)
13. [扩展建议](#扩展建议)
14. [参考资料](#参考资料)

---

## 概述

Bloom（光晕）效果是一种后处理技术，用于模拟明亮光源产生的光晕效果，增强场景的视觉表现力。本案例实现了一个可分离的两遍全屏模糊（Separable Two-Pass Fullscreen Blur）算法，通过离屏渲染（Offscreen Rendering）技术实现 Bloom 效果。

![Bloom 效果渲染结果](../screenshots/bloom.jpg)

*图 1：Bloom 效果最终渲染结果 - UFO 模型带有光晕效果*

## 技术特点

- **可分离高斯模糊**：使用垂直和水平两遍模糊，相比单遍模糊需要更少的采样次数
- **离屏渲染**：使用独立的帧缓冲区进行中间结果渲染
- **多通道渲染**：场景渲染、光晕通道、模糊通道分离
- **实时可调参数**：支持运行时调整模糊强度和缩放

## 系统架构

### 渲染管线架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    Vulkan Bloom 系统架构                      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                      资源层 (Resources)                       │
├─────────────────────────────────────────────────────────────┤
│  Models: ufo, ufoGlow, skyBox                               │
│  Textures: cubemap (天空盒纹理)                              │
│  Uniform Buffers: scene, skyBox, blurParams                 │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│                   描述符集层 (Descriptors)                   │
├─────────────────────────────────────────────────────────────┤
│  blur 描述符集: UBO + 输入纹理                                │
│  scene 描述符集: MVP矩阵 + 纹理 + 参数                        │
│  skyBox 描述符集: MVP矩阵 + 立方体贴图                        │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│                   渲染管线层 (Pipelines)                      │
├─────────────────────────────────────────────────────────────┤
│  glowPass    → 光晕提取                                      │
│  blurVert    → 垂直模糊                                      │
│  blurHorz    → 水平模糊                                      │
│  phongPass   → Phong 光照                                    │
│  skyBox      → 天空盒渲染                                    │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│                   帧缓冲区层 (Framebuffers)                  │
├─────────────────────────────────────────────────────────────┤
│  离屏帧缓冲区 0: 光晕通道输出 (256×256)                      │
│  离屏帧缓冲区 1: 垂直模糊输出 (256×256)                      │
│  主交换链帧缓冲区: 最终输出 (全分辨率)                        │
└─────────────────────────────────────────────────────────────┘
```

*图 12：Bloom 系统架构图*

## 核心数据结构

### 1. 离屏渲染相关结构

```cpp
struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
};

struct FrameBuffer {
    VkFramebuffer framebuffer;
    FrameBufferAttachment color, depth;
    VkDescriptorImageInfo descriptor;
};

struct OffscreenPass {
    int32_t width, height;
    VkRenderPass renderPass;
    VkSampler sampler;
    std::array<FrameBuffer, 2> framebuffers;  // 两个帧缓冲区用于垂直和水平模糊
} offscreenPass;
```

**关键参数**：
- `FB_DIM = 256`：离屏帧缓冲区尺寸（256x256）
- `FB_COLOR_FORMAT = VK_FORMAT_R8G8B8A8_UNORM`：颜色格式

### 2. 模型资源

```cpp
struct {
    vkglTF::Model ufo;        // UFO 主体模型
    vkglTF::Model ufoGlow;    // UFO 光晕模型（单独网格）
    vkglTF::Model skyBox;     // 天空盒模型
} models;
```

### 3. 统一缓冲区（UBO）

```cpp
struct UBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

struct UBOBlurParams {
    float blurScale = 1.0f;      // 模糊缩放
    float blurStrength = 1.5f;   // 模糊强度
};
```

### 4. 渲染管线

```cpp
struct {
    VkPipeline blurVert;    // 垂直模糊管线
    VkPipeline blurHorz;    // 水平模糊管线
    VkPipeline glowPass;    // 光晕通道管线
    VkPipeline phongPass;   // Phong 光照管线
    VkPipeline skyBox;      // 天空盒管线
} pipelines;
```

## 渲染流程

### 渲染流程图

```
┌─────────────────────────────────────────────────────────────┐
│                     Bloom 渲染流程                           │
└─────────────────────────────────────────────────────────────┘

阶段一：光晕通道渲染 (Offscreen Pass 1)
┌─────────────────────────────────────────────────────────────┐
│ 渲染目标: framebuffers[0] (256x256)                         │
│ 管线: glowPass                                              │
│ 模型: ufoGlow (光晕网格)                                    │
│ 输出: 高亮区域颜色                                           │
└─────────────────────────────────────────────────────────────┘
                        ↓
阶段二：垂直模糊 (Offscreen Pass 2)
┌─────────────────────────────────────────────────────────────┐
│ 渲染目标: framebuffers[1] (256x256)                         │
│ 管线: blurVert                                              │
│ 输入: framebuffers[0] 的颜色附件                            │
│ 处理: 垂直方向高斯模糊                                       │
└─────────────────────────────────────────────────────────────┘
                        ↓
阶段三：主场景渲染 + 水平模糊叠加
┌─────────────────────────────────────────────────────────────┐
│ 渲染目标: 主交换链帧缓冲区 (全分辨率)                        │
│ 1. 天空盒渲染 (skyBox pipeline)                             │
│ 2. UFO 主体渲染 (phongPass pipeline)                        │
│ 3. 水平模糊叠加 (blurHorz pipeline + 加法混合)              │
│    输入: framebuffers[1] 的垂直模糊结果                      │
└─────────────────────────────────────────────────────────────┘
                        ↓
                   最终输出
```

*图 2：Bloom 渲染流程示意图*

### 阶段一：光晕通道渲染（Offscreen Pass 1）

**目标**：将 UFO 的光晕部分渲染到第一个离屏帧缓冲区

1. **渲染目标**：`offscreenPass.framebuffers[0]`
2. **使用管线**：`pipelines.glowPass`
3. **渲染模型**：`models.ufoGlow`
4. **着色器**：
   - Vertex: `colorpass.vert` - 标准顶点变换
   - Fragment: `colorpass.frag` - 直接输出顶点颜色

```glsl
// colorpass.frag
void main() {
    outFragColor.rgb = inColor;
}
```

### 阶段二：垂直模糊（Offscreen Pass 2）

**目标**：对第一个帧缓冲区的内容进行垂直方向的高斯模糊

1. **渲染目标**：`offscreenPass.framebuffers[1]`
2. **使用管线**：`pipelines.blurVert`
3. **输入纹理**：`offscreenPass.framebuffers[0].color.view`
4. **着色器**：
   - Vertex: `gaussblur.vert` - 全屏四边形生成
   - Fragment: `gaussblur.frag` - 垂直方向高斯模糊

**高斯模糊算法**：
```glsl
// gaussblur.frag
float weight[5];
weight[0] = 0.227027;   // 中心权重
weight[1] = 0.1945946;
weight[2] = 0.1216216;
weight[3] = 0.054054;
weight[4] = 0.016216;

// 垂直方向采样
for(int i = 1; i < 5; ++i) {
    result += texture(samplerColor, inUV + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
    result += texture(samplerColor, inUV - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
}
```

**高斯模糊采样示意图**：
```
垂直模糊采样模式 (blurdirection = 0):
     ●
     ●
     ●  ← 中心像素 (weight[0])
     ●
     ●

水平模糊采样模式 (blurdirection = 1):
● ● ● ● ●  ← 中心像素 (weight[0])
```

*图 3：高斯模糊采样模式 - 左：垂直模糊，右：水平模糊*

### 阶段三：主场景渲染 + 水平模糊

**目标**：渲染完整场景，并将垂直模糊的结果进行水平模糊后叠加

1. **渲染目标**：主交换链帧缓冲区
2. **渲染顺序**：
   - 天空盒（`pipelines.skyBox`）
   - UFO 主体（`pipelines.phongPass`）
   - 水平模糊叠加（`pipelines.blurHorz`）

**水平模糊**：
- 输入纹理：`offscreenPass.framebuffers[1].color.view`（垂直模糊结果）
- 使用管线：`pipelines.blurHorz`
- 混合模式：加法混合（Additive Blending）

```cpp
// 加法混合配置
blendAttachmentState.blendEnable = VK_TRUE;
blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
```

## 着色器详解

### 1. 高斯模糊着色器（gaussblur.vert / gaussblur.frag）

**顶点着色器**：生成全屏四边形
```glsl
void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
```

**片段着色器**：使用 specialization constant 区分垂直/水平方向
```glsl
layout (constant_id = 0) const int blurdirection = 0;  // 0=垂直, 1=水平
```

### 2. Phong 光照着色器（phongpass.vert / phongpass.frag）

**顶点着色器**：计算光照相关向量
```glsl
vec3 lightPos = vec3(-5.0, -5.0, 0.0);
vec4 pos = ubo.view * ubo.model * inPos;
outNormal = mat3(ubo.view * ubo.model) * inNormal;
outLightVec = lightPos - pos.xyz;
outViewVec = -pos.xyz;
```

**片段着色器**：Phong 光照模型
```glsl
vec3 N = normalize(inNormal);
vec3 L = normalize(inLightVec);
vec3 V = normalize(inViewVec);
vec3 R = reflect(-L, N);
vec3 diffuse = max(dot(N, L), 0.0) * inColor;
vec3 specular = pow(max(dot(R, V), 0.0), 8.0) * vec3(0.75);
```

### 3. 光晕通道着色器（colorpass.vert / colorpass.frag）

直接输出顶点颜色，用于提取高亮区域。

## 离屏渲染设置

### Render Pass 配置

```cpp
// 颜色附件
attchmentDescriptions[0].format = FB_COLOR_FORMAT;
attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

// 深度附件
attchmentDescriptions[1].format = fbDepthFormat;
attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
```

### 子通道依赖

使用三个子通道依赖确保正确的同步：
1. **外部到子通道 0**：深度测试同步
2. **外部到子通道 0**：片段着色器读取到颜色附件写入
3. **子通道 0 到外部**：颜色附件写入到片段着色器读取

```
子通道依赖关系图：

外部 (EXTERNAL)
    │
    ├─→ 依赖 1: 深度测试同步
    │   (EARLY_FRAGMENT_TESTS → EARLY_FRAGMENT_TESTS)
    │
    ├─→ 依赖 2: 着色器读取 → 颜色写入
    │   (FRAGMENT_SHADER → COLOR_ATTACHMENT_OUTPUT)
    │
    ↓
子通道 0 (SUBPASS 0)
    │
    ↓
    ├─→ 依赖 3: 颜色写入 → 着色器读取
    │   (COLOR_ATTACHMENT_OUTPUT → FRAGMENT_SHADER)
    │
外部 (EXTERNAL)
```

*图 4：Render Pass 子通道依赖关系图*

## 描述符集布局

### 模糊描述符集（blur）

```
┌─────────────────────────────────────┐
│      Blur Descriptor Set            │
├─────────────────────────────────────┤
│ Binding 0: UBO (blurParams)         │
│   - blurScale: float                │
│   - blurStrength: float             │
├─────────────────────────────────────┤
│ Binding 1: Combined Image Sampler   │
│   - 输入纹理 (framebuffer color)     │
└─────────────────────────────────────┘
```

*图 13：模糊描述符集布局*

### 场景描述符集（scene）

```
┌─────────────────────────────────────┐
│      Scene Descriptor Set           │
├─────────────────────────────────────┤
│ Binding 0: UBO (MVP matrices)       │
│   - projection: mat4                │
│   - view: mat4                       │
│   - model: mat4                      │
├─────────────────────────────────────┤
│ Binding 1: Combined Image Sampler   │
│   - 纹理贴图 (可选)                  │
├─────────────────────────────────────┤
│ Binding 2: UBO (Fragment params)    │
│   - 片段着色器参数                   │
└─────────────────────────────────────┘
```

*图 14：场景描述符集布局*

## 性能优化要点

1. **可分离模糊**：O(n²) 采样减少到 O(2n)，大幅降低计算量

```
性能对比示意图：

单遍 N×N 模糊:
采样次数 = N² (例如: 5×5 = 25 次采样)

可分离两遍模糊:
第一遍垂直: N 次采样
第二遍水平: N 次采样
总采样次数 = 2N (例如: 5+5 = 10 次采样)

性能提升: 25/10 = 2.5倍
```

*图 5：可分离模糊性能优势示意图*

2. **低分辨率离屏渲染**：256x256 而非全分辨率，减少带宽和计算

```
分辨率对比 (假设主渲染 1920×1080):

全分辨率离屏渲染:
像素数 = 1920 × 1080 = 2,073,600
内存占用 = 2,073,600 × 4 bytes = 8.3 MB

低分辨率离屏渲染 (256×256):
像素数 = 256 × 256 = 65,536
内存占用 = 65,536 × 4 bytes = 0.26 MB

内存节省: 8.3 / 0.26 ≈ 32倍
```

*图 6：低分辨率离屏渲染内存对比*

3. **专用化常量**：使用 specialization constant 避免分支，提高着色器效率
4. **隐式同步**：利用 Render Pass 的子通道依赖，避免显式同步开销

## 数据流图

```
数据在渲染流程中的流动:

┌─────────────────────────────────────────────────────────────┐
│                      输入数据                                │
├─────────────────────────────────────────────────────────────┤
│  • 模型数据: ufo, ufoGlow, skyBox                           │
│  • 纹理数据: cubemap                                        │
│  • 统一缓冲区: MVP矩阵, 模糊参数                             │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│              阶段一: 光晕通道 (Offscreen Pass 1)              │
├─────────────────────────────────────────────────────────────┤
│  输入: ufoGlow 模型 + 顶点颜色                               │
│  处理: colorpass 着色器 (直接输出颜色)                        │
│  输出: framebuffers[0].color (256×256 RGBA)                 │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│              阶段二: 垂直模糊 (Offscreen Pass 2)              │
├─────────────────────────────────────────────────────────────┤
│  输入: framebuffers[0].color                                │
│  处理: gaussblur 着色器 (垂直方向, 5点采样)                   │
│  输出: framebuffers[1].color (256×256 RGBA)                 │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│              阶段三: 主场景渲染 + 水平模糊叠加                 │
├─────────────────────────────────────────────────────────────┤
│  1. 天空盒渲染                                                │
│     输入: skyBox 模型 + cubemap                              │
│     输出: 主帧缓冲区                                         │
│                                                              │
│  2. UFO 主体渲染                                             │
│     输入: ufo 模型 + Phong 光照                              │
│     输出: 主帧缓冲区                                         │
│                                                              │
│  3. 水平模糊叠加                                             │
│     输入: framebuffers[1].color (垂直模糊结果)               │
│     处理: gaussblur 着色器 (水平方向, 5点采样) + 加法混合      │
│     输出: 主帧缓冲区 (最终结果)                              │
└─────────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────────┐
│                      最终输出                                 │
├─────────────────────────────────────────────────────────────┤
│  主交换链帧缓冲区 (全分辨率)                                  │
│  包含: 场景 + Bloom 光晕效果                                  │
└─────────────────────────────────────────────────────────────┘
```

*图 15：Bloom 渲染数据流图*

## 可调参数

通过 UI 可以实时调整：
- **Bloom 开关**：`bloom` 布尔值
- **模糊缩放**：`ubos.blurParams.blurScale` (0.1 - 2.0)
- **模糊强度**：`ubos.blurParams.blurStrength` (默认 1.5)

```
UI 控制面板:
┌─────────────────────────────┐
│  Vulkan Example              │
│  Bloom (offscreen rendering)│
│  ┌─────────────────────────┐ │
│  │ Settings                │ │
│  │ ☑ Bloom                 │ │
│  │ Scale: [1.00] -  +      │ │
│  └─────────────────────────┘ │
└─────────────────────────────┘
```

*图 16：UI 控制面板示意图*

## 关键代码路径

### 初始化流程
```
prepare()
  ├─ loadAssets()              // 加载模型和纹理
  ├─ prepareUniformBuffers()  // 创建统一缓冲区
  ├─ prepareOffscreen()        // 创建离屏渲染资源
  ├─ setupDescriptors()        // 设置描述符集
  └─ preparePipelines()       // 创建渲染管线
```

*图 7：初始化流程树状图*

### 渲染流程
```
buildCommandBuffer()
  ├─ [如果 bloom 开启]
  │   ├─ 渲染光晕到 framebuffers[0]
  │   └─ 垂直模糊到 framebuffers[1]
  └─ 主场景渲染
      ├─ 天空盒
      ├─ UFO 主体
      └─ [如果 bloom 开启] 水平模糊叠加
```

*图 8：渲染流程树状图*

## 技术要点总结

### 1. 两遍模糊的优势

```
单遍模糊 vs 两遍模糊对比:

单遍 5×5 高斯模糊:
采样模式: 5×5 网格 (25个采样点)
计算复杂度: O(N²)

两遍可分离模糊:
第一遍: 5个垂直采样点
第二遍: 5个水平采样点
总采样: 10个采样点
计算复杂度: O(2N)

性能提升: 25/10 = 2.5倍
质量: 几乎无损失 (高斯核可分离)
```

*图 9：单遍模糊 vs 两遍模糊对比图*

### 2. 离屏渲染的必要性

- 需要中间结果进行后处理
- 主交换链不能直接用于中间步骤
- 允许在低分辨率下进行后处理，节省性能

### 3. 加法混合的作用

```
混合公式:
FinalColor = SceneColor + BlurredGlow

加法混合配置:
srcColorBlendFactor = ONE
dstColorBlendFactor = ONE
colorBlendOp = ADD

效果: 光晕叠加到场景上，产生发光效果
```

*图 10：加法混合公式示意图*

### 4. 低分辨率渲染

- 光晕效果不需要高分辨率
- 256x256 足够产生平滑的模糊效果
- 降低内存和带宽占用
- 模糊本身会降低细节，高分辨率意义不大

## 扩展建议

### 1. 多级模糊（Multi-Level Bloom）

```
多级模糊流程:

原始场景 (1920×1080)
    ↓ 降采样
Level 1 (960×540) → 模糊 → 上采样
    ↓ 降采样
Level 2 (480×270) → 模糊 → 上采样
    ↓ 降采样
Level 3 (240×135) → 模糊 → 上采样
    ↓
最终叠加到场景
```

*图 11：多级模糊流程示意图*

### 2. 阈值提取（Brightness Threshold）

只对超过亮度阈值的像素进行模糊处理，提高性能和质量。

### 3. 色调映射（Tone Mapping）

结合 HDR 渲染，实现更真实的光晕效果。

### 4. 性能监控

添加性能计数器，监控模糊通道的耗时。

## 参考资料

- OpenGL Super Bible - 高斯模糊算法参考
- Vulkan 规范 - Render Pass 和子通道依赖
- 可分离卷积（Separable Convolution）算法原理

