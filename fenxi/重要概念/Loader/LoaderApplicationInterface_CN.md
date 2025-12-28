<!-- markdownlint-disable MD041 -->
[![Khronos Vulkan][1]][2]

[1]: images/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

# 应用程序与加载器的接口 <!-- omit from toc -->
[![Creative Commons][3]][4]

<!-- Copyright &copy; 2015-2023 LunarG, Inc. -->

[3]: images/Creative_Commons_88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/

## 目录 <!-- omit from toc -->

- [概述](#概述)
- [与 Vulkan 函数的接口](#与-vulkan-函数的接口)
  - [Vulkan 直接导出](#vulkan-直接导出)
  - [直接链接到加载器](#直接链接到加载器)
    - [动态链接](#动态链接)
    - [静态链接](#静态链接)
  - [间接链接到加载器](#间接链接到加载器)
  - [最佳应用程序性能设置](#最佳应用程序性能设置)
  - [ABI 版本控制](#abi-版本控制)
    - [Windows 动态库使用](#windows-动态库使用)
    - [Linux 动态库使用](#linux-动态库使用)
    - [macOS 动态库使用](#macos-动态库使用)
  - [将加载器与应用程序打包](#将加载器与应用程序打包)
- [应用程序层使用](#应用程序层使用)
  - [元层](#元层)
  - [隐式层与显式层](#隐式层与显式层)
    - [覆盖层](#覆盖层)
  - [强制层源文件夹](#强制层源文件夹)
    - [提升权限的例外情况](#提升权限的例外情况)
  - [在 Windows、Linux 和 macOS 上强制启用层](#在-windowslinux-和-macos-上强制启用层)
  - [整体层排序](#整体层排序)
  - [调试可能的层问题](#调试可能的层问题)
- [应用程序扩展使用](#应用程序扩展使用)
  - [实例扩展与设备扩展](#实例扩展与设备扩展)
  - [WSI 扩展](#wsi-扩展)
  - [未知扩展](#未知扩展)
  - [过滤未知实例扩展名称](#过滤未知实例扩展名称)
- [物理设备排序](#物理设备排序)

## 概述

本文档从应用程序的角度介绍如何与 Vulkan 加载器协作。
有关加载器所有部分的完整概述，请参阅 [LoaderInterfaceArchitecture.md](LoaderInterfaceArchitecture.md) 文件。

## 与 Vulkan 函数的接口

应用程序可以通过加载器以多种方式与 Vulkan 函数建立接口：

### Vulkan 直接导出

在 Windows、Linux、Android 和 macOS 上，加载器库会导出所有核心 Vulkan 入口点以及所有适当的窗口系统接口（WSI）入口点。
这样做是为了简化 Vulkan 开发的入门过程。
当应用程序以这种方式直接链接到加载器库时，Vulkan 调用是简单的*跳板*函数，它们会跳转到给定对象对应的调度表条目。

### 直接链接到加载器

#### 动态链接

加载器作为动态库分发（Windows 上为 .dll，Linux 上为 .so，macOS 上为 .dylib），安装到系统的动态库路径中。
此外，动态库通常在 Windows 系统上作为驱动程序安装的一部分进行安装，在 Linux 上通常通过系统包管理器提供。
这意味着应用程序通常可以期望系统上存在加载器的副本。
如果应用程序想要完全确保加载器存在，可以在应用程序中包含加载器或运行时安装程序。

#### 静态链接

在早期版本的加载器中，可以静态链接加载器。
**此功能已被移除，不再支持。**
移除静态链接的决定是因为驱动程序的更改使得静态链接的旧应用程序无法找到新的驱动程序。

此外，静态链接还存在几个问题：
 - 如果不重新链接应用程序，加载器永远无法更新
 - 两个包含的库可能包含不同版本的加载器
   - 可能导致不同加载器版本之间的冲突

唯一的例外是 macOS，但不支持或测试。

### 间接链接到加载器

应用程序不需要直接链接到加载器库，而是可以使用适当的平台特定动态符号查找来初始化应用程序自己的调度表。
这允许应用程序在找不到加载器时优雅地失败。
它还提供了应用程序调用 Vulkan 函数的最快机制。
应用程序只需要查询（通过系统调用如 `dlsym`）加载器库中 `vkGetInstanceProcAddr` 的地址。
然后应用程序使用 `vkGetInstanceProcAddr` 以平台无关的方式加载所有可用函数，例如 `vkCreateInstance`、`vkEnumerateInstanceExtensionProperties` 和 `vkEnumerateInstanceLayerProperties`。

### 最佳应用程序性能设置

为了在 Vulkan 应用程序中获得最佳性能，应用程序应该为每个 Vulkan API 入口点设置自己的调度表。
对于调度表中的每个实例级 Vulkan 命令，函数指针应该通过使用 `vkGetInstanceProcAddr` 的结果进行查询和填充。
此外，对于每个设备级 Vulkan 命令，函数指针应该通过使用 `vkGetDeviceProcAddr` 的结果进行查询和填充。

*为什么要这样做？*

答案在于实例函数的调用链实现与设备函数的调用链实现之间的差异。
请记住，[Vulkan 实例是用于提供 Vulkan 系统级信息的高级构造](LoaderInterfaceArchitecture.md#instance-specific)。
因此，实例函数需要广播到系统上的每个可用驱动程序。
下图显示了启用三个层时实例调用链的近似视图：

![实例调用链](images/loader_instance_chain.png)

如果使用 `vkGetInstanceProcAddr` 查询，这也是 Vulkan 设备函数调用链的样子。
另一方面，设备函数不需要担心广播，因为它知道调用应该终止于哪个关联的驱动程序和哪个关联的物理设备。
因此，加载器不需要在启用的层和驱动程序之间介入。
因此，使用加载器导出的 Vulkan 设备函数，在相同场景下的调用链将如下所示：

![加载器设备调用链](images/loader_device_chain_loader.png)

更好的解决方案是应用程序对所有设备函数执行 `vkGetDeviceProcAddr` 调用。
这通过在大多数情况下完全移除加载器来进一步优化调用链：

![应用程序设备调用链](images/loader_device_chain_app.png)

另外，请注意，如果没有启用层，应用程序函数指针**直接指向驱动程序**。
对于许多函数调用，每个调用中缺少间接寻址会累积成可观的性能节省。

**注意：** 有一些设备函数仍然需要加载器使用*跳板*和*终止器*进行拦截。
这些函数很少，但它们通常是加载器用自己的数据包装的函数。
在这些情况下，即使设备调用链也会继续看起来像实例调用链。
需要*终止器*的设备函数的一个例子是 `vkCreateSwapchainKHR`。
对于该函数，加载器需要将 KHR_surface 对象转换为驱动程序特定的 KHR_surface 对象，然后再将函数的其余信息传递给驱动程序。

请记住：
 * `vkGetInstanceProcAddr` 用于查询实例和物理设备函数，但可以查询所有函数。
 * `vkGetDeviceProcAddr` 仅用于查询设备函数。

### ABI 版本控制

Vulkan 加载器库将通过多种方式分发，包括 Vulkan SDK、操作系统包分发和独立硬件供应商（IHV）驱动程序包。
这些细节超出了本文档的范围。
但是，Vulkan 加载器库的名称和版本控制是规定的，以便应用程序可以链接到正确的 Vulkan ABI 库版本。
对于具有相同主版本号的所有版本（例如 1.0 和 1.1），保证 ABI 向后兼容性。

#### Windows 动态库使用

在 Windows 上，加载器库在其名称中编码 ABI 版本，以便多个 ABI 不兼容的加载器版本可以在给定系统上和平共存。
Vulkan 加载器库文件名是 `vulkan-<ABI 版本>.dll`。
例如，对于 Windows 上的 Vulkan 版本 1.X，库文件名是 `vulkan-1.dll`。
该库文件通常可以在 `windows\system32` 目录中找到（在 64 位 Windows 安装中，具有相同名称的 32 位版本的加载器可以在 `windows\sysWOW64` 目录中找到）。

#### Linux 动态库使用

对于 Linux，共享库基于后缀进行版本控制。
因此，ABI 编号不像 Windows 那样编码在库文件名的基名中。

在 Linux 上，对 Vulkan 有硬依赖的应用程序应该在其构建系统中请求链接到未版本化的名称 `libvulkan.so`。
例如，通过导入 CMake 目标 `Vulkan::Vulkan` 或使用 `pkg-config --cflags --libs vulkan` 的输出作为编译器标志。
与 Linux 库一样，编译器和链接器会将其解析为对正确版本化的 SONAME 的依赖，目前是 `libvulkan.so.1`。
在运行时动态加载 Vulkan-Loader 的 Linux 应用程序不会从此机制中受益，而应该确保将版本化的名称（如 `libvulkan.so.1`）传递给 `dlopen()`，以确保它们加载兼容的版本。

#### macOS 动态库使用

macOS 链接类似于 Linux，不同之处在于标准动态库名为 `libvulkan.dylib`，ABI 版本化的库目前名为 `libvulkan.1.dylib`。

### 将加载器与应用程序打包

Khronos 加载器通常以平台特定的方式（例如 Linux 上的包）或作为驱动程序安装的一部分（例如在 Windows 上使用 Vulkan 运行时安装程序）安装在平台上。
应用程序或引擎可能希望将 Vulkan 加载器本地安装到其执行树中，作为其自己的安装过程的一部分。
这可能是因为提供特定的加载器：

 1) 保证加载器中存在某些 Vulkan API 导出
 2) 确保某些加载器行为是已知的
 3) 在用户安装之间提供一致性

但是，**强烈不鼓励**这样做，因为：

 1) 打包的加载器可能与未来的驱动程序修订版不兼容（这在 Windows 上尤其如此，在操作系统更新期间驱动程序安装位置可能会更改）
 2) 它可能阻止应用程序/引擎利用新的 Vulkan API 版本/扩展导出
 3) 应用程序/引擎将错过重要的加载器错误修复
 4) 打包的加载器将不包含有用的功能更新（如改进的加载器可调试性）

当然，即使应用程序/引擎最初使用特定版本的 Khronos 加载器发布，它也可能选择在未来某个时候更新或删除该加载器。
这可能是由于加载器中所需功能的暴露随着时间的推移而进展。
但是，这依赖于最终用户在未来正确执行任何必要的更新过程，这可能导致不同用户系统之间的行为不同。

一个更好的替代方案，至少在 Windows 上，是将所需版本的 Vulkan 加载器的 Vulkan 运行时安装程序与您的产品打包。
然后，安装过程可以使用它来确保最终用户的系统是最新的。
运行时安装程序将检测已安装的版本，仅在必要时安装更新的运行时。

另一个替代方案是编写应用程序，使其可以回退到早期版本的 Vulkan，但显示警告，指示功能已禁用，直到用户将其系统更新到特定的运行时/驱动程序。

## 应用程序层使用

需要超出系统上 Vulkan 驱动程序已暴露功能的应用程序可以使用各种层来增强 API。
层不能添加 Vulkan.h 中未暴露的新 Vulkan 核心 API 入口点。
但是，层可以提供扩展的实现，这些扩展引入了在没有这些层的情况下不可用的额外入口点。
这些额外的扩展入口点可以通过 Vulkan 扩展接口进行查询。

层的常见用途是 API 验证，可以在应用程序开发期间启用，在发布应用程序时省略。
这允许轻松控制启用验证应用程序 API 使用所产生的开销，这在以前的图形 API 中并不总是可能的。

要查找应用程序可用的层，请使用 `vkEnumerateInstanceLayerProperties`。
这将报告加载器发现的所有层。
加载器在系统上的各种位置查找层。
有关更多信息，请参阅 [LoaderLayerInterface.md 文档](LoaderLayerInterface.md) 中的[层发现](LoaderLayerInterface.md#layer-discovery)部分。

要启用特定层，只需在调用 `vkCreateInstance` 期间将要启用的层名称传递给 `VkInstanceCreateInfo` 的 `ppEnabledLayerNames` 字段。
完成后，已启用的层将对使用创建的 `VkInstance` 及其任何子对象的所有 Vulkan 函数处于活动状态。

**注意：** 层的排序在几种情况下很重要，因为某些层会相互交互。
启用层时要小心，因为这可能是情况。
有关更多信息，请参阅[整体层排序](#整体层排序)部分。

以下代码段显示了如何启用 `VK_LAYER_KHRONOS_validation` 层。

```
char *instance_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
const VkApplicationInfo app = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = "TEST_APP",
    .applicationVersion = 0,
    .pEngineName = "TEST_ENGINE",
    .engineVersion = 0,
    .apiVersion = VK_API_VERSION_1_0,
};
VkInstanceCreateInfo inst_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = NULL,
    .pApplicationInfo = &app,
    .enabledLayerCount = 1,
    .ppEnabledLayerNames = (const char *const *)instance_layers,
    .enabledExtensionCount = 0,
    .ppEnabledExtensionNames = NULL,
};
err = vkCreateInstance(&inst_info, NULL, &demo->inst);
if (VK_ERROR_LAYER_NOT_PRESENT == err) {
  // 找不到验证层
}
```

在 `vkCreateInstance` 和 `vkCreateDevice` 时，加载器构建包含应用程序指定的（启用的）层的调用链。
`ppEnabledLayerNames` 数组中的顺序很重要；数组元素 0 是插入链中最顶层（最接近应用程序）的层，最后一个数组元素最接近驱动程序。
有关层排序的更多信息，请参阅[整体层排序](#整体层排序)部分。

**注意：** *设备层现已弃用*
> `vkCreateDevice` 最初能够以类似于 `vkCreateInstance` 的方式选择层。
> 这导致了"实例层"和"设备层"的概念。
> Khronos 决定弃用"设备层"功能，只考虑"实例层"。
> 因此，`vkCreateDevice` 将使用在 `vkCreateInstance` 时指定的层。
> 因此，以下项目已被弃用：
> * `VkDeviceCreateInfo` 字段：
>   * `ppEnabledLayerNames`
>   * `enabledLayerCount`
> * `vkEnumerateDeviceLayerProperties` 函数

### 元层

元层是包含要启用的其他层的有序列表的层。
这是为了允许以指定顺序将层组合在一起，以便它们能够正确交互。
最初，这用于以正确的顺序将各个 Vulkan 验证层组合在一起以避免冲突。
这是必要的，因为验证不是单个验证层，而是拆分为多个组件层。
新的 `VK_LAYER_KHRONOS_validation` 层将所有内容整合到单个层中，不再需要元层。
虽然验证不再需要元层，但 VkConfig 确实使用元层根据用户的偏好将层组合在一起。
有关此功能的更多信息，请参阅 [VkConfig 文档](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md) 以及后面关于[覆盖层](#覆盖层)的部分。

元层在本文件夹中的 [LoaderLayerInterface.md](LoaderLayerInterface.md) 文件的[元层](LoaderLayerInterface.md#meta-layers)部分中有更详细的说明。

### 隐式层与显式层

![不同类型的层](images/loader_layer_order.png)

显式层是由应用程序启用的层（例如，使用前面提到的 vkCreateInstance 函数）。

隐式层通过其存在自动启用，除非需要额外的手动启用步骤，这与必须显式启用的显式层不同。
例如，某些应用程序环境（例如 Steam 或汽车信息娱乐系统）可能具有它们希望为它们启动的所有应用程序始终启用的层。
其他隐式层可能适用于在给定系统上启动的所有应用程序（例如，覆盖每秒帧数的层）。

隐式层相对于显式层有一个额外要求，即它们必须能够通过环境变量禁用。
这是因为它们对应用程序不可见，可能会造成问题。
要记住的一个好原则是定义启用和禁用环境变量，以便用户可以确定性地启用功能。
在桌面平台（Windows、Linux 和 macOS）上，这些启用/禁用设置在层的 JSON 文件中定义。

系统安装的隐式和显式层的发现在 [LoaderLayerInterface.md 文档](LoaderLayerInterface.md) 中的[层发现](LoaderLayerInterface.md#layer-discovery)部分中有详细说明。

隐式和显式层可能根据底层操作系统在不同的位置找到。
下表提供了更多信息：

<table style="width:100%">
  <tr>
    <th>操作系统</th>
    <th>隐式层标识</th>
  </tr>
  <tr>
    <td>Windows</td>
    <td>隐式层位于与显式层不同的 Windows 注册表位置。</td>
  </tr>
  <tr>
    <td>Linux</td>
    <td>隐式层位于与显式层不同的目录位置。</td>
  </tr>
  <tr>
    <td>Android</td>
    <td>Android 上**不支持隐式层**。</td>
  </tr>
  <tr>
    <td>macOS</td>
    <td>隐式层位于与显式层不同的目录位置。</td>
  </tr>
</table>

#### 覆盖层

"覆盖层"是由 [VkConfig](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md) 工具创建的特殊隐式元层，在工具运行时默认可用。
一旦 VkConfig 退出，覆盖层将被移除，系统应该返回标准 Vulkan 行为。
每当覆盖层存在于层搜索路径中时，加载器会将其与标准隐式层一起拉入层调用堆栈，以及要加载的层列表中包含的所有层。
这允许最终用户或开发人员通过 VkConfig 轻松强制启用任意数量的层和设置。

覆盖层在本文件夹中的 [LoaderLayerInterface.md](LoaderLayerInterface.md) 文件的[覆盖元层](LoaderLayerInterface.md#override-meta-layer)部分中有更详细的讨论。

### 强制层源文件夹

开发人员可能需要使用特殊的、预生产的层，而不修改系统安装的层。

这可以通过以下两种方式之一来完成：

 1. 使用随 Vulkan SDK 一起提供的 [VkConfig](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md) 工具选择特定的层路径。
 2. 通过使用 `VK_LAYER_PATH` 和/或 `VK_IMPLICIT_LAYER_PATH` 环境变量，指示加载器在特定文件和/或文件夹中查找层。

`VK_LAYER_PATH` 和 `VK_IMPLICIT_LAYER_PATH` 环境变量可以包含多个路径，由操作系统特定的路径分隔符分隔。
在 Windows 上，这是分号（`;`），而在 Linux 和 macOS 上，这是冒号（`:`）。

如果存在 `VK_LAYER_PATH`，列出的文件和/或文件夹将被扫描以查找显式层清单文件。
隐式层发现不受此环境变量的影响。

如果存在 `VK_IMPLICIT_LAYER_PATH`，列出的文件和/或文件夹将被扫描以查找隐式层清单文件。
显式层发现不受此环境变量的影响。

`VK_LAYER_PATH` 和 `VK_IMPLICIT_LAYER_PATH` 中列出的每个目录应该是包含层清单文件的文件夹的完整路径名。

有关更多详细信息，请参阅 [LoaderInterfaceArchitecture.md 文档](LoaderInterfaceArchitecture.md) 中的[调试环境变量表](LoaderInterfaceArchitecture.md#table-of-debug-environment-variables)。

#### 提升权限的例外情况

出于安全原因，如果以提升权限运行，`VK_LAYER_PATH` 和 `VK_IMPLICIT_LAYER_PATH` 将被忽略。
因此，这些环境变量只能用于不使用提升权限的应用程序。

有关更多信息，请参阅顶级 [LoaderInterfaceArchitecture.md][LoaderInterfaceArchitecture.md] 文档中的[提升权限注意事项](LoaderInterfaceArchitecture.md#elevated-privilege-caveats)。

### 在 Windows、Linux 和 macOS 上强制启用层

开发人员可能希望启用给定应用程序未启用的层。

这也可以通过以下两种方式之一来完成：

 1. 使用随 Vulkan SDK 一起提供的 [VkConfig](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md) 工具选择特定层。
 2. 通过使用 `VK_INSTANCE_LAYERS` 环境变量，指示加载器按名称查找其他层。

两者都可以用于启用应用程序在 `vkCreateInstance` 时未指定（启用）的其他层。

`VK_INSTANCE_LAYERS` 环境变量是要启用的层名称列表，由操作系统特定的路径分隔符分隔。
在 Windows 上，这是分号（`;`），而在 Linux 和 macOS 上，这是冒号（`:`）。
名称的顺序是相关的，列表中的第一个层名称是最顶层（最接近应用程序），列表中的最后一个层名称是最底层（最接近驱动程序）。
有关更多信息，请参阅[整体层排序](#整体层排序)部分。

应用程序指定的层和用户指定的层（通过环境变量）在启用层时由加载器聚合并删除重复项。
通过环境变量指定的层是最顶层（最接近应用程序），而由应用程序指定的层是最底层。

在 Linux 或 macOS 上使用这些环境变量激活验证层 `VK_LAYER_KHRONOS_validation` 的示例如下：

```
> $ export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
```

有关更多详细信息，请参阅 [LoaderInterfaceArchitecture.md 文档](LoaderInterfaceArchitecture.md) 中的[调试环境变量表](LoaderInterfaceArchitecture.md#table-of-debug-environment-variables)。

### 整体层排序

基于上述内容，加载器对所有层的整体排序如下所示：

![加载器层排序](images/loader_layer_order_calls.png)

排序在显式层列表内部也可能很重要。
某些层可能依赖于在加载器调用它之前或之后实现的其他行为。
例如：覆盖层可能希望使用 `VK_LAYER_KHRONOS_validation` 来验证覆盖层的行为是否适当。
这需要将覆盖层更靠近应用程序，以便验证层可以拦截覆盖层需要进行的任何 Vulkan API 调用以发挥作用。

### 调试可能的层问题

如果层可能导致问题，可以尝试几种方法，这些方法在 docs 文件夹中的 [LoaderDebugging.md 文档](LoaderDebugging.md) 的[调试可能的层问题](LoaderDebugging.md#debugging-possible-layer-issues)部分中有详细说明。

## 应用程序扩展使用

扩展是由层、加载器或驱动程序提供的可选功能。
扩展可以修改 Vulkan API 的行为，需要指定并在 Khronos 注册。
这些扩展可以由 Vulkan 驱动程序、加载器或层实现，以暴露核心 API 中不可用的功能。
有关各种扩展的信息可以在 Vulkan 规范和 vulkan.h 头文件中找到。

### 实例扩展与设备扩展

如主 [LoaderInterfaceArchitecture.md](LoaderInterfaceArchitecture.md) 文档的[实例与设备](LoaderInterfaceArchitecture.md#instance-versus-device)部分所暗示的，有两种类型的扩展：
 * 实例扩展
 * 设备扩展

实例扩展修改实例级对象（如 `VkInstance` 和 `VkPhysicalDevice`）上的现有行为或实现新行为。
设备扩展对设备级对象（如 `VkDevice`、`VkQueue` 和 `VkCommandBuffer` 以及这些对象的任何子对象）执行相同的操作。

了解扩展的类型**非常重要**，因为实例扩展必须在 `vkCreateInstance` 时启用，而设备扩展必须在 `vkCreateDevice` 时启用。

当调用 `vkEnumerateInstanceExtensionProperties` 和 `vkEnumerateDeviceExtensionProperties` 时，加载器发现并聚合来自层（显式和隐式）、驱动程序和加载器的各自类型的所有扩展，然后向应用程序报告它们。

查看 `vulkan.h`，两个函数非常相似，例如，`vkEnumerateInstanceExtensionProperties` 原型如下所示：

```
VkResult
   vkEnumerateInstanceExtensionProperties(
      const char *pLayerName,
      uint32_t *pPropertyCount,
      VkExtensionProperties *pProperties);
```

而 `vkEnumerateDeviceExtensionProperties` 原型如下所示：

```
VkResult
   vkEnumerateDeviceExtensionProperties(
      VkPhysicalDevice physicalDevice,
      const char *pLayerName,
      uint32_t *pPropertyCount,
      VkExtensionProperties *pProperties);
```

这些函数中的 "pLayerName" 参数用于选择单个层或 Vulkan 平台实现。
如果 "pLayerName" 为 NULL，则枚举来自 Vulkan 实现组件（包括加载器、隐式层和驱动程序）的扩展。
如果 "pLayerName" 等于发现的层模块名称，则仅枚举来自该层（可能是隐式或显式）的扩展。

**注意：** 虽然设备层已弃用，但实例启用的层仍然存在于设备调用链中。

重复的扩展（例如，隐式层和驱动程序可能报告支持相同的扩展）由加载器消除。
对于重复项，报告驱动程序版本，并剔除层版本。

此外，扩展**必须启用**（在 `vkCreateInstance` 或 `vkCreateDevice` 中），然后才能使用与扩展关联的函数。
如果使用 `vkGetInstanceProcAddr` 或 `vkGetDeviceProcAddr` 查询扩展函数，但未启用扩展，则可能导致未定义的行为。
验证层将捕获此无效的 API 使用。

### WSI 扩展

Khronos 批准的 WSI 扩展可用，并为各种执行环境提供窗口系统集成支持。
重要的是要理解，某些 WSI 扩展对所有目标都有效，但其他扩展特定于给定的执行环境（和加载器）。
此 Khronos 加载器（目前针对 Windows、Linux、macOS、Stadia 和 Fuchsia）仅启用并直接导出适合当前环境的 WSI 扩展。
在很大程度上，选择是在加载器中使用编译时预处理器标志完成的。
所有版本的 Khronos 加载器目前至少暴露以下 WSI 扩展支持：
- VK_KHR_surface
- VK_KHR_swapchain
- VK_KHR_display

此外，加载器的以下每个操作系统目标都支持特定于目标的扩展：

| 窗口系统 | 可用扩展                       |
| ---------------- | ------------------------------------------ |
| Windows          | VK_KHR_win32_surface                       |
| Linux (Wayland)  | VK_KHR_wayland_surface                     |
| Linux (X11)      | VK_KHR_xcb_surface 和 VK_KHR_xlib_surface |
| macOS (MoltenVK) | VK_MVK_macos_surface                       |
| QNX (Screen)     | VK_QNX_screen_surface                      |

重要的是要理解，虽然加载器可能支持这些扩展的各种入口点，但要实际使用它们需要握手：
* 至少一个物理设备必须支持扩展
* 应用程序在创建逻辑设备时必须使用这样的物理设备
* 应用程序必须在创建实例或逻辑设备时请求启用扩展（这取决于给定的扩展是与实例还是设备一起工作）

只有这样，WSI 扩展才能在 Vulkan 程序中正确使用。

### 未知扩展

由于能够如此轻松地扩展 Vulkan，将创建加载器不知道的扩展。
如果扩展是设备扩展，加载器会将未知入口点传递到设备调用链，以适当的驱动程序入口点结束。
如果扩展是实例扩展，其第一个组件是物理设备参数，也会发生同样的情况。
但是，对于所有其他实例扩展，加载器将无法加载它。

*但是为什么加载器不支持未知的实例扩展？*
<br/>
让我们再次查看实例调用链：

![实例调用链](images/loader_instance_chain.png)

请注意，对于正常的实例函数调用，加载器必须处理将函数调用传递给可用的驱动程序。
如果加载器不知道实例调用的参数或返回值，它无法正确地将信息传递给驱动程序。
可能有办法做到这一点，这将在未来进行探索。
但是，目前，加载器不支持不暴露以物理设备作为第一个参数的入口点的实例扩展。

因为设备调用链通常不通过加载器*终止器*，所以这对设备扩展不是问题。
此外，由于物理设备与一个驱动程序关联，加载器可以使用指向一个驱动程序的通用*终止器*。
这是因为这两个扩展都直接终止于它们关联的驱动程序中。

*这是一个大问题吗？*
<br/>
不！
大多数扩展功能只影响物理或逻辑设备，而不影响实例。
因此，绝大多数扩展应该通过直接加载器支持得到支持。

### 过滤未知实例扩展名称

在某些情况下，驱动程序可能支持加载器不支持的实例扩展。
由于上述原因，当应用程序调用 `vkEnumerateInstanceExtensionProperties` 时，加载器将过滤掉这些未知实例扩展的名称。
此外，如果应用程序仍然尝试使用这些扩展之一，此行为将导致加载器在 `vkCreateInstance` 期间发出错误。
目的是保护应用程序，使其不会无意中使用可能导致崩溃的功能。

另一方面，如果必须强制启用扩展，可以通过将 `VK_LOADER_DISABLE_INST_EXT_FILTER` 环境变量定义为非零数字来禁用过滤。
这将有效地禁用加载器对实例扩展名称的过滤。

## 物理设备排序

在 1.3.204 加载器之前，Linux 上的物理设备可能以不一致的顺序返回。
为了解决这个问题，Vulkan 加载器现在将在从驱动程序接收设备后（在将信息返回给任何启用的层之前）按以下方式对设备进行排序：
 * 基于设备类型排序（独立显卡、集成显卡、虚拟设备、所有其他）
 * 在类型内部基于 PCI 信息排序（域、总线、设备和功能）。

这允许在同一系统上运行之间物理设备顺序保持一致，除非实际的底层硬件发生变化。

定义了一个新的环境变量，使用户能够强制选择特定设备 `VK_LOADER_DEVICE_SELECT`。
此环境变量应设置为所需设备的十六进制值，用于供应商 ID 和设备 ID（从 `VkPhysicalDeviceProperties` 结构中的 `vkGetPhysicalDeviceProperties` 返回）。
它应该如下所示：

```
set VK_LOADER_DEVICE_SELECT=0x10de:0x1f91
```

这将强制选择供应商 ID 为 "0x10de" 和设备 ID 为 "0x1f91" 的设备。
如果找不到该设备，则简单地忽略此设置。

可以通过将环境变量 `VK_LOADER_DISABLE_SELECT` 设置为非零值来禁用加载器中完成的所有设备选择工作。
这旨在用于调试目的，以缩小加载器设备选择机制的任何问题，但也可以由其他人使用。

[返回顶级 LoaderInterfaceArchitecture.md 文件。](LoaderInterfaceArchitecture.md)

