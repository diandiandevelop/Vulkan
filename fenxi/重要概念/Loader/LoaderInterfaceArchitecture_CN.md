<!-- markdownlint-disable MD041 -->
[![Khronos Vulkan][1]][2]

[1]: images/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

# Vulkan 加载器接口架构 <!-- omit from toc -->
[![Creative Commons][3]][4]

<!-- Copyright &copy; 2015-2023 LunarG, Inc. -->

[3]: images/Creative_Commons_88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/
## 目录 <!-- omit from toc -->

- [概述](#概述)
  - [谁应该阅读本文档](#谁应该阅读本文档)
  - [加载器](#加载器)
    - [加载器的目标](#加载器的目标)
  - [层](#层)
  - [驱动程序](#驱动程序)
    - [可安装客户端驱动程序](#可安装客户端驱动程序)
  - [VkConfig](#vkconfig)
- [重要的 Vulkan 概念](#重要的-vulkan-概念)
  - [实例与设备](#实例与设备)
    - [实例特定](#实例特定)
      - [实例对象](#实例对象)
      - [实例函数](#实例函数)
      - [实例扩展](#实例扩展)
    - [设备特定](#设备特定)
      - [设备对象](#设备对象)
      - [设备函数](#设备函数)
      - [设备扩展](#设备扩展)
  - [调度表和调用链](#调度表和调用链)
    - [实例调用链示例](#实例调用链示例)
    - [设备调用链示例](#设备调用链示例)
- [提升权限注意事项](#提升权限注意事项)
- [应用程序与加载器的接口](#应用程序与加载器的接口)
- [层与加载器的接口](#层与加载器的接口)
- [驱动程序与加载器的接口](#驱动程序与加载器的接口)
- [调试问题](#调试问题)
- [加载器策略](#加载器策略)
- [过滤环境变量行为](#过滤环境变量行为)
  - [比较字符串](#比较字符串)
  - [逗号分隔列表](#逗号分隔列表)
  - [Glob 模式](#glob-模式)
  - [不区分大小写](#不区分大小写)
  - [环境变量优先级](#环境变量优先级)
- [调试环境变量表](#调试环境变量表)
  - [活动环境变量](#活动环境变量)
  - [已弃用的环境变量](#已弃用的环境变量)
- [术语表](#术语表)

## 概述

Vulkan 是一个分层架构，由以下元素组成：
  * Vulkan 应用程序
  * [Vulkan 加载器](#加载器)
  * [Vulkan 层](#层)
  * [驱动程序](#驱动程序)
  * [VkConfig](#vkconfig)

![High Level View of Loader](images/high_level_loader.png)

本文档中的一般概念适用于
Windows、Linux、Android 和 macOS 系统上可用的加载器。


### 谁应该阅读本文档

虽然本文档主要面向 Vulkan 应用程序、
驱动程序和层的开发人员，但其中包含的信息可能对任何
想要更好地理解 Vulkan 运行时的人都有用。


### 加载器

应用程序位于顶部，直接与 Vulkan
加载器接口。
在堆栈底部是驱动程序。
驱动程序可以控制一个或多个能够渲染 Vulkan 的物理设备，
实现从 Vulkan 到本机图形 API 的转换（如
[MoltenVk](https://github.com/KhronosGroup/MoltenVK)，或实现完全
软件路径，可以在 CPU 上执行以模拟 Vulkan 设备（如
[SwiftShader](https://github.com/google/swiftshader) 或 LavaPipe）。
请记住，支持 Vulkan 的硬件可能是基于图形的、基于计算的，或
两者兼而有之。
在应用程序和驱动程序之间，加载器可以注入任意数量的
可选的[层](#层)，这些层提供特殊功能。
加载器对于管理 Vulkan
函数到适当的层和驱动程序集的正确调度至关重要。
Vulkan 对象模型允许加载器将层插入到调用链中
，以便层可以在调用驱动程序之前
处理 Vulkan 函数。

本文档旨在提供这些元素之间必要接口的
概述。


#### 加载器的目标

加载器的设计考虑了以下目标：
 1. 在用户系统上支持一个或多个支持 Vulkan 的驱动程序，而不会
 相互干扰。
 2. 支持 Vulkan 层，这些层是可选的模块，可以由
应用程序、开发人员或标准系统设置启用。
 3. 将加载器的总体开销保持在最低水平。


### 层

层是可选的组件，用于增强 Vulkan 开发环境。
它们可以拦截、评估和修改现有的 Vulkan 函数，在
从应用程序到驱动程序再返回的路径上。
层被实现为库，可以通过不同方式启用
，并在 CreateInstance 期间加载。
每个层可以选择挂钩或拦截 Vulkan 函数，这些函数
可以依次被忽略、检查或增强。
层未挂钩的任何函数都会简单地跳过该层，
控制流将简单地继续到下一个支持的层或
驱动程序。
因此，层可以选择是拦截所有已知的 Vulkan
函数还是仅拦截它感兴趣的子集。

层可能公开的功能示例包括：
 * 验证 API 使用
 * 跟踪 API 调用
 * 调试辅助
 * 性能分析
 * 覆盖层

因为层是可选的并且是动态加载的，它们可以根据需要
启用和禁用。
例如，在开发和调试应用程序时，启用
某些层可以帮助确保它正确使用 Vulkan API。
但在发布应用程序时，这些层是不必要的
，因此不会被启用，从而提高应用程序的速度。


### 驱动程序

实现 Vulkan 的库，无论是通过直接支持物理
硬件设备、将 Vulkan 命令转换为本机图形
命令，还是通过软件模拟 Vulkan，都被视为"驱动程序"。
最常见的驱动程序类型仍然是可安装客户端驱动程序（或 ICD）。
加载器负责发现系统上可用的 Vulkan 驱动程序。
给定可用驱动程序列表，加载器可以枚举所有可用的
物理设备，并为应用程序提供此信息。


#### 可安装客户端驱动程序

Vulkan 允许多个 ICD，每个 ICD 支持一个或多个设备。
这些设备中的每一个都由 Vulkan `VkPhysicalDevice` 对象表示。
加载器负责通过系统上的标准
驱动程序搜索发现可用的 Vulkan ICD。


### VkConfig

VkConfig 是 LunarG 开发的一个工具，用于协助修改本地系统上的 Vulkan
环境。
它可以用于查找层、启用它们、更改层设置和其他
有用功能。
VkConfig 可以通过安装
[Vulkan SDK](https://vulkan.lunarg.com/) 或从
[LunarG VulkanTools GitHub 仓库](https://github.com/LunarG/VulkanTools) 构建源代码来找到。

VkConfig 生成三个输出，其中两个与 Vulkan 加载器和
层一起工作。
这些输出是：
 * Vulkan 覆盖层
 * Vulkan 层设置文件
 * VkConfig 配置设置

这些文件根据您的平台位于不同位置：

<table style="width:100%">
  <tr>
    <th>平台</th>
    <th>输出</th>
    <th>位置</th>
  </tr>
  <tr>
    <th rowspan="3">Linux</th>
    <td>Vulkan 覆盖层</td>
    <td>$USER/.local/share/vulkan/implicit_layer.d/VkLayer_override.json</td>
  </tr>
  <tr>
    <td>Vulkan 层设置</td>
    <td>$USER/.local/share/vulkan/settings.d/vk_layer_settings.txt</td>
  </tr>
  <tr>
    <td>VkConfig 配置设置</td>
    <td>$USER/.local/share/vulkan/settings.d/vk_layer_settings.txt</td>
  </tr>
  <tr>
    <th rowspan="3">Windows</th>
    <td>Vulkan 覆盖层</td>
    <td>%HOME%\AppData\Local\LunarG\vkconfig\override\VkLayerOverride.json</td>
  </tr>
  <tr>
    <td>Vulkan 层设置</td>
    <td>(注册表) HKEY_CURRENT_USER\Software\Khronos\Vulkan\LoaderSettings</td>
  </tr>
  <tr>
    <td>VkConfig 配置设置</td>
    <td>(注册表) HKEY_CURRENT_USER\Software\LunarG\vkconfig </td>
  </tr>
</table>

[覆盖元层](./LoaderLayerInterface.md#override-meta-layer)是
VkConfig 工作方式的重要组成部分。
当加载器找到此层时，它会强制加载在 VkConfig 内启用的所需层
，并禁用那些
被有意禁用的层（包括隐式层）。

Vulkan 层设置文件可用于指定某些行为和
每个启用的层预期执行的操作。
这些设置也可以由 VkConfig 控制，或者可以手动
启用。
有关可以使用哪些设置的详细信息，请参阅各个层。

将来，VkConfig 可能与 Vulkan
加载器有更多交互。

有关 VkConfig 的更多详细信息，请参阅其
[GitHub 文档](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md)。
<br/>
<br/>


## 重要的 Vulkan 概念

Vulkan 有一些概念为其组织提供了基础。
任何试图使用 Vulkan 或
开发其任何组件的人都应该理解这些概念。


### 实例与设备

一个重要的概念需要理解，在本文档中反复提到
，是 Vulkan API 的组织方式。
Vulkan 中的许多对象、函数、扩展和其他行为可以
分为两组：
 * [实例特定](#实例特定)
 * [设备特定](#设备特定)


#### 实例特定

"Vulkan 实例"（`VkInstance`）是一个高级构造，用于提供
Vulkan 系统级信息和功能。

##### 实例对象

与实例直接关联的一些 Vulkan 对象是：
 * `VkInstance`
 * `VkPhysicalDevice`
 * `VkPhysicalDeviceGroup`

##### 实例函数

"实例函数"是第一个参数是
[实例对象](#实例对象)或根本没有对象的任何 Vulkan 函数。

一些 Vulkan 实例函数是：
 * `vkEnumerateInstanceExtensionProperties`
 * `vkEnumeratePhysicalDevices`
 * `vkCreateInstance`
 * `vkDestroyInstance`

应用程序可以通过
Vulkan 加载器的头文件直接链接到所有核心实例函数。
或者，应用程序可以使用
`vkGetInstanceProcAddr` 查询函数指针。
`vkGetInstanceProcAddr` 可用于查询任何实例或设备入口点
，以及所有核心入口点。

如果使用 `VkInstance` 调用 `vkGetInstanceProcAddr`，则返回的任何函数
指针特定于该 `VkInstance` 以及从它创建的
任何其他对象。

##### 实例扩展

Vulkan 的扩展类似地根据它们提供的
函数类型进行关联。
因此，扩展被分为实例或设备扩展
，其中扩展中的大多数（如果不是全部）函数都是
相应类型。
例如，"实例扩展"主要由"实例
函数"组成，这些函数主要接受实例对象。
这些将在后面更详细地讨论。


#### 设备特定

另一方面，Vulkan 设备（`VkDevice`）是一个逻辑标识符，用于
通过用户系统上的特定驱动程序将函数与特定的 Vulkan 物理设备
（`VkPhysicalDevice`）关联。

##### 设备对象

与设备直接关联的一些 Vulkan 构造包括：
 * `VkDevice`
 * `VkQueue`
 * `VkCommandBuffer`

##### 设备函数

"设备函数"是任何将任何设备对象作为其
第一个参数或设备的子对象的 Vulkan 函数。
绝大多数 Vulkan 函数都是设备函数。
一些 Vulkan 设备函数是：
 * `vkQueueSubmit`
 * `vkBeginCommandBuffer`
 * `vkCreateEvent`

Vulkan 设备函数可以使用 `vkGetInstanceProcAddr` 或
`vkGetDeviceProcAddr` 查询。
如果应用程序选择使用 `vkGetInstanceProcAddr`，每次调用都会有
内置到调用链中的额外函数调用，这将略微降低
性能。
相反，如果应用程序使用 `vkGetDeviceProcAddr`，调用链将
针对特定设备进行更多优化，但返回的函数指针将
**仅**适用于查询它们时使用的设备。
与 `vkGetInstanceProcAddr` 不同，`vkGetDeviceProcAddr` 只能用于
Vulkan 设备函数。

最佳解决方案是使用
`vkGetInstanceProcAddr` 查询实例扩展函数，并使用
`vkGetDeviceProcAddr` 查询设备扩展函数。
有关此内容的更多信息，请参阅
[LoaderApplicationInterface.md](LoaderApplicationInterface.md) 文档中的
[最佳应用程序性能设置](LoaderApplicationInterface.md#best-application-performance-setup)
部分。

##### 设备扩展

与实例扩展一样，设备扩展是一组 Vulkan 设备
函数，用于扩展 Vulkan 语言。
有关设备扩展的更多信息可以在本文档后面找到。


### 调度表和调用链

Vulkan 使用对象模型来控制特定操作或
操作的范围。
要操作的对象始终是 Vulkan 调用的第一个参数，并且是
可调度对象（请参阅 Vulkan 规范第 3.3 节对象模型）。
在底层，可调度对象句柄是指向结构的指针，
该结构又包含指向由加载器维护的调度表的指针。
此调度表包含指向适合
该对象的 Vulkan 函数的指针。

加载器维护两种类型的调度表：
 - 实例调度表
   - 在调用 `vkCreateInstance` 期间在加载器中创建
 - 设备调度表
   - 在调用 `vkCreateDevice` 期间在加载器中创建

此时，应用程序和系统都可以指定要包含的可选层
。
加载器将初始化指定的层，为每个
Vulkan 函数创建调用链，调度表的每个条目将指向该链的
第一个元素。
因此，加载器为创建的每个 `VkInstance` 构建一个实例调用链
，为创建的每个 `VkDevice` 构建一个设备调用链。

当应用程序调用 Vulkan 函数时，这通常会首先命中加载器中的
*跳板*函数。
这些*跳板*函数是小的、简单的函数，它们跳转到
它们给定的对象的适当调度表条目。
此外，对于实例调用链中的函数，加载器有一个
附加函数，称为*终止符*，在所有启用的
层之后调用，以将适当的信息编组到所有可用的驱动程序。


#### 实例调用链示例

例如，下面的图表表示在 `vkCreateInstance` 的调用链中
发生的情况。
初始化链后，加载器调用第一个层的
`vkCreateInstance`，它将调用下一个层的 `vkCreateInstance`
，然后最终在加载器中再次终止，它将调用
每个驱动程序的 `vkCreateInstance`。
这允许链中的每个启用层根据
应用程序的 `VkInstanceCreateInfo` 结构设置它需要的内容。

![Instance Call Chain](images/loader_instance_chain.png)

这也突出了加载器在使用
实例调用链时必须管理的一些复杂性。
如此处所示，加载器的*终止符*必须在存在多个驱动程序时聚合
来自多个驱动程序的信息。
这意味着加载器必须知道任何实例级扩展
，这些扩展在 `VkInstance` 上工作以正确聚合它们。


#### 设备调用链示例

设备调用链在 `vkCreateDevice` 中创建，通常更简单
，因为它们只处理单个设备。
这允许公开此设备的特定驱动程序始终是
链的*终止符*。

![Loader Device Call Chain](images/loader_device_chain_loader.png)
<br/>


## 提升权限注意事项

为了确保系统免受利用，以提升权限运行的 Vulkan 应用程序
被限制执行某些操作，例如
从不安全的位置读取环境变量或在
用户控制的路径中搜索文件。
这样做是为了确保以提升权限运行的应用程序
不会使用未安装在适当批准位置的组件运行。

加载器使用平台特定的机制（如 `secure_getenv` 及其
等效项）来查询敏感环境变量，以避免意外
使用不受信任的结果。

这些行为还会导致忽略某些环境变量，例如：

  * `VK_DRIVER_FILES` / `VK_ICD_FILENAMES`
  * `VK_ADD_DRIVER_FILES`
  * `VK_LAYER_PATH`
  * `VK_ADD_LAYER_PATH`
  * `VK_IMPLICIT_LAYER_PATH`
  * `VK_ADD_IMPLICIT_LAYER_PATH`
  * `XDG_CONFIG_HOME`（Linux/Mac 特定）
  * `XDG_DATA_HOME`（Linux/Mac 特定）

有关受影响搜索路径的更多信息，请参阅
[层发现](LoaderLayerInterface.md#layer-discovery)和
[驱动程序发现](LoaderDriverInterface.md#driver-discovery)。
<br/>
<br/>


## 应用程序与加载器的接口

应用程序与 Vulkan 加载器的接口现在在
[LoaderApplicationInterface.md](LoaderApplicationInterface.md) 文档中详细说明，该文档位于
与此文件相同的目录中。
<br/>
<br/>


## 层与加载器的接口

层与 Vulkan 加载器的接口在
[LoaderLayerInterface.md](LoaderLayerInterface.md) 文档中详细说明，该文档位于与此文件相同的
目录中。
<br/>
<br/>


## 驱动程序与加载器的接口

驱动程序与 Vulkan 加载器的接口在
[LoaderDriverInterface.md](LoaderDriverInterface.md) 文档中详细说明，该文档位于与此文件相同的
目录中。
<br/>
<br/>


## 调试问题


如果您的应用程序崩溃或行为异常，加载器提供了
几种机制供您调试问题。
这些在 [LoaderDebugging.md](LoaderDebugging.md) 文档中详细说明
，该文档位于与此文件相同的目录中。
<br/>
<br/>


## 加载器策略

关于加载器与驱动程序和层交互的加载器策略
现在记录在相应的部分中。
这些部分的目的是明确定义加载器
与这些组件交互的预期行为。
在可能需要符合现有加载器行为的新的或专门的加载器的情况下
，这可能特别有用。
因此，这些部分的主要重点是
所有相关组件的预期行为，以跨平台创建一致的体验。
从长远来看，这也可以用作任何
现有 Vulkan 加载器的验证要求。

要查看特定的策略部分，请参阅下面列出的
一个或两个部分：
 * [加载器和驱动程序策略](LoaderDriverInterface.md#loader-and-driver-policy)
 * [加载器和层策略](LoaderLayerInterface.md#loader-and-layer-policy)
<br/>
<br/>

## 过滤环境变量行为

在某些区域提供的过滤环境变量有一些常见的
限制和行为，应该列出。

### 比较字符串

过滤变量将与驱动程序或层的
适当字符串进行比较。
层的适当字符串是层
清单文件中提供的层名称。
由于驱动程序不像层那样有名称，因此此子字符串用于与
驱动程序清单的文件名进行比较。

### 逗号分隔列表

所有过滤环境变量都接受逗号分隔的输入。
因此，您可以将多个字符串链接在一起，它将使用这些字符串
来单独启用或禁用当前可用项列表中的
相应项。

### Glob 模式

为了提供足够的灵活性，将名称搜索限制为仅开发人员想要的
名称，加载器对字符串使用有限的 glob 格式。
可接受的 glob 是：
 - 前缀：   `"string*"`
 - 后缀：   `"*string"`
 - 子字符串：  `"*string*"`
 - 完整字符串： `"string"`
   - 对于完整字符串，字符串将与每个
     层或驱动程序文件名进行完整比较。
   - 因此，它只会匹配特定目标，例如：
     `VK_LAYER_KHRONOS_validation` 将匹配层名称
     `VK_LAYER_KHRONOS_validation`，但**不会**匹配名为
     `VK_LAYER_KHRONOS_validation2` 的层（没有这样的层）。

这特别有用，因为有时很难确定
驱动程序清单文件的完整名称，甚至一些常用的层
，如 `VK_LAYER_KHRONOS_validation`。

### 不区分大小写

所有过滤环境变量都假定 glob 内的字符串
不区分大小写。
因此，"Bob"、"bob" 和 "BOB" 都是相同的东西。

### 环境变量优先级

*禁用*环境变量的值将在
*启用*或*选择*环境变量**之前**考虑。
因此，可以使用*禁用*
环境变量禁用层/驱动程序，然后由*启用*/*选择*
环境变量重新启用它。
如果您禁用所有层/驱动程序，目的是仅
启用特定层/驱动程序的较小子集以进行问题分类，这很有用。

### 多重过滤

当提供多个 `VK_LOADER_<DEVICE|VENDOR|DRIVER>_ID_FILTER` 时，它们
累积应用。只有匹配所有过滤器的设备才会呈现给
应用程序。

## 调试环境变量表

以下是可用于
加载器的所有调试环境变量。
这些在文本中引用，但为了便于
发现而收集在这里。

### 活动环境变量

<table style="width:100%">
  <tr>
    <th>环境变量</th>
    <th>行为</th>
    <th>限制</th>
    <th>示例格式</th>
  </tr>
  <tr>
    <td><small>
        <i>VK_ADD_DRIVER_FILES</i>
    </small></td>
    <td><small>
        提供加载器将使用的附加驱动程序 JSON 文件列表
        ，除了加载器通常找到的驱动程序之外。
        驱动程序列表将首先添加，在通常
        找到的驱动程序列表之前。
        该值包含驱动程序 JSON 清单文件的
        分隔完整路径列表。<br/>
    </small></td>
    <td><small>
        如果不使用 JSON 文件的全局路径，可能会遇到问题。
        <br/> <br/>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_ADD_DRIVER_FILES=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<folder_a>/intel.json:<folder_b>/amd.json
        <br/> <br/>
        set<br/>
        &nbsp;&nbsp;VK_ADD_DRIVER_FILES=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<folder_a>\nvidia.json;<folder_b>\mesa.json
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_ADD_LAYER_PATH</i>
    </small></td>
    <td><small>
        提供加载器将用于搜索
        显式层的附加路径列表，除了加载器的标准层库
        搜索路径（在查找层清单文件时）。
        路径将首先添加，在通常
        搜索的文件夹列表之前。
    </small></td>
    <td><small>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_ADD_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;:&lt;path_b&gt;<br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_ADD_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;;&lt;path_b&gt;</small>
    </td>
  </tr>
    <tr>
    <td><small>
        <i>VK_ADD_IMPLICIT_LAYER_PATH</i>
    </small></td>
    <td><small>
        提供加载器将用于搜索
        隐式层的附加路径列表，除了加载器的标准层库
        搜索路径（在查找层清单文件时）。
        路径将首先添加，在通常
        搜索的文件夹列表之前。
    </small></td>
    <td><small>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_ADD_IMPLICIT_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;:&lt;path_b&gt;<br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_ADD_IMPLICIT_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;;&lt;path_b&gt;</small>
    </td>
  </tr>
  <tr>
    <td><small>
        <i>VK_DRIVER_FILES</i>
    </small></td>
    <td><small>
        强制加载器使用特定的驱动程序 JSON 文件。
        该值包含驱动程序 JSON 清单文件的
        分隔完整路径列表和/或
        包含驱动程序 JSON 文件的文件夹路径。<br/>
        <br/>
        这已取代较旧的已弃用环境变量
        <i>VK_ICD_FILENAMES</i>，但旧的环境变量将
        继续工作。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.207 的 Vulkan 头文件的加载器中可用。<br/>
        建议使用 JSON 文件的绝对路径。
        由于加载器将相对库
        路径转换为绝对路径的方式，相对路径可能会出现问题。
        <br/> <br/>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_DRIVER_FILES=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<folder_a>/intel.json:<folder_b>/amd.json
        <br/> <br/>
        set<br/>
        &nbsp;&nbsp;VK_DRIVER_FILES=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<folder_a>\nvidia.json;<folder_b>\mesa.json
        </small>
    </td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LAYER_PATH</i></small></td>
    <td><small>
        覆盖加载器的标准显式层搜索路径，并使用
        提供的分隔文件和/或文件夹来定位层清单文件。
    </small></td>
    <td><small>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;:&lt;path_b&gt;<br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;;&lt;path_b&gt;
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_IMPLICIT_LAYER_PATH</i></small></td>
    <td><small>
        覆盖加载器的标准隐式层搜索路径，并使用
        提供的分隔文件和/或文件夹来定位层清单文件。
    </small></td>
    <td><small>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_IMPLICIT_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;:&lt;path_b&gt;<br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_IMPLICIT_LAYER_PATH=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;path_a&gt;;&lt;path_b&gt;
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DEBUG</i>
    </small></td>
    <td><small>
        使用逗号分隔的级别
        选项列表启用加载器调试消息。这些选项是：<br/>
        &nbsp;&nbsp;* error（仅错误）<br/>
        &nbsp;&nbsp;* warn（仅警告）<br/>
        &nbsp;&nbsp;* info（仅信息）<br/>
        &nbsp;&nbsp;* debug（仅调试）<br/>
        &nbsp;&nbsp;* layer（层特定输出）<br/>
        &nbsp;&nbsp;* driver（驱动程序特定输出）<br/>
        &nbsp;&nbsp;* all（报告所有消息）<br/><br/>
        要启用多个选项（除了 "all"），如信息、警告和
        错误消息，请将值设置为 "error,warn,info"。
    </small></td>
    <td><small>
        无
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DEBUG=all<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DEBUG=warn
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DEVICE_SELECT</i>
    </small></td>
    <td><small>
        允许用户强制特定设备在所有
        其他设备之前优先，在 <i>vkGetPhysicalDevices<i> 和
        <i>vkGetPhysicalDeviceGroups<i> 函数的返回顺序中。<br/>
        该值应为 "<十六进制供应商 id>:<十六进制设备 id>"。<br/>
        <b>注意：</b> 这不会从列表中删除设备，只会重新排序。
    </small></td>
    <td><small>
        <b>仅 Linux</b>
    </small></td>
    <td><small>
        set VK_LOADER_DEVICE_SELECT=0x10de:0x1f91
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DISABLE_SELECT</i>
    </small></td>
    <td><small>
        允许用户禁用加载器在将物理设备集返回给层之前运行的
        一致排序算法。<br/>
    </small></td>
    <td><small>
        <b>仅 Linux</b>
    </small></td>
    <td><small>
        set VK_LOADER_DISABLE_SELECT=1
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DISABLE_INST_EXT_FILTER</i>
    </small></td>
    <td><small>
        禁用过滤掉加载器不知道的实例扩展。
        这将允许应用程序启用驱动程序公开的实例扩展，但加载器不支持这些扩展。<br/>
    </small></td>
    <td><small>
        <b>谨慎使用！</b> 这可能导致加载器或应用程序崩溃。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DISABLE_INST_EXT_FILTER=1<br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DISABLE_INST_EXT_FILTER=1
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DRIVERS_SELECT</i>
    </small></td>
    <td><small>
        在已知驱动程序中搜索的逗号分隔 glob 列表
        ，用于仅选择清单文件名与一个或多个
        提供的 glob 匹配的驱动程序。<br/>
        由于驱动程序不像层那样有名称，因此此 glob 用于
        与清单文件名进行比较。
        已知驱动程序清单是加载器已经找到的文件
        ，考虑了默认搜索路径和其他环境
        变量（如 <i>VK_ICD_FILENAMES</i> 或 <i>VK_ADD_DRIVER_FILES</i>）。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.234 的 Vulkan 头文件的加载器中可用。<br/>
        如果未找到清单文件名与任何提供的
        glob 匹配的驱动程序，则不会启用任何驱动程序，它<b>可能</b>导致
        Vulkan 应用程序无法正常运行。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DRIVERS_SELECT=nvidia*<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DRIVERS_SELECT=nvidia*<br/><br/>
        如果 Nvidia 驱动程序存在于系统上并且已经对加载器可见，则上述将仅选择 Nvidia 驱动程序。
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DRIVERS_DISABLE</i>
    </small></td>
    <td><small>
        在已知驱动程序中搜索的逗号分隔 glob 列表
        ，用于仅禁用清单文件名与一个或多个
        提供的 glob 匹配的驱动程序。<br/>
        由于驱动程序不像层那样有名称，因此此 glob 用于
        与清单文件名进行比较。
        已知驱动程序清单是加载器已经找到的文件
        ，考虑了默认搜索路径和其他环境
        变量（如 <i>VK_ICD_FILENAMES</i> 或 <i>VK_ADD_DRIVER_FILES</i>）。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.234 的 Vulkan 头文件的加载器中可用。<br/>
        如果使用此环境变量禁用所有可用驱动程序，
        则加载器将找不到任何驱动程序，并且<b>将</b>导致
        Vulkan 应用程序无法正常运行。<br/>
        这也在其他驱动程序环境变量（如
        <i>VK_LOADER_DRIVERS_SELECT</i>）之前检查，以便用户可以轻松禁用所有
        驱动程序，然后使用
        启用环境变量有选择地重新启用单个驱动程序。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DRIVERS_DISABLE=*amd*,*intel*<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DRIVERS_DISABLE=*amd*,*intel*<br/><br/>
        如果 Intel 和 AMD 驱动程序都存在于系统上并且已经对加载器可见，则上述将禁用 Intel 和 AMD 驱动程序。
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_LAYERS_ENABLE</i>
    </small></td>
    <td><small>
        在已知层中搜索的逗号分隔 glob 列表
        ，用于仅选择层名称与一个或多个
        提供的 glob 匹配的层。<br/>
        已知层是加载器找到的层，考虑了
        默认搜索路径和其他环境变量
        （如 <i>VK_LAYER_PATH</i>）。
        </i>
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.234 的 Vulkan 头文件的加载器中可用。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_LAYERS_ENABLE=*validation,*recon*<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_LAYERS_ENABLE=*validation,*recon*<br/><br/>
        如果 Khronos 验证层和
        GfxReconstruct 层都存在于系统上并且已经对加载器可见，则上述将启用它们。
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_LAYERS_DISABLE</i>
    </small></td>
    <td><small>
        在已知层中搜索的逗号分隔 glob 列表
        ，用于仅禁用层名称与一个或多个
        提供的 glob 匹配的层。<br/>
        已知层是加载器找到的层，考虑了
        默认搜索路径和其他环境变量
        （如 <i>VK_LAYER_PATH</i>）。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.234 的 Vulkan 头文件的加载器中可用。<br/>
        禁用应用程序有意启用为
        显式层的层<b>可能</b>导致应用程序无法
        正常工作。<br/>
        这也在其他层环境变量（如
        <i>VK_LOADER_LAYERS_ENABLE</i>）之前检查，以便用户可以轻松禁用所有
        层，然后使用
        启用环境变量有选择地重新启用单个层。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_LAYERS_DISABLE=*MESA*,~implicit~<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_LAYERS_DISABLE=*MESA*,~implicit~<br/><br/>
        上述将禁用任何 Mesa 层和所有其他
        通常在系统上启用的隐式层。
    </small></td>
  </tr>
  <tr>
  <td><small>
    <i>VK_LOADER_LAYERS_ALLOW</i>
    </small></td>
  <td><small>
        在已知层中搜索的逗号分隔 glob 列表
        ，用于防止层名称与一个或多个
        提供的 glob 匹配的层被 <i>VK_LOADER_LAYERS_DISABLE</i> 禁用。<br/>
        已知层是加载器找到的层，考虑了
        默认搜索路径和其他环境变量
        （如 <i>VK_LAYER_PATH</i>）。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.262 的 Vulkan 头文件的加载器中可用。<br/>
        如果启用它们的正常机制
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_LAYERS_ALLOW=*validation*,*recon*<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_LAYERS_ALLOW=*validation*,*recon*<br/><br/>
        上述将允许任何名称是 validation 或 recon 的层被
        启用，无论 <i>VK_LOADER_LAYERS_DISABLE</i> 的值如何。
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING</i>
    </small></td>
    <td><small>
        如果设置为 "1"，会导致加载器在 vkDestroyInstance 期间不卸载动态库。
        此选项允许泄漏清理器具有完整的堆栈跟踪。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.3.259 的 Vulkan 头文件的加载器中可用。<br/>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING=1<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DISABLE_DYNAMIC_LIBRARY_UNLOADING=1<br/><br/>
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DEVICE_ID_FILTER</i>
    </small></td>
    <td><small>
        如果设置，会导致加载器仅枚举匹配
        过滤器的物理设备。过滤器是设备 id 或设备 id
        范围的逗号分隔列表，其中 id 可以作为十进制或十六进制值提供，范围
        是冒号分隔的。如果任何过滤器匹配其
        设备 id（由 <i>VkPhysicalDeviceProperties::deviceID<i> 提供），则设备通过。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.4.326 的 Vulkan 头文件的加载器中可用。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DEVICE_ID_FILTER=0x7460:0x747e,29827<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DEVICE_ID_FILTER=0x7460:0x747e,29827<br/><br/>
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_VENDOR_ID_FILTER</i>
    </small></td>
    <td><small>
        如果设置，会导致加载器仅枚举匹配
        过滤器的物理设备。过滤器是供应商 id 或供应商 id
        范围的逗号分隔列表，其中 id 可以作为十进制或十六进制值提供，范围
        是冒号分隔的。如果任何过滤器匹配其
        供应商 id（由 <i>VkPhysicalDeviceProperties::vendorID<i> 提供），则设备通过。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.4.326 的 Vulkan 头文件的加载器中可用。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_VENDOR_ID_FILTER=65541<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_VENDOR_ID_FILTER=65541<br/><br/>
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_LOADER_DRIVER_ID_FILTER</i>
    </small></td>
    <td><small>
        如果设置，会导致加载器仅枚举匹配
        过滤器的物理设备。过滤器是驱动程序 id 或驱动程序 id
        范围的逗号分隔列表，其中 id 可以作为十进制或十六进制值提供，范围
        是冒号分隔的。如果任何过滤器匹配其
        驱动程序 id（由 <i>VkPhysicalDeviceDriverProperties::driverID<i> 提供），则设备通过。
    </small></td>
    <td><small>
        此功能仅在构建时使用版本
        1.4.326 的 Vulkan 头文件的加载器中可用。
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_LOADER_DRIVER_ID_FILTER=1-3:13<br/>
        <br/>
        set<br/>
        &nbsp;&nbsp;VK_LOADER_DRIVER_ID_FILTER=1-3:13<br/><br/>
    </small></td>
  </tr>
</table>

<br/>

### 已弃用的环境变量

这些环境变量仍然有效并受支持，但支持
可能在未来的加载器版本中移除。

<table style="width:100%">
  <tr>
    <th>环境变量</th>
    <th>行为</th>
    <th>替换为</th>
    <th>限制</th>
    <th>示例格式</th>
  </tr>
  <tr>
    <td><small><i>VK_ICD_FILENAMES</i></small></td>
    <td><small>
            强制加载器使用特定的驱动程序 JSON 文件。
            该值包含驱动程序 JSON 清单文件的
            分隔完整路径列表。<br/>
            <br/>
            <b>注意：</b> 如果不使用 JSON 文件的全局路径，可能会遇到
            问题。<br/>
    </small></td>
    <td><small>
        这已被 <i>VK_DRIVER_FILES</i> 取代。
    </small></td>
    <td><small>
        <a href="#提升权限注意事项">
            在以提升权限运行 Vulkan 应用程序时忽略。
        </a>
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_ICD_FILENAMES=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<folder_a>/intel.json:<folder_b>/amd.json
        <br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_ICD_FILENAMES=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<folder_a>\nvidia.json;<folder_b>\mesa.json
    </small></td>
  </tr>
  <tr>
    <td><small>
        <i>VK_INSTANCE_LAYERS</i>
    </small></td>
    <td><small>
        强制加载器将给定的层添加到通常传递到 <b>vkCreateInstance</b> 的
        已启用层列表。
        这些层首先添加，加载器将删除
        在此列表以及传递到
        <i>ppEnabledLayerNames</i> 的列表中出现的任何重复层。
    </small></td>
    <td><small>
        它覆盖使用
        <i>VK_LOADER_LAYERS_DISABLE</i> 禁用的任何层。
    </small></td>
    <td><small>
        无
    </small></td>
    <td><small>
        export<br/>
        &nbsp;&nbsp;VK_INSTANCE_LAYERS=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;layer_a&gt;;&lt;layer_b&gt;<br/><br/>
        set<br/>
        &nbsp;&nbsp;VK_INSTANCE_LAYERS=<br/>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;layer_a&gt;;&lt;layer_b&gt;
    </small></td>
  </tr>
</table>
<br/>
<br/>

## 术语表

<table style="width:100%">
  <tr>
    <th>字段名称</th>
    <th>字段值</th>
  </tr>
  <tr>
    <td>Android 加载器</td>
    <td>主要为 Android OS 设计的加载器。
        这是从与 Khronos 加载器不同的代码库生成的。
        但是，在所有重要方面，它应该在功能上等效。
    </td>
  </tr>
  <tr>
    <td>Khronos 加载器</td>
    <td>由 Khronos 发布的加载器，目前主要设计用于
        Windows、Linux、macOS、Stadia 和 Fuchsia。
        这是从与 Android 加载器不同的
        <a href="https://github.com/KhronosGroup/Vulkan-Loader">代码库</a>
        生成的。
        但在所有重要方面，它应该在功能上等效。
    </td>
  </tr>
  <tr>
    <td>核心函数</td>
    <td>已经是 Vulkan 核心规范一部分且不是
        扩展的函数。<br/>
        例如，<b>vkCreateDevice()</b>。
    </td>
  </tr>
  <tr>
    <td>设备调用链</td>
    <td>设备函数遵循的函数调用链。
        设备函数的此调用链通常如下：首先
        应用程序调用加载器跳板，然后加载器跳板
        调用启用的层，最后一层调用特定于设备的驱动程序
        。<br/>
        有关更多信息，请参阅
        <a href="#调度表和调用链">调度表和调用
        链</a>部分。
    </td>
  </tr>
  <tr>
    <td>设备函数</td>
    <td>设备函数是任何将 <i>VkDevice</i>、
        <i>VkQueue</i>、<i>VkCommandBuffer</i> 或这些的任何子对象作为其
        第一个参数的 Vulkan 函数。<br/><br/>
        一些 Vulkan 设备函数是：<br/>
        &nbsp;&nbsp;<b>vkQueueSubmit</b>，<br/>
        &nbsp;&nbsp;<b>vkBeginCommandBuffer</b>，<br/>
        &nbsp;&nbsp;<b>vkCreateEvent</b>。<br/><br/>
        有关更多信息，请参阅 <a href="#实例与设备">实例与设备</a>
        部分。
    </td>
  </tr>
  <tr>
    <td>发现</td>
    <td>加载器搜索驱动程序和层文件以设置
        可用 Vulkan 对象的内部列表的过程。<br/>
        在 <i>Windows/Linux/macOS</i> 上，发现过程通常专注于
        搜索清单文件。<br/>
        在 <i>Android</i> 上，该过程专注于搜索库文件。
    </td>
  </tr>
  <tr>
    <td>调度表</td>
    <td>函数指针数组（包括核心和可能的扩展
        函数），用于在调用链中步进到下一个实体。
        实体可以是加载器、层或驱动程序。<br/>
        有关更多信息，请参阅 <a href="#调度表和调用链">调度表和调用
        链</a>。
    </td>
  </tr>
  <tr>
    <td>驱动程序</td>
    <td>为 Vulkan API 提供支持的底层库。
        此支持可以实现为 ICD、API 转换
        库或纯软件。<br/>
        有关更多信息，请参阅 <a href="#驱动程序">驱动程序</a> 部分。
    </td>
  </tr>
  <tr>
    <td>扩展</td>
    <td>用于扩展核心 Vulkan 功能的 Vulkan 概念。
        扩展可能是 IHV 特定的、平台特定的，或更广泛地
        可用。<br/>
        始终首先查询扩展是否存在，并在
        <b>vkCreateInstance</b>（如果是实例扩展）或
        <b>vkCreateDevice</b>（如果是设备扩展）期间启用它，然后再尝试
        使用它。<br/>
        扩展总是对每个
        结构、枚举条目、命令入口点或与其关联的定义都有一个作者前缀或后缀修饰符。
        例如，`KHR` 是 Khronos 编写的扩展的前缀，并且
        也会在结构、枚举条目和命令上找到
        与这些扩展关联。
  </tr>
  <tr>
    <td>Extension Function</td>
    <td>作为扩展的一部分定义且不属于
        Vulkan 核心规范的函数。<br/>
        与函数作为其一部分定义的扩展一样，它将有一个
        后缀修饰符，指示扩展的作者。<br/>
        一些扩展后缀示例包括：<br/>
        &nbsp;&nbsp;<b>KHR</b>  - 用于 Khronos 编写的扩展，<br/>
        &nbsp;&nbsp;<b>EXT</b>  - 用于多公司编写的扩展，<br/>
        &nbsp;&nbsp;<b>AMD</b>  - 用于 AMD 编写的扩展，<br/>
        &nbsp;&nbsp;<b>ARM</b>  - 用于 ARM 编写的扩展，<br/>
        &nbsp;&nbsp;<b>NV</b>   - 用于 Nvidia 编写的扩展。<br/>
    </td>
  </tr>
  <tr>
    <td>ICD</td>
    <td>"Installable Client Driver"（可安装客户端驱动程序）的缩写。
        这些是由 IHV 提供的驱动程序，用于与它们提供的
        硬件交互。<br/>
        这些是最常见的 Vulkan 驱动程序类型。<br/>
        有关更多信息，请参阅 <a href="#可安装客户端驱动程序">可安装客户端驱动程序</a>
        部分。
    </td>
  </tr>
  <tr>
    <td>IHV</td>
    <td>"Independent Hardware Vendor"（独立硬件供应商）的缩写。
        通常是构建正在使用的底层硬件技术的
        公司。<br/>
        图形 IHV 的典型示例包括（但不限于）：
        AMD、ARM、Imagination、Intel、Nvidia、Qualcomm
    </td>
  </tr>
  <tr>
    <td>Instance Call Chain</td>
    <td>实例函数遵循的函数调用链。
        实例函数的调用链通常如下：首先
        应用程序调用加载器跳板函数，然后加载器
        跳板函数调用启用的层，最后一层调用加载器
        终止函数，加载器终止函数调用所有可用的
        驱动程序。<br/>
        有关更多信息，请参阅 <a href="#调度表和调用链">调度表和
        调用链</a> 部分。
    </td>
  </tr>
  <tr>
    <td>Instance Function</td>
    <td>实例函数是任何第一个
        参数是 <i>VkInstance</i> 或 <i>VkPhysicalDevice</i> 或
        根本没有对象的 Vulkan 函数。<br/><br/>
        一些 Vulkan 实例函数是：<br/>
        &nbsp;&nbsp;<b>vkEnumerateInstanceExtensionProperties</b>，<br/>
        &nbsp;&nbsp;<b>vkEnumeratePhysicalDevices</b>，<br/>
        &nbsp;&nbsp;<b>vkCreateInstance</b>，<br/>
        &nbsp;&nbsp;<b>vkDestroyInstance</b>。<br/><br/>
        有关更多信息，请参阅 <a href="#实例与设备">实例与设备</a>
        部分。
    </td>
  </tr>
  <tr>
    <td>Layer</td>
    <td>层是增强 Vulkan 系统的可选组件。
        它们可以拦截、评估和修改现有的 Vulkan 函数，在
        从应用程序到驱动程序的路径上。<br/>
        有关更多信息，请参阅 <a href="#层">层</a> 部分。
    </td>
  </tr>
  <tr>
    <td>Layer Library</td>
    <td><b>层库</b> 是加载器能够
        发现的所有层的组。
        这些可能包括隐式层和显式层。
        这些层可供应用程序使用，除非以某种方式禁用。
        有关更多信息，请参阅
        <a href="LoaderLayerInterface.md#层发现">层发现
        </a>。
    </td>
  </tr>
  <tr>
    <td>Loader</td>
    <td>作为 Vulkan
        应用程序、Vulkan 层和 Vulkan 驱动程序之间中介的中间件程序。<br/>
        有关更多信息，请参阅 <a href="#加载器">加载器</a> 部分。
    </td>
  </tr>
  <tr>
    <td>Manifest Files</td>
    <td>Khronos 加载器使用的 JSON 格式数据文件。
        这些文件包含
        <a href="LoaderLayerInterface.md#层清单文件格式">层</a>
        或
        <a href="LoaderDriverInterface.md#驱动程序清单文件格式">驱动程序</a>
        的特定信息，并定义必要的信息，例如在哪里查找文件和默认
        设置。
    </td>
  </tr>
  <tr>
    <td>Terminator Function</td>
    <td>实例调用链中驱动程序上方且由
        加载器拥有的最后一个函数。
        实例调用链中需要此函数，因为所有
        实例功能必须传达给所有能够
        接收调用的驱动程序。<br/>
        有关更多信息，请参阅 <a href="#调度表和调用链">调度表和调用
        链</a>。
    </td>
  </tr>
  <tr>
    <td>Trampoline Function</td>
    <td>实例或设备调用链中由加载器拥有的
        第一个函数，它使用
        适当的调度表处理设置和正确的调用链遍历。
        在设备函数（在设备调用链中）上，此函数实际上可以
        跳过。<br/>
        有关更多信息，请参阅 <a href="#调度表和调用链">调度表和调用
        链</a>。
    </td>
  </tr>
  <tr>
    <td>WSI Extension</td>
    <td>窗口系统集成（Windowing System Integration）的缩写。
        针对特定窗口系统的 Vulkan 扩展，旨在
        在窗口系统和 Vulkan 之间提供接口。<br/>
        有关更多信息，请参阅
        <a href="LoaderApplicationInterface.md#wsi-扩展">WSI 扩展</a>。
    </td>
  </tr>
  <tr>
    <td>Exported Function</td>
    <td>旨在通过平台特定的
        动态链接器获得的函数，特别是从驱动程序或层库中。
        需要导出的函数主要是加载器在层或驱动程序库上调用的
        第一个函数。<br/>
    </td>
  </tr>
  <tr>
    <td>Exposed Function</td>
    <td>旨在通过查询函数获得的函数，例如
        `vkGetInstanceProcAddr`。
        特定公开函数所需的精确查询函数在
        层和驱动程序之间以及接口版本之间有所不同。<br/>
    </td>
  </tr>
  <tr>
    <td>Querying Functions</td>
    <td>这些是允许加载器从
        驱动程序和层查询其他函数的函数。这些函数可能在 Vulkan API 中，但也可能
        来自私有加载器和驱动程序接口或加载器和层接口。<br/>
        这些函数是：
        `vkGetInstanceProcAddr`、`vkGetDeviceProcAddr`、
        `vk_icdGetInstanceProcAddr`、`vk_icdGetPhysicalDeviceProcAddr` 和
        `vk_layerGetPhysicalDeviceProcAddr`。
    </td>
  </tr>
</table>