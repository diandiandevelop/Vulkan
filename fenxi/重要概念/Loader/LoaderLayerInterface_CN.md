<!-- markdownlint-disable MD041 -->
[![Khronos Vulkan][1]][2]

[1]: images/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

# 层与加载器的接口 <!-- omit from toc -->
[![Creative Commons][3]][4]

<!-- Copyright &copy; 2015-2023 LunarG, Inc. -->

[3]: images/Creative_Commons_88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/


## 目录 <!-- omit from toc -->

- [概述](#概述)
- [层发现](#层发现)
  - [层清单文件的使用](#层清单文件的使用)
  - [Android 层发现](#android-层发现)
  - [Windows 层发现](#windows-层发现)
  - [Linux 层发现](#linux-层发现)
    - [Linux 显式层搜索路径示例](#linux-显式层搜索路径示例)
  - [Fuchsia 层发现](#fuchsia-层发现)
  - [macOS 层发现](#macos-层发现)
    - [macOS 隐式层搜索路径示例](#macos-隐式层搜索路径示例)
  - [层过滤](#层过滤)
    - [层启用过滤](#层启用过滤)
    - [层禁用过滤](#层禁用过滤)
    - [层特殊情况禁用](#层特殊情况禁用)
    - [层禁用警告](#层禁用警告)
    - [允许某些层忽略层禁用](#允许某些层忽略层禁用)
      - [`VK_INSTANCE_LAYERS`](#vk_instance_layers)
  - [提升权限的例外情况](#提升权限的例外情况)
- [层版本协商](#层版本协商)
- [层调用链和分布式调度](#层调用链和分布式调度)
- [层未知物理设备扩展](#层未知物理设备扩展)
  - [添加 `vk_layerGetPhysicalDeviceProcAddr` 的原因](#添加-vk_layergetphysicaldeviceprocaddr-的原因)
- [层拦截要求](#层拦截要求)
- [分布式调度要求](#分布式调度要求)
- [层约定和规则](#层约定和规则)
- [层调度初始化](#层调度初始化)
- [CreateInstance 示例代码](#createinstance-示例代码)
- [CreateDevice 示例代码](#createdevice-示例代码)
- [元层](#元层)
  - [覆盖元层](#覆盖元层)
- [实例前函数](#实例前函数)
- [特殊注意事项](#特殊注意事项)
  - [在层内将私有数据与 Vulkan 对象关联](#在层内将私有数据与-vulkan-对象关联)
    - [包装](#包装)
    - [关于包装的注意事项](#关于包装的注意事项)
    - [哈希映射](#哈希映射)
  - [创建新的可调度对象](#创建新的可调度对象)
  - [版本控制和激活交互](#版本控制和激活交互)
- [层清单文件格式](#层清单文件格式)
  - [层清单文件版本历史](#层清单文件版本历史)
  - [层清单文件版本 1.2.1](#层清单文件版本-121)
    - [层清单文件版本 1.2.0](#层清单文件版本-120)
    - [层清单文件版本 1.1.2](#层清单文件版本-112)
    - [层清单文件版本 1.1.1](#层清单文件版本-111)
    - [层清单文件版本 1.1.0](#层清单文件版本-110)
    - [层清单文件版本 1.0.1](#层清单文件版本-101)
    - [层清单文件版本 1.0.0](#层清单文件版本-100)
- [层接口版本](#层接口版本)
  - [层接口版本 2](#层接口版本-2)
  - [层接口版本 1](#层接口版本-1)
  - [层接口版本 0](#层接口版本-0)
- [加载器与层接口策略](#加载器与层接口策略)
  - [编号格式](#编号格式)
  - [Android 差异](#android-差异)
  - [行为良好的层的要求](#行为良好的层的要求)
  - [行为良好的加载器的要求](#行为良好的加载器的要求)


## 概述

这是从层角度与 Vulkan 加载器协作的视图。
有关加载器所有部分的完整概述，请参阅
[LoaderInterfaceArchitecture.md](LoaderInterfaceArchitecture.md) 文件。


## 层发现

如
[LoaderApplicationInterface.md](LoaderApplicationInterface.md) 文档的
[隐式与显式](LoaderApplicationInterface.md#implicit-vs-explicit-layers)
部分所述，层可以分为两类：
 * 隐式层
 * 显式层

两者之间的主要区别是隐式层会自动
启用（除非被覆盖），而显式层必须被启用。
请记住，隐式层并非在所有操作系统上都存在（如
Android）。

在任何系统上，加载器都会在特定区域查找有关
可以在用户请求时加载的层的信息。
在系统上查找可用层的过程称为层
发现。
在发现过程中，加载器确定哪些层可用、层
名称、层版本以及层支持的任何扩展。
此信息通过
`vkEnumerateInstanceLayerProperties` 提供给应用程序。

加载器可用的层组称为 `层库`。
本节定义了一个可扩展的接口来发现
`层库` 中包含哪些层。

本节还指定了层必须遵循的
最小约定和规则，特别是关于与加载器和其他
层交互的规则。

搜索层时，加载器将按照检测到的顺序查看 `层库`，如果名称匹配，则加载该层。
如果同一库的多个实例存在于不同位置
在整个用户系统中，则将使用搜索顺序中首先出现的那个。
每个操作系统都有自己的搜索顺序，在其层发现
部分中定义。
如果同一目录中的多个清单文件定义同一层，但
指向不同的库文件，则加载层的顺序是
[由于 readdir 的行为而随机](https://www.ibm.com/support/pages/order-directory-contents-returned-calls-readdir)。

此外，在调用 `vkCreateInstance` 或
`vkCreateDevice` 期间，组件层列表或
所有启用层中的任何重复层名称都将被加载器简单地忽略。
只会使用任何层名称的第一次出现。


### 层清单文件的使用

在 Windows、Linux 和 macOS 系统上，使用 JSON 格式的清单
文件来存储层信息。
为了找到系统安装的层，Vulkan 加载器将读取 JSON
文件以识别层及其扩展的名称和属性。
使用清单文件允许加载器在应用程序不查询也不请求任何扩展时避免加载任何共享库
文件。
[层清单文件](#层清单文件格式) 的格式在
下面详细说明。

Android 加载器不使用清单文件。
相反，加载器使用称为
"内省"函数的特殊函数查询层属性。
这些函数的目的是确定与从读取清单文件中收集的相同必需信息
。
这些内省函数不被 Khronos 加载器使用，但应该
存在于层中以保持一致性。
具体的"内省"函数在
[层清单文件格式](#层清单文件格式) 表中列出。


### Android 层发现

在 Android 上，加载器在
`/data/local/debug/vulkan` 文件夹中查找要枚举的层。
启用调试的应用程序能够枚举和启用该
位置的任何层。


### Windows 层发现

为了找到系统安装的层，Vulkan 加载器将扫描
以下 Windows 注册表项中的值：

```
HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ExplicitLayers
HKEY_CURRENT_USER\SOFTWARE\Khronos\Vulkan\ExplicitLayers
HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers
HKEY_CURRENT_USER\SOFTWARE\Khronos\Vulkan\ImplicitLayers
```

除了在 64 位 Windows 上运行 32 位应用程序时，加载器
将改为扫描 32 位注册表位置：

```
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers
HKEY_CURRENT_USER\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\ImplicitLayers
HKEY_CURRENT_USER\SOFTWARE\WOW6432Node\Khronos\Vulkan\ImplicitLayers
```

对于这些键中 DWORD 数据设置为 0 的每个值，加载器打开
由值的名称指定的 JSON 清单文件。
每个名称必须是清单文件的绝对路径。
此外，只有在
应用程序未以管理员权限执行时，才会搜索 `HKEY_CURRENT_USER` 位置。
这样做是为了确保具有管理员权限的应用程序不会
运行不需要管理员访问权限即可安装的层。

因为某些层与驱动程序一起安装，加载器将扫描
特定于显示适配器和所有软件组件的注册表项
，这些组件与这些适配器关联，以查找 JSON 清单文件的位置。
这些键位于驱动程序安装期间创建设备键中，
包含基本设置的配置信息，包括 Vulkan、OpenGL
和 Direct3D ICD 位置。

设备适配器和软件组件键路径应通过
PnP 配置管理器 API 获得。
`000X` 键将是一个编号键，其中每个设备被分配
不同的编号。

```
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Adapter GUID}\000X\VulkanExplicitLayers
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Adapter GUID}\000X\VulkanImplicitLayers
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Software Component GUID}\000X\VulkanExplicitLayers
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Software Component GUID}\000X\VulkanImplicitLayers
```

此外，在 64 位系统上可能还有另一组注册表值，
如下所列。
这些值记录 64 位操作系统上 32 位层的位置，
与 Windows-on-Windows 功能相同。

```
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Adapter GUID}\000X\VulkanExplicitLayersWow
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Adapter GUID}\000X\VulkanImplicitLayersWow
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Software Component GUID}\000X\VulkanExplicitLayersWow
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Software Component GUID}\000X\VulkanImplicitLayersWow
```

如果上述任何值存在且类型为 `REG_SZ`，加载器将打开
由键值指定的 JSON 清单文件。
每个值必须是 JSON 清单文件的绝对路径。
键值也可以是 `REG_MULTI_SZ` 类型，在这种情况下，该值将被
解释为 JSON 清单文件路径的列表。

通常，应用程序应将层安装到
`SOFTWARE\Khronos\Vulkan` 路径。
PnP 注册表位置专门用于作为
驱动程序安装一部分分发的层。
应用程序安装程序不应修改设备特定的注册表，
而设备驱动程序不应修改系统注册表。

此外，Vulkan 加载器将扫描系统以查找已知的 Windows
AppX/MSIX 包。
如果找到包，加载器将扫描此已安装
包的根目录以查找 JSON 清单文件。目前，唯一已知的包是
Microsoft 的
[OpenCL™、OpenGL® 和 Vulkan® 兼容包](https://apps.microsoft.com/store/detail/9NQPSL29BFFF?hl=en-us&gl=US)。

Vulkan 加载器将打开每个清单文件以获取有关
层的信息，包括共享库（".dll"）文件的名称或路径名。

如果定义了 `VK_LAYER_PATH`，则加载器将查看该
变量定义的路径以查找显式层清单文件，而不是使用
显式层注册表项提供的信息。

如果定义了 `VK_ADD_LAYER_PATH`，则加载器将查看提供的
路径以查找显式层清单文件，同时使用
显式层注册表项提供的信息。
`VK_ADD_LAYER_PATH` 提供的路径在标准搜索文件夹列表之前添加
，因此将首先搜索。

如果存在 `VK_LAYER_PATH`，则加载器不会使用 `VK_ADD_LAYER_PATH`，任何值都将被
忽略。

如果定义了 `VK_IMPLICIT_LAYER_PATH`，则加载器将查看该
变量定义的路径以查找隐式层清单文件，而不是使用
隐式层注册表项提供的信息。

如果定义了 `VK_ADD_IMPLICIT_LAYER_PATH`，则加载器将查看提供的
路径以查找隐式层清单文件，同时使用
隐式层注册表项提供的信息。
`VK_ADD_IMPLICIT_LAYER_PATH` 提供的路径在标准搜索文件夹列表之前添加
，因此将首先搜索。

出于安全原因，如果以提升权限运行，`VK_LAYER_PATH`、`VK_ADD_LAYER_PATH`、`VK_IMPLICIT_LAYER_PATH`
和 `VK_ADD_IMPLICIT_LAYER_PATH` 将被忽略。
有关更多信息，请参阅[提升权限的例外情况](#提升权限的例外情况)
。

有关此内容的更多信息，请参阅
[LoaderApplicationInterface.md 文档](LoaderApplicationInterface.md)中的
[强制层源文件夹](LoaderApplicationInterface.md#forcing-layer-source-folders)
。


### Linux 层发现

在 Linux 上，Vulkan 加载器将使用环境
变量或相应的回退值扫描清单文件，如果相应的环境
变量未定义：

<table style="width:100%">
  <tr>
    <th>搜索顺序</th>
    <th>目录/环境变量</th>
    <th>回退</th>
    <th>附加说明</th>
  </tr>
  <tr>
    <td>1</td>
    <td>$XDG_CONFIG_HOME</td>
    <td>$HOME/.config</td>
    <td><b>在以提升权限运行时（如
           setuid、setgid 或文件系统功能）会忽略此路径</b>。<br/>
        这是因为在这些情况下，信任
        环境变量是非恶意的并不安全。
        有关更多信息，请参阅 <a href="LoaderInterfaceArchitecture.md#elevated-privilege-caveats">
        提升权限注意事项</a>。
    </td>
  </tr>
  <tr>
    <td>1</td>
    <td>$XDG_CONFIG_DIRS</td>
    <td>/etc/xdg</td>
    <td></td>
  </tr>
  <tr>
    <td>2</td>
    <td>SYSCONFDIR</td>
    <td>/etc</td>
    <td>编译时选项，设置为可能从非 Linux 发行版提供的包安装的层的位置。
    </td>
  </tr>
  <tr>
    <td>3</td>
    <td>EXTRASYSCONFDIR</td>
    <td>/etc</td>
    <td>编译时选项，设置为可能从非 Linux 发行版提供的包安装的层的位置。
        通常仅在 SYSCONFDIR 设置为 /etc 以外的其他值时设置
    </td>
  </tr>
  <tr>
    <td>4</td>
    <td>$XDG_DATA_HOME</td>
    <td>$HOME/.local/share</td>
    <td><b>在以提升权限运行时（如
           setuid、setgid 或文件系统功能）会忽略此路径</b>。<br/>
        这是因为在这些情况下，信任
        环境变量是非恶意的并不安全。
        有关更多信息，请参阅 <a href="LoaderInterfaceArchitecture.md#elevated-privilege-caveats">
        提升权限注意事项</a>。
    </td>
  </tr>
  <tr>
    <td>5</td>
    <td>$XDG_DATA_DIRS</td>
    <td>/usr/local/share/:/usr/share/</td>
    <td></td>
  </tr>
</table>

目录列表使用标准平台路径
分隔符（:）连接在一起。
然后加载器选择每个路径，并为其应用后缀以搜索特定的
层类型，并在该特定文件夹中查找
清单文件：

  * 隐式层：后缀 =  /vulkan/implicit_layer.d
  * 显式层：后缀 =  /vulkan/explicit_layer.d

如果定义了 `VK_LAYER_PATH`，则加载器将查看该
变量定义的路径以查找显式层清单文件，而不是使用
上述标准显式层路径提供的信息。

如果定义了 `VK_ADD_LAYER_PATH`，则加载器将查看提供的
路径以查找显式层清单文件，同时使用
上述标准显式层路径提供的信息。
`VK_ADD_LAYER_PATH` 提供的路径在标准搜索文件夹列表之前添加
，因此将首先搜索。

如果存在 `VK_LAYER_PATH`，则加载器不会使用 `VK_ADD_LAYER_PATH`，任何值都将被
忽略。

如果定义了 `VK_IMPLICIT_LAYER_PATH`，则加载器将查看该
变量定义的路径以查找隐式层清单文件，而不是使用
上述标准隐式层路径提供的信息。

如果定义了 `VK_ADD_IMPLICIT_LAYER_PATH`，则加载器将查看
提供的路径以查找隐式层清单文件，同时使用
上述标准隐式层路径提供的信息。
`VK_ADD_IMPLICIT_LAYER_PATH` 提供的路径在标准
搜索文件夹列表之前添加，因此将首先搜索。

如果存在 `VK_IMPLICIT_LAYER_PATH`，则加载器不会使用 `VK_ADD_IMPLICIT_LAYER_PATH`，任何值都将被
忽略。

出于安全原因，如果以提升权限运行，`VK_LAYER_PATH`、`VK_ADD_LAYER_PATH`、`VK_IMPLICIT_LAYER_PATH`
和 `VK_ADD_IMPLICIT_LAYER_PATH` 将被忽略。
有关更多信息，请参阅[提升权限的例外情况](#提升权限的例外情况)
。

**注意** 虽然搜索清单文件的文件夹顺序是明确定义的，
但加载器在每个目录中读取内容的顺序是
[由于 readdir 的行为而随机](https://www.ibm.com/support/pages/order-directory-contents-returned-calls-readdir)。

有关此内容的更多信息，请参阅
[LoaderApplicationInterface.md 文档](LoaderApplicationInterface.md)中的
[强制层源文件夹](LoaderApplicationInterface.md#forcing-layer-source-folders)
。

同样重要的是要注意，虽然 `VK_LAYER_PATH`、`VK_ADD_LAYER_PATH`、
`VK_IMPLICIT_LAYER_PATH` 和 `VK_ADD_IMPLICIT_LAYER_PATH` 会将加载器指向
搜索清单文件的路径，但它不能保证
清单中提到的库文件会立即被找到。
通常，层清单文件将使用相对
或绝对路径指向库文件。
当使用相对或绝对路径时，加载器通常可以找到
库文件，而无需查询操作系统。
但是，如果库仅按名称列出，加载器可能找不到它。
如果查找与层关联的库文件时出现问题，请尝试更新
`LD_LIBRARY_PATH` 环境变量以指向
相应的 `.so` 文件的位置。


#### Linux 显式层搜索路径示例

对于虚构用户 "me"，层清单搜索路径可能
如下所示：

```
  /home/me/.config/vulkan/explicit_layer.d
  /etc/xdg/vulkan/explicit_layer.d
  /usr/local/etc/vulkan/explicit_layer.d
  /etc/vulkan/explicit_layer.d
  /home/me/.local/share/vulkan/explicit_layer.d
  /usr/local/share/vulkan/explicit_layer.d
  /usr/share/vulkan/explicit_layer.d
```

### Fuchsia 层发现

在 Fuchsia 上，Vulkan 加载器将使用环境
变量或相应的回退值扫描清单文件，如果相应的环境
变量未定义，则与 [Linux](#linux-层发现) 相同。
**唯一的**区别是 Fuchsia 不允许
*$XDG_DATA_DIRS* 或 *$XDG_HOME_DIRS* 的回退值。


### macOS 层发现

在 macOS 上，Vulkan 加载器将使用
应用程序资源文件夹以及环境变量或相应的回退
值（如果相应的环境变量未定义）扫描清单文件。
顺序类似于 Linux 上的搜索路径，但
应用程序的包资源首先被搜索：
`(bundle)/Contents/Resources/`。

#### macOS 隐式层搜索路径示例

对于虚构用户 "Me"，层清单搜索路径可能
如下所示：

```
  <bundle>/Contents/Resources/vulkan/implicit_layer.d
  /Users/Me/.config/vulkan/implicit_layer.d
  /etc/xdg/vulkan/implicit_layer.d
  /usr/local/etc/vulkan/implicit_layer.d
  /etc/vulkan/implicit_layer.d
  /Users/Me/.local/share/vulkan/implicit_layer.d
  /usr/local/share/vulkan/implicit_layer.d
  /usr/share/vulkan/implicit_layer.d
```

### 层过滤

**注意：** 此功能仅在构建时使用版本
1.3.234 的 Vulkan 头文件的加载器中可用。

加载器支持过滤环境变量，可以强制启用和
禁用已知层。
已知层是加载器已经找到的层，考虑了
默认搜索路径和环境变量 `VK_LAYER_PATH`、`VK_ADD_LAYER_PATH`、
`VK_IMPLICIT_LAYER_PATH` 和 `VK_ADD_IMPLICIT_LAYER_PATH`。

过滤变量将与层清单文件中提供的
层名称进行比较。

过滤器还必须遵循
[LoaderLayerInterface](LoaderLayerInterface.md) 文档的
[过滤环境变量行为](LoaderInterfaceArchitecture.md#filter-environment-variable-behaviors)
部分中定义的行为。

#### 层启用过滤

层启用环境变量 `VK_LOADER_LAYERS_ENABLE` 是一个
逗号分隔的 glob 列表，用于在已知层中搜索。
层名称与环境
变量中列出的 glob 进行比较，如果匹配，它们将自动添加到每个应用程序的加载器中的已启用
层列表。
这些层在隐式层之后但在其他显式层之前启用。

当使用 `VK_LOADER_LAYERS_ENABLE` 过滤器启用层时，如果
加载器日志设置为发出警告或层消息，则会
为每个被强制启用的层显示一条消息。
此消息将如下所示：

```
[Vulkan Loader] WARNING | LAYER:  Layer "VK_LAYER_LUNARG_wrap_objects" force enabled due to env var 'VK_LOADER_LAYERS_ENABLE'
```

#### 层禁用过滤

层禁用环境变量 `VK_LOADER_LAYERS_DISABLE` 是一个
逗号分隔的 glob 列表，用于在已知层中搜索。
层名称与环境
变量中列出的 glob 进行比较，如果匹配，它们将自动被禁用（无论
层是隐式还是显式）。
这意味着它们不会被添加到每个应用程序的加载器中的已启用层列表
。
这可能意味着应用程序请求的层也不会被启用
，例如 `VK_KHRONOS_LAYER_synchronization2`，这可能导致某些应用程序
行为异常。

当使用 `VK_LOADER_LAYERS_DISABLE` 过滤器禁用层时，如果
加载器日志设置为发出警告或层消息，则会
为每个被强制禁用的层显示一条消息。
此消息将如下所示：

```
[Vulkan Loader] WARNING | LAYER:  Layer "VK_LAYER_LUNARG_wrap_objects" disabled because name matches filter of env var 'VK_LOADER_LAYERS_DISABLE'
```

#### 层特殊情况禁用

因为存在不同类型的层，使用 `VK_LOADER_LAYERS_DISABLE` 环境
变量时，还有 3 个额外的特殊
禁用选项可用。

这些是：

  * `~all~`
  * `~implicit~`
  * `~explicit~`

`~all~` 将有效地禁用每个层。
这使开发人员能够禁用系统上的所有层。
`~implicit~` 将有效地禁用每个隐式层（保留显式
层仍然存在于应用程序调用链中）。
`~explicit~` 将有效地禁用每个显式层（保留隐式
层仍然存在于应用程序调用链中）。

#### 层禁用警告

禁用层，无论是仅通过正常使用
`VK_LOADER_LAYERS_DISABLE` 还是通过调用特殊禁用选项（如
`~all~` 或 `~explicit~`），如果应用程序
依赖于一个或多个显式层提供的功能，则可能导致应用程序中断。

#### 允许某些层忽略层禁用

**注意：** VK_LOADER_LAYERS_DISABLE 仅在构建时使用版本
1.3.262 的 Vulkan 头文件的加载器中可用。

层允许环境变量 `VK_LOADER_LAYERS_ALLOW` 是一个
逗号分隔的 glob 列表，用于在已知层中搜索。
层名称与环境
变量中列出的 glob 进行比较，如果匹配，它们将无法被
`VK_LOADER_LAYERS_DISABLE` 禁用。

隐式层具有仅在指定层
环境变量设置时才启用的能力，允许上下文相关的启用。
`VK_LOADER_LAYERS_ENABLE` 忽略该上下文。
`VK_LOADER_LAYERS_ALLOW` 的行为类似于 `VK_LOADER_LAYERS_ENABLE`，同时
也尊重通常用于确定是否应启用
隐式层的上下文。

`VK_LOADER_LAYERS_ALLOW` 有效地否定了
`VK_LOADER_LAYERS_DISABLE` 的行为。
由 `VK_LOADER_LAYERS_ALLOW` 列出的显式层不会被启用。
由 ``VK_LOADER_LAYERS_ALLOW` 列出的隐式层始终处于活动状态，
即它们不需要任何外部上下文即可启用，将被启用。

##### `VK_INSTANCE_LAYERS`

原始的 `VK_INSTANCE_LAYERS` 可以视为
`VK_LOADER_LAYERS_ENABLE` 的特殊情况。
因此，通过 `VK_INSTANCE_LAYERS` 启用的任何层将被视为与
使用 `VK_LOADER_LAYERS_ENABLE` 启用的层相同，因此将
覆盖 `VK_LOADER_LAYERS_DISABLE` 中提供的任何禁用。

### 提升权限的例外情况

出于安全原因，如果以提升权限运行 Vulkan 应用程序，
`VK_LAYER_PATH`、`VK_ADD_LAYER_PATH`、`VK_IMPLICIT_LAYER_PATH`
和 `VK_ADD_IMPLICIT_LAYER_PATH` 将被忽略。
这是因为它们可能会将新的库插入到可执行进程中，而这些库
通常不会被加载器找到。
因此，这些环境变量只能用于
不使用提升权限的应用程序。

有关更多信息，请参阅
顶级 [LoaderInterfaceArchitecture.md][LoaderInterfaceArchitecture.md] 文档中的
[提升权限注意事项](LoaderInterfaceArchitecture.md#elevated-privilege-caveats)
。

## 层版本协商

现在层已被发现，应用程序可以选择加载它，或者
对于隐式层，它可以默认加载。
当加载器尝试加载层时，它首先做的是尝试
协商加载器到层接口的版本。
为了协商加载器/层接口版本，层必须
实现 `vkNegotiateLoaderLayerInterfaceVersion` 函数。
以下信息在
include/vulkan/vk_layer.h 中提供此接口：

```cpp
typedef enum VkNegotiateLayerStructType {
    LAYER_NEGOTIATE_INTERFACE_STRUCT = 1,
} VkNegotiateLayerStructType;

typedef struct VkNegotiateLayerInterface {
    VkNegotiateLayerStructType sType;
    void *pNext;
    uint32_t loaderLayerInterfaceVersion;
    PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr;
    PFN_GetPhysicalDeviceProcAddr pfnGetPhysicalDeviceProcAddr;
} VkNegotiateLayerInterface;

VkResult
   vkNegotiateLoaderLayerInterfaceVersion(
      VkNegotiateLayerInterface *pVersionStruct);
```

`VkNegotiateLayerInterface` 结构类似于其他 Vulkan 结构。
在这种情况下，"sType" 字段采用仅为内部
加载器/层接口使用而定义的新枚举。
"sType" 的有效值可能会在未来增长，但现在只有
一个值 "LAYER_NEGOTIATE_INTERFACE_STRUCT"。

此函数（`vkNegotiateLoaderLayerInterfaceVersion`）应该由
层导出，以便在 Windows 上使用 "GetProcAddress" 或在 Linux 或
macOS 上使用 "dlsym"，应该返回指向它的有效函数指针。
一旦加载器获取了层函数的有效地址，加载器
将创建一个类型为 `VkNegotiateLayerInterface` 的变量并按以下方式初始化它
：
 1. 将结构 "sType" 设置为 "LAYER_NEGOTIATE_INTERFACE_STRUCT"
 2. 将 pNext 设置为 NULL。
     - 这是为了未来的增长
 3. 将 "loaderLayerInterfaceVersion" 设置为加载器希望
将接口设置到的当前版本。
      - 加载器发送的最小值将是 2，因为它是第一个
支持此函数的版本。

然后加载器将单独调用每个层的
`vkNegotiateLoaderLayerInterfaceVersion` 函数，使用填充的
"VkNegotiateLayerInterface"。

此函数允许加载器和层就使用的接口版本达成一致
。
"loaderLayerInterfaceVersion" 字段既是输入参数也是输出参数。
"loaderLayerInterfaceVersion" 由加载器填充，其中包含所需的
最新接口版本，该版本由加载器支持（通常是最新的）。
层接收此值并在同一
字段中返回它期望的版本。
因为它正在设置加载器和层之间的接口版本，
这应该是加载器对层进行的第一次调用（甚至在
任何对 `vkGetInstanceProcAddr` 的调用之前）。

如果接收调用的层不再支持加载器提供的接口
版本（由于弃用），则它应该报告
`VK_ERROR_INITIALIZATION_FAILED` 错误。
否则，它将 "loaderLayerInterfaceVersion" 指向的值设置为
最新接口版本，该版本由层和加载器共同支持，并返回
`VK_SUCCESS`。

如果加载器提供的接口版本比层支持的更新，层应该报告 `VK_SUCCESS`，因为确定它是否可以支持层支持的较旧接口版本是加载器的责任。
如果层的接口版本大于加载器的接口版本，层也应该报告 `VK_SUCCESS`，但返回加载器的版本。
因此，在返回 `VK_SUCCESS` 时，"loaderLayerInterfaceVersion" 将包含
层要使用的所需接口版本。

如果加载器接收到 `VK_ERROR_INITIALIZATION_FAILED` 而不是 `VK_SUCCESS`，
则加载器将层视为不可用，并且不会加载它。
在这种情况下，应用程序在枚举期间不会看到该层。
*请注意，加载器目前与所有层
接口版本向后兼容，因此层不应该能够请求
比加载器支持的更旧的版本。*

此函数**不得**向下调用层链到下一层。
加载器将与每个层单独协作。

如果层支持新接口并报告版本 2 或更高版本，则
层应该填充其内部
函数的函数指针值：
    - "pfnGetInstanceProcAddr" 应该设置为层的内部
`GetInstanceProcAddr` 函数。
    - "pfnGetDeviceProcAddr" 应该设置为层的内部
`GetDeviceProcAddr` 函数。
    - "pfnGetPhysicalDeviceProcAddr" 应该设置为层的内部
`GetPhysicalDeviceProcAddr` 函数。
      - 如果层不支持物理设备扩展，它可以将
值设置为 NULL。
      - 稍后有关此函数的更多信息
加载器将使用来自 "VkNegotiateLayerInterface" 结构的 "fpGetInstanceProcAddr" 和 "fpGetDeviceProcAddr"
函数。
在这些更改之前，加载器将使用 Windows 上的 "GetProcAddress" 或 Linux 或 macOS 上的 "dlsym" 查询这些函数中的每一个。


## 层调用链和分布式调度

有两个关键的架构特性驱动加载器到
`层库` 接口：
 1. 分离且不同的实例和设备调用链
 2. 分布式调度。

有关更多信息，请阅读上面
[LoaderInterfaceArchitecture.md 文档](LoaderInterfaceArchitecture.md)的
[调度表和调用链](LoaderInterfaceArchitecture.md#dispatch-tables-and-call-chains)
部分中调度表和调用链的概述。

这里需要注意的是，层可以拦截 Vulkan 实例
函数、设备函数或两者。
对于层拦截实例函数，它必须参与
实例调用链。
对于层拦截设备函数，它必须参与设备
调用链。

请记住，层不需要拦截所有实例或设备函数，
相反，它可以选择仅拦截这些函数的子集。

通常，当层拦截给定的 Vulkan 函数时，它将根据需要向下调用
实例或设备调用链。
加载器和参与调用链的所有层库协作
以确保从一个实体到下一个实体的调用顺序正确。
这种用于调用链排序的协作努力在下文中称为
**分布式调度**。

在分布式调度中，每个层负责正确调用调用链中的下一个
实体。
这意味着对于层拦截的所有 Vulkan 函数，都需要调度机制
。
如果 Vulkan 函数未被层拦截，或者如果层选择通过不向下调用链来
终止函数，则不需要为该特定函数进行调度
。

例如，如果启用的层仅拦截某些实例函数，
调用链将如下所示：
![实例函数链](images/function_instance_chain.png)

同样，如果启用的层仅拦截几个设备函数，
调用链可能如下所示：
![设备函数链](images/function_device_chain.png)

加载器负责将所有核心和实例扩展 Vulkan
函数调度到调用链中的第一个实体。


## 层未知物理设备扩展

拦截以 `VkPhysicalDevice` 作为第一个
参数的入口点的层*应该*支持 `vk_layerGetPhysicalDeviceProcAddr`。此函数
已添加到层接口版本 2，允许加载器区分
以 `VkDevice` 和 `VkPhysicalDevice` 作为第一个
参数的入口点。这允许加载器优雅地正确支持它未知的入口点
。

```cpp
PFN_vkVoidFunction
   vk_layerGetPhysicalDeviceProcAddr(
      VkInstance instance,
      const char* pName);
```

此函数的行为类似于 `vkGetInstanceProcAddr` 和
`vkGetDeviceProcAddr`，但它应该只返回物理设备
扩展入口点的值。
通过这种方式，它将 "pName" 与层支持的每个物理设备函数进行比较
。

该函数的实现应具有以下行为：
  * 如果它是层支持的物理设备函数的名称，
则应返回指向层的相应函数的指针。
  * 如果它是**不是**物理设备
函数的有效函数名称（即实例、设备或层实现的其他函数），则应返回 NULL 值。
    * 层不向下调用，因为命令不是物理设备
 扩展。
  * 如果层不知道此函数是什么，它应该向下调用
层链到下一个 `vk_layerGetPhysicalDeviceProcAddr` 调用。
    * 可以通过以下两种方式之一检索：
      * 在 `vkCreateInstance` 期间，它被传递给链中的层
，信息被传递给 `VkLayerInstanceCreateInfo` 结构中的层。
        * 使用 `get_chain_info()` 获取指向
`VkLayerInstanceCreateInfo` 结构的指针。让我们称它为 chain_info。
        * 地址在
chain_info->u.pLayerInfo->pfnNextGetPhysicalDeviceProcAddr 下
        * 请参阅
[CreateInstance 示例代码](#createinstance-示例代码)
      * 使用下一层的 `GetInstanceProcAddr` 函数查询
`vk_layerGetPhysicalDeviceProcAddr`。

如果层打算支持以 VkPhysicalDevice 作为
可调度参数的函数，则层应该支持
`vk_layerGetPhysicalDeviceProcAddr`。
这是因为如果这些函数不为加载器所知，例如来自
未发布的扩展或因为加载器是较旧的构建，因此
_尚未_知道它们，加载器将无法区分这是
设备函数还是物理设备函数。

如果层确实实现了 `vk_layerGetPhysicalDeviceProcAddr`，它应该在
[层版本协商](#层版本协商) 期间在 `VkNegotiateLayerInterface`
结构的 "pfnGetPhysicalDeviceProcAddr" 成员中返回其 `vk_layerGetPhysicalDeviceProcAddr` 函数的地址
。
此外，层还应该确保 `vkGetInstanceProcAddr` 返回
指向 `vk_layerGetPhysicalDeviceProcAddr` 查询的有效函数指针。

注意：如果层包装 VkInstance 句柄，对
`vk_layerGetPhysicalDeviceProcAddr` 的支持*不是*可选的，必须实现。

加载器的 `vkGetInstanceProcAddr` 对
`vk_layerGetPhysicalDeviceProcAddr` 函数的支持行为如下：
 1. 检查是否是核心函数：
    - 如果是，返回函数指针
 2. 检查是否是已知的实例或设备扩展函数：
    - 如果是，返回函数指针
 3. 调用层/驱动程序的 `GetPhysicalDeviceProcAddr`
    - 如果它返回 non-NULL，返回指向通用物理设备
函数的跳板，并设置一个通用终止符，该终止符将把它传递给适当的
驱动程序。
 4. 使用 `GetInstanceProcAddr` 向下调用
    - 如果它返回 non-NULL，将其视为未知逻辑设备命令。
这意味着设置一个通用跳板函数，该函数接受 `VkDevice`
作为第一个参数，并调整调度表以在从 `VkDevice` 获取调度表后调用
驱动程序/层的函数。
然后，返回指向相应跳板函数的指针。
 5. 返回 NULL

然后，如果命令稍后提升为核心，它将不再
使用 `vk_layerGetPhysicalDeviceProcAddr` 设置。
此外，如果加载器添加对扩展的直接支持，它将不再
到达步骤 3，因为步骤 2 将返回有效的函数指针。
但是，层应该继续支持通过
`vk_layerGetPhysicalDeviceProcAddr` 查询命令，至少直到 Vulkan 版本更新，
因为较旧的加载器可能仍在使用这些命令。

### 添加 `vk_layerGetPhysicalDeviceProcAddr` 的原因

最初，如果在加载器中调用 `vkGetInstanceProcAddr`，它会导致
以下行为：
 1. 加载器将检查它是否是核心函数：
    - 如果是，它将返回函数指针
 2. 加载器将检查它是否是已知的扩展函数：
    - 如果是，它将返回函数指针
 3. 如果加载器对它一无所知，它将使用
`GetInstanceProcAddr` 向下调用
    - 如果它返回 non-NULL，将其视为未知逻辑设备命令。
    - 这意味着设置一个通用跳板函数，该函数接受
VkDevice 作为第一个参数，并调整调度表以在从 `VkDevice` 获取调度表后调用
驱动程序/层的函数。
 4. 如果上述所有操作都失败，加载器将向应用程序返回 NULL。

这导致当层尝试公开加载器不知道的新物理设备
扩展时出现问题，但应用程序知道。
因为加载器对它一无所知，加载器将到达上述过程中的步骤 3，并将该函数视为未知逻辑设备
命令。
问题是，这将创建一个通用 VkDevice 跳板函数，该函数
在第一次调用时，将尝试将 VkPhysicalDevice 解引用为
VkDevice。
这将导致崩溃或损坏。


## 层拦截要求

  * 层通过定义具有
与 Vulkan API 该函数**完全相同**签名的 C/C++ 函数来拦截 Vulkan 函数。
  * 层**必须至少拦截** `vkGetInstanceProcAddr` 和
`vkCreateInstance` 以参与实例调用链。
  * 层**也可以拦截** `vkGetDeviceProcAddr` 和 `vkCreateDevice`
以参与设备调用链。
  * 对于层拦截的任何具有非 void 返回
值的 Vulkan 函数，**层拦截
函数必须返回适当的值**。
  * 层拦截的大多数函数**应该向下调用链**到
下一个实体中的相应 Vulkan 函数。
    * 层的常见行为是拦截调用，执行某些
行为，然后将其传递给下一个实体。
      * 如果层不向下传递信息，可能会发生未定义的行为
        。
      * 这是因为函数不会被链中更下方的层
或任何驱动程序接收。
    * **永远不得向下调用链**的一个函数是：
      * `vkNegotiateLoaderLayerInterfaceVersion`
    * **可能不向下调用链**的三个常见函数是：
      * `vkGetInstanceProcAddr`
      * `vkGetDeviceProcAddr`
      * `vk_layerGetPhysicalDeviceProcAddr`
      * 这些函数仅对它们不拦截的 Vulkan 函数向下调用链。
  * 层拦截函数**可以插入额外的调用**到 Vulkan 函数，除了
拦截之外。
    * 例如，拦截 `vkQueueSubmit` 的层可能希望在向下调用链调用 `vkQueueSubmit` 之后添加对 `vkQueueWaitIdle` 的调用。
    * 这将导致两次向下调用链：首先向下调用
`vkQueueSubmit` 链，然后向下调用 `vkQueueWaitIdle` 链。
    * 层插入的任何额外调用必须在同一链上
      * 如果函数是设备函数，只应添加其他设备函数
。
      * 同样，如果函数是实例函数，只应添加其他实例
函数。


## 分布式调度要求

- 对于层拦截的每个入口点，它必须跟踪
链中下一个实体中的入口点，它将向下调用。
  * 换句话说，层必须有一个指向适当类型函数的指针列表，以调用下一个实体。
  * 这可以通过各种方式实现，但
为了清楚起见，将称为调度表。
- 层可以使用 `VkLayerDispatchTable` 结构作为设备调度
表（请参阅 include/vulkan/vk_dispatch_table_helper.h）。
- 层可以使用 `VkLayerInstanceDispatchTable` 结构作为实例
调度表（请参阅 include/vulkan/vk_dispatch_table_helper.h）。
- 层的 `vkGetInstanceProcAddr` 函数使用下一个实体的
`vkGetInstanceProcAddr` 向下调用链以获取未知（即
非拦截的）函数。
- 层的 `vkGetDeviceProcAddr` 函数使用下一个实体的
`vkGetDeviceProcAddr` 向下调用链以获取未知（即非拦截的）
函数。
- 层的 `vk_layerGetPhysicalDeviceProcAddr` 函数使用下一个实体的
`vk_layerGetPhysicalDeviceProcAddr` 向下调用链以获取未知（即
非拦截的）函数。


## 层约定和规则

当层插入到其他符合规范的 Vulkan 驱动程序中时，它<b>必须</b>
仍然产生符合规范的 Vulkan 驱动程序。
目的是让层具有明确定义的基线行为。
因此，它必须遵循下面定义的一些约定和规则。

为了使层具有唯一名称，并减少加载器尝试加载这些层时可能发生的冲突
机会，层
<b>必须</b> 遵循以下命名标准：
 * 以 `VK_LAYER_` 前缀开头
 * 在前缀后跟组织或公司名称（LunarG）、
   唯一公司标识符（NV 代表 Nvidia）或软件产品名称
   （RenderDoc），全部大写
 * 后跟层的特定名称（通常是小写，但不
   要求是）
   * 注意：特定名称，如果超过一个单词，<b>必须</b> 用下划线
     分隔

有效层名称的示例包括：
 * <b>VK_LAYER_KHRONOS_validation</b>
   * 组织 = "KHRONOS"
   * 特定名称 = "validation"
 * <b>VK_LAYER_RENDERDOC_Capture</b>
   * 应用程序 = "RENDERDOC"
   * 特定名称 = "Capture"
 * <b>VK_LAYER_VALVE_steam_fossilize_32</b>
   * 组织 = "VALVE"
   * 应用程序 = "steam"
   * 特定名称 = "fossilize"
   * OS 修饰符 = "32"  （用于 32 位版本）
 * <b>VK_LAYER_NV_nsight</b>
   * 组织缩写 = "NV"（代表 Nvidia）
   * 特定名称 = "nsight"

有关层命名的更多详细信息，请参阅
[Vulkan 风格指南](https://www.khronos.org/registry/vulkan/specs/1.2/styleguide.html#extensions-naming-conventions)
第 3.4 节"版本、扩展和层命名约定"。

层总是与其他层链接在一起。
它不得对其下层进行无效调用，也不得依赖其未定义的行为。
当它更改函数的行为时，它必须确保其上层
不会因为更改的行为而对其下层进行无效调用或依赖其未定义的行为。
例如，当层拦截对象创建函数以包装
其下层创建的对象时，它必须确保其下层永远不会
看到包装对象，无论是直接从自身还是间接从其上层
。

当层需要主机内存时，它可能忽略提供的分配器。
如果层旨在在生产环境中运行，则首选层使用任何提供的内存分配器。
例如，这通常适用于始终启用的隐式层。
这将允许应用程序包含层的内存使用。

其他规则包括：
  - `vkEnumerateInstanceLayerProperties` **必须**枚举并**仅**
枚举层本身。
  - `vkEnumerateInstanceExtensionProperties` **必须**处理
`pLayerName` 是它自身的情况。
    - 否则，它**必须**返回 `VK_ERROR_LAYER_NOT_PRESENT`，包括当
`pLayerName` 是 `NULL` 时。
  - `vkEnumerateDeviceLayerProperties` **已弃用，可以省略**。
    - 使用此函数将导致未定义的行为。
  - `vkEnumerateDeviceExtensionProperties` **必须**处理
`pLayerName` 是它自身的情况。
    - 在其他情况下，它应该链接到其他层。
  - `vkCreateInstance` **不得**为无法识别的层
名称和扩展名称生成错误。
    - 它可以假设层名称和扩展名称已经过验证。
  - `vkGetInstanceProcAddr` 通过返回本地
入口点来拦截 Vulkan 函数
    - 否则它返回通过向下调用实例调用
链获得的值。
  - `vkGetDeviceProcAddr` 通过返回本地
入口点来拦截 Vulkan 函数
    - 否则它返回通过向下调用设备调用
链获得的值。
    - 如果层实现
设备级调用链，则必须拦截这些附加函数：
      - `vkGetDeviceProcAddr`
      - `vkCreateDevice`（仅对任何设备级链必需）
         - **注意：** 较旧的层库可能期望
           `vkGetInstanceProcAddr`
在 `pName` 是 `vkCreateDevice` 时忽略 `instance`。
  - 规范**要求**从
`vkGetInstanceProcAddr` 和 `vkGetDeviceProcAddr` 为禁用的函数返回 `NULL`。
    - 层可以自己返回 `NULL` 或依赖以下层这样做。
  - 层的实现 `vkGetInstanceProcAddr` **应该**，在查询
`vkCreateInstance` 时，无论
`instance` 参数的值如何，都返回有效的函数指针。
    - 规范**要求** `instance` 参数**必须**为 NULL。
但是，较旧版本的规范没有此要求，允许
传入非 NULL `instance` 句柄并返回有效的 `vkCreateInstance`
函数指针。Vulkan-Loader 本身这样做，并将继续这样做以
与在此规范
更改之前发布的层保持兼容。

## 层调度初始化

- 层在其 `vkCreateInstance`
函数内初始化其实例调度表。
- 层在其 `vkCreateDevice`
函数内初始化其设备调度表。
- 加载器通过
`VkInstanceCreateInfo` 和 `VkDeviceCreateInfo`
结构中的 "pNext" 字段分别向层传递初始化结构的链表，用于 `vkCreateInstance` 和 `VkCreateDevice`。
- 此链表中的头节点对于
实例是 `VkLayerInstanceCreateInfo` 类型，对于设备是 VkLayerDeviceCreateInfo 类型。
有关详细信息，请参阅文件 `include/vulkan/vk_layer.h`。
- VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO 由加载器用于
`VkLayerInstanceCreateInfo` 中的 "sType" 字段。
- VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO 由加载器用于
`VkLayerDeviceCreateInfo` 中的 "sType" 字段。
- "function" 字段指示联合字段 "u" 在
`VkLayer*CreateInfo` 中应如何解释。
加载器将 "function" 字段设置为 VK_LAYER_LINK_INFO。
这表示 "u" 字段应该是 `VkLayerInstanceLink` 或
`VkLayerDeviceLink`。
- `VkLayerInstanceLink` 和 `VkLayerDeviceLink` 结构是列表
节点。
- `VkLayerInstanceLink` 包含下一个实体的 `vkGetInstanceProcAddr`
，由层使用。
- `VkLayerDeviceLink` 包含下一个实体的 `vkGetInstanceProcAddr`
和 `vkGetDeviceProcAddr`，由层使用。
- 给定加载器设置的上述结构，层必须按以下方式初始化其
调度表：
  - 在 `VkInstanceCreateInfo`/`VkDeviceCreateInfo` 结构中找到 `VkLayerInstanceCreateInfo`/`VkLayerDeviceCreateInfo` 结构。
  - 从 "pLayerInfo" 字段获取下一个实体的 vkGet*ProcAddr。
  - 对于 CreateInstance，通过调用
"pfnNextGetInstanceProcAddr" 获取下一个实体的 `vkCreateInstance`：
     pfnNextGetInstanceProcAddr(NULL, "vkCreateInstance")。
  - 对于 CreateDevice，通过调用
"pfnNextGetInstanceProcAddr" 获取下一个实体的 `vkCreateDevice`：
pfnNextGetInstanceProcAddr(instance, "vkCreateDevice")，传递
已创建的实例句柄。
  - 将链表推进到下一个节点：pLayerInfo = pLayerInfo->pNext。
  - 向下调用链 `vkCreateDevice` 或 `vkCreateInstance`
  - 通过为调度表中需要的每个 Vulkan 函数调用一次下一个实体的
Get*ProcAddr 函数来初始化层调度表

## CreateInstance 示例代码

```cpp
VkResult
   vkCreateInstance(
      const VkInstanceCreateInfo *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkInstance *pInstance)
{
   VkLayerInstanceCreateInfo *chain_info =
        get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
        chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance =
        (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // 将链接信息推进到链的下一个元素。
    // 这确保下一层获得它的层信息，而不是
    // 我们当前层的信息。
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    // 继续向下调用链
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    // 使用链中下一层的 GetInstanceProcAddr 初始化层的调度表。
    instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(
        *pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    // 其他层初始化
    ...

    return VK_SUCCESS;
}
```

## CreateDevice 示例代码

```cpp
VkResult
   vkCreateDevice(
      VkPhysicalDevice gpu,
      const VkDeviceCreateInfo *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDevice *pDevice)
{
    VkInstance instance = GetInstanceFromPhysicalDevice(gpu);
    VkLayerDeviceCreateInfo *chain_info =
        get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr =
        chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr =
        chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice =
        (PFN_vkCreateDevice)fpGetInstanceProcAddr(instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // 将链接信息推进到链的下一个元素。
    // 这确保下一层获得它的层信息，而不是
    // 我们当前层的信息。
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // 初始化层的调度表
    device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(
        *pDevice, device_dispatch_table, fpGetDeviceProcAddr);

    // 其他层初始化
    ...

    return VK_SUCCESS;
}
```
在这种情况下，调用函数 `GetInstanceFromPhysicalDevice` 来获取
实例句柄。
实际上，这将通过层选择的任何方法从物理设备获取
实例句柄来完成。


## 元层

元层是一种特殊类型的层，仅通过
Khronos 加载器可用。
虽然普通层与一个特定的库关联，但元层
实际上是一个集合层，包含其他层的有序列表
（称为组件层）。

元层的好处是：
 1. 可以通过简单地将多个层分组到元层中来使用单个层名称激活多个层
。
 2. 可以定义各个组件层的加载顺序
在元层内。
 3. 层配置（元层清单文件内部）可以轻松
与他人共享。
 4. 加载器将自动整理元层组件层中的所有实例和设备扩展，并在查询时将它们报告为元层的属性
给应用程序。

定义和使用元层的限制是：
 1. 元层清单文件**必须**是正确格式化的，包含
一个或多个组件层。
 3. 所有组件层**必须**存在于系统上，元层才能
被使用。
 4. 所有组件层**必须**与元层处于相同的 Vulkan API 主版本和次版本
，元层才能被使用。

元层组件层在实例或设备调用链中的顺序很简单：
  * 列出的第一层将是最接近应用程序的层。
  * 列出的最后一层将是最接近驱动程序的层。

在元层清单文件内，每个组件层按其
层名称列出。
这是与每个组件层清单
文件下的 "layer" 或 "layers" 标签关联的 "name" 标签的值。
这也是在 `vkCreateInstance` 期间激活层时通常使用的名称
。

组件层列表或全局所有启用层中的
任何重复层名称都将被加载器简单地忽略。
只会使用任何层名称的第一次出现。

例如，如果使用环境变量
`VK_INSTANCE_LAYERS` 启用层，并且在元层中列出了同一层，则
将使用环境变量启用的层，组件层将
被丢弃。
同样，如果一个人要启用元层，然后单独启用
之后的一个组件层，层名称的第二个实例
将被忽略。

定义元层所需的清单文件格式可以在
[层清单文件格式](#层清单文件格式) 部分找到。

### 覆盖元层

如果在系统上找到名称为
`VK_LAYER_LUNARG_override` 的隐式元层，加载器将其用作"覆盖"层。
这用于选择性地启用和禁用其他层被加载。
它可以全局应用或应用于特定应用程序或应用程序。
覆盖元层可以具有以下附加键：
  * `blacklisted_layers` - 即使应用程序请求也不应
加载的显式层名称列表。
  * `app_keys` - 覆盖层应用到的
可执行文件的路径列表。
  * `override_paths` - 将用作搜索位置的路径列表
用于组件层。

当应用程序启动且存在覆盖层时，加载器
首先检查应用程序是否在列表中。
如果不在，则不应用覆盖层。
如果列表为空或 `app_keys` 不存在，加载器使
覆盖层全局化，并在启动时将其应用于每个应用程序。

如果覆盖层包含 `override_paths`，则它专门使用此路径列表
用于组件层。
因此，它忽略默认显式和隐式层搜索
位置以及由环境变量（如 `VK_LAYER_PATH`）设置的路径。
如果任何组件层不在提供的覆盖路径中，则元
层被禁用。

覆盖元层主要在 using
[VkConfig](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md)
时启用，该工具包含在 Vulkan SDK 中。
它通常仅在 VkConfig 工具实际执行时可用。
有关更多信息，请参阅该文档。

## 实例前函数

Vulkan 包含少量在没有任何
可调度对象的情况下调用的函数。
<b>大多数层不拦截这些函数</b>，因为层在
创建实例时启用。
但是，在某些条件下，层可以拦截
这些函数。

层可能希望拦截这些实例前函数的一个原因是
过滤掉通常从 Vulkan 驱动程序返回到
应用程序的扩展。
[RenderDoc](https://renderdoc.org/) 就是这样一个层，它拦截这些
实例前函数，以便它可以禁用它不支持的扩展。

为了拦截实例前函数，必须满足几个条件：
* 层必须是隐式的
* 层清单版本必须是 1.1.2 或更高版本
* 层必须为每个拦截的函数导出入口点符号
* 层清单必须在
`pre_instance_functions` JSON 对象中指定每个拦截函数的名称

可以以这种方式拦截的函数是：
* `vkEnumerateInstanceExtensionProperties`
* `vkEnumerateInstanceLayerProperties`
* `vkEnumerateInstanceVersion`

实例前函数的工作方式与所有其他层拦截
函数不同。
其他拦截函数具有与它们拦截的
函数完全相同的函数原型。
然后它们依赖于在实例或设备
创建时传递给层的数据，以便层可以向下调用链。
因为在调用实例前函数之前不需要创建实例，这些函数必须使用单独的机制来构建
调用链。
此机制包括一个额外参数，当调用层
拦截函数时，该参数将传递给层
拦截函数。
此参数将是指向结构的指针，定义如下：

```cpp
typedef struct Vk...Chain
{
    struct {
        VkChainType type;
        uint32_t version;
        uint32_t size;
    } header;
    PFN_vkVoidFunction pfnNextLayer;
    const struct Vk...Chain* pNextLink;
} Vk...Chain;
```

这些结构在 `vk_layer.h` 文件中定义，因此不需要
在任何外部代码中重新定义链结构。
每个结构的名称与其对应的函数名称
相似，但前导 "V" 大写，并在末尾添加单词 "Chain"。
例如，`vkEnumerateInstanceExtensionProperties` 的结构称为
`VkEnumerateInstanceExtensionPropertiesChain`。
此外，`pfnNextLayer` 结构成员实际上不是 void 函数
指针 &mdash; 它的类型将是调用
链中每个函数的实际类型。

每个层拦截函数必须具有与
被拦截函数的原型相同的原型，除了第一个参数
必须是该函数的链结构（作为 const 指针传递）。
例如，希望拦截
`vkEnumerateInstanceExtensionProperties` 的函数将具有原型：

```cpp
VkResult
   InterceptFunctionName(
      const VkEnumerateInstanceExtensionPropertiesChain* pChain,
      const char* pLayerName,
      uint32_t* pPropertyCount,
      VkExtensionProperties* pProperties);
```

函数的名称是任意的；它可以是任何名称，只要它
在层清单文件中给出（请参阅
[层清单文件格式](#层清单文件格式)）。
每个拦截函数的实现负责调用
调用链中的下一项，使用链参数。
这是通过调用链结构的 `pfnNextLayer` 成员来完成的，传递
`pNextLink` 作为第一个参数，然后传递剩余的函数参数
。
例如，`vkEnumerateInstanceExtensionProperties` 的简单实现，除了向下调用
链之外什么都不做，将如下所示：

```cpp
VkResult
   InterceptFunctionName(
      const VkEnumerateInstanceExtensionPropertiesChain* pChain,
      const char* pLayerName,
      uint32_t* pPropertyCount,
      VkExtensionProperties* pProperties)
{
   return pChain->pfnNextLayer(
      pChain->pNextLink, pLayerName, pPropertyCount, pProperties);
}
```

使用 C++ 编译器时，每个链类型还定义一个名为
`CallDown` 的函数，可用于自动处理第一个参数。
使用此方法实现上述函数将如下所示：

```cpp
VkResult
   InterceptFunctionName(
      const VkEnumerateInstanceExtensionPropertiesChain* pChain,
      const char* pLayerName,
      uint32_t* pPropertyCount,
      VkExtensionProperties* pProperties)
{
   return pChain->CallDown(pLayerName, pPropertyCount, pProperties);
}
```

与层中的其他函数不同，层可能不会在这些函数调用之间保存任何全局数据
。
因为 Vulkan 在创建实例之前不存储任何状态，所有
层库在每次实例前调用结束时被释放。
这意味着隐式层可以使用实例前拦截来修改
函数返回的数据，但它们不能用于记录该数据。

## 特殊注意事项


### 在层内将私有数据与 Vulkan 对象关联

层可能希望将其自己的私有数据与一个或多个 Vulkan
对象关联。
执行此操作的两种常见方法是哈希映射和对象包装。


#### 包装

加载器支持层包装任何 Vulkan 对象，包括可调度
对象。
对于返回对象句柄的函数，每个层不接触
向下传递到调用链的值。
这是因为较低的项目可能需要使用原始值。
但是，当值从较低级别的层（可能是
驱动程序）返回时，层保存句柄并将其自己的句柄返回到
其上方的层（可能是应用程序）。
当层使用它之前
返回的句柄接收 Vulkan 函数时，层需要解包句柄并将
保存的句柄传递给其下方的层。
这意味着层**必须拦截使用**
**相关对象的每个 Vulkan 函数**，并适当地包装或解包对象。
这包括为所有扩展添加支持，这些扩展具有使用层包装的任何
对象的函数，以及任何加载器-层接口函数，例如
`vk_layerGetPhysicalDeviceProcAddr`。

对象包装层上方的层将看到包装的对象。
包装可调度对象的层必须确保包装结构中的第一个字段是指向 `vk_layer.h` 中定义的调度表的指针。
具体来说，实例包装的可调度对象可能如下所示：

```cpp
struct my_wrapped_instance_obj_ {
    VkLayerInstanceDispatchTable *disp;
    // 层想要添加到此对象的任何数据
};
```
设备包装的可调度对象可能如下所示：
```cpp
struct my_wrapped_instance_obj_ {
    VkLayerDispatchTable *disp;
    // 层想要添加到此对象的任何数据
};
```

包装可调度对象的层必须遵循创建
新可调度对象的指南（如下）。

#### 关于包装的注意事项

通常不鼓励层包装对象，因为
可能与新扩展不兼容。
例如，假设层包装 `VkImage` 对象，并正确包装
和解包所有核心函数的 `VkImage` 对象句柄。
如果创建了一个新扩展，该扩展具有接受 `VkImage` 对象的
函数作为参数，并且如果层不支持这些新函数，则
同时使用层和新扩展的应用程序在调用这些新函数时将具有未定义
的行为（例如，应用程序可能崩溃）。
这是因为较低级别的层和驱动程序不会收到它们生成的句柄
。
相反，它们将收到一个只有包装对象的层才知道的句柄
。

由于可能与不支持的扩展不兼容，
包装对象的层必须检查应用程序正在使用哪些扩展，并在层与不支持的扩展一起使用时采取适当的操作，例如向用户发出警告/错误消息
。

验证层包装对象的原因是为了跟踪每个对象的正确使用
和销毁。
如果与不支持的扩展一起使用，它们会发出验证错误，提醒
用户可能出现未定义的行为。


#### 哈希映射

或者，层可能希望使用哈希映射将数据与
给定对象关联。
映射的键可以是对象。或者，对于
给定级别（例如设备或实例）的可调度对象，层可能希望与
`VkDevice` 或 `VkInstance` 对象关联的数据。
由于给定 `VkInstance` 或
`VkDevice` 有多个可调度对象，`VkDevice` 或 `VkInstance` 对象不是很好的映射键。
相反，层应该使用 `VkDevice`
或 `VkInstance` 内的调度表指针，因为这对于给定的 `VkInstance` 或
`VkDevice` 将是唯一的。


### 创建新的可调度对象

创建可调度对象的层必须特别小心。
请记住，加载器*跳板*代码通常填充新创建对象中的调度表
指针。
因此，如果加载器
*跳板*不会这样做，层必须填充调度表指针。
层（或驱动程序）可能在没有
加载器*跳板*代码的情况下创建可调度对象的常见情况如下：
- 包装可调度对象的层
- 添加创建可调度对象的扩展的层
- 在它们拦截的应用程序函数流中插入额外 Vulkan 函数的层
- 添加创建可调度对象的扩展的驱动程序

Khronos 加载器提供了一个回调，可用于初始化
可调度对象。
回调通过 `pNext` 字段作为扩展结构传递，在
创建实例（`VkInstanceCreateInfo`）或
设备（`VkDeviceCreateInfo`）时。
回调原型分别定义为实例和设备回调
（请参阅 `vk_layer.h`）：

```cpp
VKAPI_ATTR VkResult VKAPI_CALL
   vkSetInstanceLoaderData(
      VkInstance instance,
      void *object);

VKAPI_ATTR VkResult VKAPI_CALL
   vkSetDeviceLoaderData(
      VkDevice device,
      void *object);
```

为了获得这些回调，层必须搜索
`VkInstanceCreateInfo` 和
`VkDeviceCreateInfo` 参数中 "pNext" 字段指向的结构列表，以查找加载器插入的任何回调结构。
重要细节如下：
- 对于 `VkInstanceCreateInfo`，由 "pNext" 指向的回调结构是
`include/vulkan/vk_layer.h` 中定义的 `VkLayerInstanceCreateInfo`。
- `VkInstanceCreateInfo` 参数中 VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO 的 "sType" 字段表示加载器结构。
- 在 `VkLayerInstanceCreateInfo` 内，"function" 字段指示
联合字段 "u" 应如何解释。
- 等于 VK_LOADER_DATA_CALLBACK 的 "function" 表示 "u" 字段将
在 "pfnSetInstanceLoaderData" 中包含回调。
- 对于 `VkDeviceCreateInfo`，由 "pNext" 指向的回调结构是
`include/vulkan/vk_layer.h` 中定义的 `VkLayerDeviceCreateInfo`。
- `VkDeviceCreateInfo` 参数中 VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO 的 "sType" 字段表示加载器结构。
- 在 `VkLayerDeviceCreateInfo` 内，"function" 字段指示联合
字段 "u" 应如何解释。
- 等于 VK_LOADER_DATA_CALLBACK 的 "function" 表示 "u" 字段将
在 "pfnSetDeviceLoaderData" 中包含回调。

或者，如果使用不提供这些
回调的旧加载器，层可以手动初始化新创建的可调度
对象。
为了填充新创建的可调度对象中的调度表指针，层应该复制调度指针，该指针始终是
结构中的第一个条目，从同一级别的现有父对象（实例与
设备）。

例如，如果有一个新创建的 `VkCommandBuffer` 对象，则应该
将 `VkDevice` 对象（`VkCommandBuffer` 对象的父对象）的调度指针复制到新创建的对象中。

### 版本控制和激活交互

关于激活层有几个相互作用的规则，具有
非显而易见的结果。
这不是一个详尽的列表，但应该更好地阐明
加载器在复杂情况下的行为。

* Vulkan 加载器在 1.3.228 及更高版本中将启用隐式层
，无论应用程序在
`VkApplicationInfo::apiVersion` 中指定的 API 版本如何。
以前的加载器版本（1.3.227 及更低版本）曾经有一个要求，即
隐式层的 API 版本必须等于或大于应用程序的 API 版本
，层才能被启用。
更改放宽了隐式层加载要求，因为确定
防止旧层与新应用程序一起运行的感知保护不足以证明它造成的摩擦是合理的。
这是由于旧层不再与新应用程序一起工作
，没有明显的原因，以及旧层必须更新清单
才能与新应用程序一起工作。
层不需要做任何其他事情来使其层再次工作，
这意味着层不需要证明其层与
较新的 API 版本一起工作。
因此，禁用给用户带来了困惑，但没有保护他们免受
可能行为不良的层的影响。

* 如果隐式层是活动元层中的组件，它将忽略其禁用环境变量的设置
。

* 环境 `VK_LAYER_PATH` 仅影响显式层搜索，不影响
隐式层。
在此路径中找到的层被视为显式层，即使它们包含所有
成为隐式层的必需字段。
这意味着它们不会被隐式启用。

* 元层不必是隐式的 - 它们可以是显式的。
不能假设因为存在元层，它就会处于活动状态。

* 覆盖元层的 `blacklisted_layers` 成员将阻止
隐式启用和显式启用的层激活。
应用程序的 `VkInstanceCreateInfo::ppEnabledLayerNames` 中
在黑名单中的任何层都不会被启用。

* 覆盖元层的 `app_keys` 成员将使元层仅应用于
在此列表中找到的应用程序。
如果应用程序键列表中有任何项目，则元层不会为
列表中找到的应用程序之外的任何应用程序启用。

* 覆盖元层的 `override_paths` 成员（如果存在）将
替换加载器用于查找组件层的搜索路径。
如果任何组件层不在覆盖路径中，则不会应用覆盖元
层。
因此，如果覆盖元层想要混合默认和自定义层位置，
覆盖路径必须包含自定义和默认层位置。

* 如果覆盖层既存在又包含 `override_paths`，则在搜索
显式层时，环境变量 `VK_LAYER_PATH` 的路径将被忽略。
例如，当元层覆盖路径和 `VK_LAYER_PATH` 都
存在时，`VK_LAYER_PATH` 中的任何层都不可发现，并且
加载器将找不到它们。


## 层清单文件格式

Khronos 加载器使用清单文件来发现可用的层库
和层。
它不直接查询层的动态库，除了在链接期间。
这是为了减少将恶意层加载到内存中的可能性。
相反，从清单文件中读取详细信息，然后提供
给应用程序以确定应该实际加载哪些层。

以下部分讨论层清单 JSON 文件
格式的详细信息。
JSON 文件本身对命名没有任何要求。
唯一的要求是文件的扩展后缀是 ".json"。

以下是包含单个层的层 JSON 清单文件示例：

```json
{
   "file_format_version" : "1.2.1",
   "layer": {
       "name": "VK_LAYER_LUNARG_overlay",
       "type": "INSTANCE",
       "library_path": "vkOverlayLayer.dll",
       "library_arch" : "64",
       "api_version" : "1.0.5",
       "implementation_version" : "2",
       "description" : "LunarG HUD layer",
       "functions": {
           "vkNegotiateLoaderLayerInterfaceVersion":
               "OverlayLayer_NegotiateLoaderLayerInterfaceVersion"
       },
       "instance_extensions": [
           {
               "name": "VK_EXT_debug_report",
               "spec_version": "1"
           },
           {
               "name": "VK_VENDOR_ext_x",
               "spec_version": "3"
            }
       ],
       "device_extensions": [
           {
               "name": "VK_EXT_debug_marker",
               "spec_version": "1",
               "entrypoints": ["vkCmdDbgMarkerBegin", "vkCmdDbgMarkerEnd"]
           }
       ],
       "enable_environment": {
           "ENABLE_LAYER_OVERLAY_1": "1"
       },
       "disable_environment": {
           "DISABLE_LAYER_OVERLAY_1": ""
       }
   }
}
```

以下是支持每个
清单文件多个层所需的更改片段：
```json
{
   "file_format_version" : "1.0.1",
   "layers": [
      {
           "name": "VK_LAYER_layer_name1",
           "type": "INSTANCE",
           ...
      },
      {
           "name": "VK_LAYER_layer_name2",
           "type": "INSTANCE",
           ...
      }
   ]
}
```

以下是元层清单文件的示例：
```json
{
   "file_format_version" : "1.1.1",
   "layer": {
       "name": "VK_LAYER_META_layer",
       "type": "GLOBAL",
       "api_version" : "1.0.40",
       "implementation_version" : "1",
       "description" : "LunarG Meta-layer example",
       "component_layers": [
           "VK_LAYER_KHRONOS_validation",
           "VK_LAYER_LUNARG_api_dump"
       ]
   }
}
```


<table style="width:100%">
  <tr>
    <th>JSON 节点</th>
    <th>描述和说明</th>
    <th>限制</th>
    <th>父节点</th>
    <th>内省查询</th>
  </tr>
  <tr>
    <td>"api_version"</td>
    <td>层支持的 Vulkan API 的主版本.次版本.补丁版本号。
        它不要求应用程序使用该 API 版本。
        它只是表明层可以支持 Vulkan API
        实例和设备函数，包括该 API 版本。</br>
        例如：1.0.33。
    </td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateInstanceLayerProperties</small></td>
  </tr>
  <tr>
    <td>"app_keys"</td>
    <td>元层应用到的可执行文件的路径列表。
    </td>
    <td><b>仅元层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"blacklisted_layers"</td>
    <td>即使应用程序请求也不应加载的显式层名称列表
        。
    </td>
    <td><b>仅元层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"component_layers"</td>
    <td>指示作为
        元层一部分的组件层名称。
        列出的名称必须是每个组件
        层清单文件 "name" 标签中标识的 "name"（这与传递给 `vkCreateInstance` 命令的层名称相同
        ）。
        所有组件层必须存在于系统上并被
        加载器找到，此元层才能可用和激活。<br/>
        <b>如果定义了 "library_path"，则此字段不得存在</b>。
    </td>
    <td><b>仅元层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"description"</td>
    <td>层及其预期用途的高级描述。</td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateInstanceLayerProperties</small></td>
  </tr>
  <tr>
    <td>"device_extensions"</td>
    <td><b>可选：</b> 包含此层支持的设备扩展名称列表
        。如果一个层支持任何设备扩展，则需要一个 "device\_extensions" 节点，其中包含一个或多个
        元素的数组；否则该节点是可选的。
        数组的每个元素必须具有节点 "name" 和 "spec_version"
        ，它们分别对应于 `VkExtensionProperties` "extensionName" 和
        "specVersion"。
        此外，如果设备扩展添加 Vulkan API
        函数，则设备扩展数组的每个元素必须具有
        节点 "entrypoints"；否则不需要此节点。
        "entrypoint" 节点是由支持的扩展添加的所有入口点名称的数组
        。
    </td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateDeviceExtensionProperties</small></td>
  </tr>
  <tr>
    <td>"disable_environment"</td>
    <td><b>必需：</b> 指示用于禁用
        隐式层的环境变量（当定义为任何非空字符串值时）。<br/>
        在应用程序与隐式层不工作的罕见情况下，
        应用程序可以设置此环境变量（在调用 Vulkan
        函数之前）以"黑名单"该层。
        必须设置此环境变量（不一定设置为任何值）（可能随层的每个变体而变化
        ）。
        如果同时设置了 "enable_environment" 和 "disable_environment" 变量，
        则隐式层被禁用。
    </td>
    <td><b>仅隐式层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"enable_environment"</td>
    <td><b>可选：</b> 指示用于启用
        隐式层的环境变量（当定义为任何非空字符串值时）。<br/>
        必须将此环境变量（可能随层的每个变体而变化
        ）设置为给定值，否则不会加载隐式层。
        这适用于希望启用
        层（或多个层）仅用于它们启动的应用程序的应用程序环境（例如 Steam），并允许
        在应用程序环境之外运行的应用程序不会获得该
        隐式层（或多个隐式层）。
    </td>
    <td><b>仅隐式层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"file_format_version"</td>
    <td>清单格式主版本.次版本.补丁版本号。<br/>
        支持的版本有：1.0.0、1.0.1、1.1.0、1.1.1、1.1.2 和 1.2.0。
    </td>
    <td>无</td>
    <td>无</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"functions"</td>
    <td><b>可选：</b> 此部分可用于标识不同的
        函数名称，供加载器使用以替代标准层接口
        函数。
        如果层为 `vkNegotiateLoaderLayerInterfaceVersion` 使用替代
        名称，则需要 "functions" 节点。
    </td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkGet*ProcAddr</small></td>
  </tr>
  <tr>
    <td>"implementation_version"</td>
    <td>实现的层版本。
        如果层本身有任何重大更改，此数字应该更改，以便
        加载器和/或应用程序可以正确识别它。
    </td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateInstanceLayerProperties</small></td>
  </tr>
  <tr>
    <td>"instance_extensions"</td>
    <td><b>可选：</b> 包含此层支持的实例扩展名称列表
        。
        如果一个层支持任何实例扩展，则需要一个 "instance_extensions" 节点，其中包含一个或多个元素的数组
        ；否则该节点是可选的。
        数组的每个元素必须具有节点 "name" 和 "spec_version"
        ，它们分别对应于 `VkExtensionProperties` "extensionName" 和
        "specVersion"。
    </td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateInstanceExtensionProperties</small></td>
  </tr>
  <tr>
    <td>"layer"</td>
    <td>用于将单个层的信息组合在一起的标识符。
    </td>
    <td>无</td>
    <td>无</td>
    <td><small>vkEnumerateInstanceLayerProperties</small></td>
  </tr>
  <tr>
    <td>"layers"</td>
    <td>用于将多个层的信息组合在一起的标识符。
        这需要最低清单文件格式版本 1.0.1。
    </td>
    <td>无</td>
    <td>无</td>
    <td><small>vkEnumerateInstanceLayerProperties</small></td>
  </tr>
  <tr>
    <td>"library_path"</td>
    <td>指定层共享库文件的文件名、相对路径名或完整路径名
        。
        如果 "library_path" 指定相对路径名，则它相对于
        JSON 清单文件的路径（例如，当应用程序
        提供与应用程序文件的其余部分位于同一文件夹层次结构中的层时）。
        如果 "library_path" 指定文件名，则库必须位于
        系统的共享对象搜索路径中。
        关于层共享库文件的名称没有规则，
        除了它应该以适当的后缀结尾（Windows 上为 ".DLL"，
        Linux 上为 ".so"，macOS 上为 ".dylib"）。<br/>
        <b>如果定义了 "component_layers"，则此字段不得存在</b>。
    </td>
    <td><b>不适用于元层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"library_arch"</td>
    <td>可选字段，指定与
        "library_path" 关联的二进制文件的架构。<br />
        允许加载器快速确定层的架构
        是否与正在运行的应用程序的架构匹配。<br />
        唯一有效的值是 "32" 和 "64"。</td>
    <td><small>N/A</small></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"name"</td>
    <td>用于向应用程序唯一标识此层的字符串。</td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateInstanceLayerProperties</small></td>
  </tr>
  <tr>
    <td>"override_paths"</td>
    <td>将用作组件层搜索位置的路径列表
        。
    </td>
    <td><b>仅元层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td>"pre_instance_functions"</td>
    <td><b>可选：</b> 指示层希望
        拦截哪些函数，这些函数不需要已创建实例。
        这应该是一个对象，其中要拦截的每个函数都
        定义为字符串条目，其中键是 Vulkan 函数名称，
        值是层动态
        库中拦截函数的名称。
        在层清单版本 1.1.2 及更高版本中可用。<br/>
        有关更多信息，请参阅 <a href="#实例前函数">实例前函数</a>。
    </td>
    <td><b>仅隐式层</b></td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerateInstance*Properties</small></td>
  </tr>
  <tr>
    <td>"type"</td>
    <td>此字段指示层的类型。值可以是：GLOBAL 或
        INSTANCE。<br/>
        <b> 注意： </b> 在弃用之前，"type" 节点用于
        指示在哪个层链（或多个层链）上激活层：实例、
        设备或两者。
        不同的实例和设备层已弃用；现在只有
        实例层。
        最初，允许的值是 "INSTANCE"、"GLOBAL" 和 "DEVICE"。
        但现在加载器会跳过 "DEVICE" 层，就好像它们
        没有被找到一样。
    </td>
    <td>无</td>
    <td>"layer"/"layers"</td>
    <td><small>vkEnumerate*LayerProperties</small></td>
  </tr>
</table>

### 层清单文件版本历史

当前支持的最高层清单文件格式是 1.2.0。
有关每个版本的信息在以下子节中详细说明：

### 层清单文件版本 1.2.1

添加了 "library\_arch" 字段到层清单，以允许加载器
快速确定层是否与当前运行的应用程序的架构匹配
。

#### 层清单文件版本 1.2.0

能够定义由
[层清单模式](https://github.com/LunarG/VulkanTools/blob/main/vkconfig_core/layers/layers_schema.json) 定义的层设置。

能够简要记录层，这要归功于以下字段：
 * "introduction"：一段中层的用途介绍。
 * "url"：层主页的链接。
 * "platforms"：层支持的平台列表
 * "status"：层的生命周期：Alpha、Beta、Stable 或 Deprecated

进行这些更改是为了使第三方层能够在其功能中公开其功能
[Vulkan Configurator](https://github.com/LunarG/VulkanTools/blob/main/vkconfig/README.md)
或其他工具。

#### 层清单文件版本 1.1.2

版本 1.1.2 引入了层拦截没有实例的函数调用的能力
。

#### 层清单文件版本 1.1.1

添加了定义自定义元层的能力。
为了支持元层，添加了 "component_layers" 部分，并且
当
"component_layers" 部分存在时，移除了 "library_path" 部分必须存在的要求。

#### 层清单文件版本 1.1.0

层清单文件版本 1.1.0 与
加载器/层接口版本 2 公开的更改相关。
  1. 在 "functions" 部分重命名 "vkGetInstanceProcAddr" 已弃用
，因为加载器不再需要直接查询层关于
"vkGetInstanceProcAddr"。
它现在在层协商期间返回，因此此字段将被
忽略。
  2. 在 "functions" 部分重命名 "vkGetDeviceProcAddr" 已弃用
，因为加载器不再需要直接查询层关于
"vkGetDeviceProcAddr"。
它现在也在层协商期间返回，因此此字段将被
忽略。
  3. 重命名 "vkNegotiateLoaderLayerInterfaceVersion" 函数正在
添加到 "functions" 部分，因为这是现在加载器需要
使用特定于操作系统的调用查询的唯一函数。
      - 注意：这是一个可选字段，与前两个字段一样，仅
在层因某种原因需要更改函数名称时才需要。

如果任何
列出的函数的名称没有更改，则不需要更新层清单文件。

#### 层清单文件版本 1.0.1

添加了使用 "layers" 数组定义多个层的能力。
此 JSON 数组字段可以在定义单个层或多个
层时使用。
"layer" 字段仍然存在，对于单个层定义有效。

#### 层清单文件版本 1.0.0

层清单文件的初始版本指定了层 JSON 文件的基本格式和
字段。
1.0.0 文件格式的字段包括：
 * "file\_format\_version"
 * "layer"
 * "name"
 * "type"
 * "library\_path"
 * "api\_version"
 * "implementation\_version"
 * "description"
 * "functions"
 * "instance\_extensions"
 * "device\_extensions"
 * "enable\_environment"
 * "disable\_environment"

也是在这个时候，"type" 字段中的 "DEVICE" 值被弃用。


## 层接口版本

当前的加载器/层接口是版本 2。
以下部分详细说明了各个版本之间的差异。

### 层接口版本 2

引入了
使用
`vkNegotiateLoaderLayerInterfaceVersion` 函数的[加载器和层接口](#层版本协商) 的概念。
此外，它引入了
[层未知物理设备扩展](#层未知物理设备扩展)
的概念以及相关的 `vk_layerGetPhysicalDeviceProcAddr` 函数。
最后，它将清单文件定义更改为 1.1.0。

注意：如果层包装 VkInstance 句柄，对
`vk_layerGetPhysicalDeviceProcAddr` 的支持*不是*可选的，必须实现。

### 层接口版本 1

支持接口版本 1 的层具有以下行为：
 1. `vkGetInstanceProcAddr` 和 `vkGetDeviceProcAddr` 直接导出
 2. 层清单文件能够覆盖
`GetInstanceProcAddr` 和 `GetDeviceProcAddr` 函数的名称。

### 层接口版本 0

支持接口版本 0 的层必须定义并导出这些
内省函数，与任何 Vulkan 函数无关，尽管名称、
签名和其他相似性：

- `vkEnumerateInstanceLayerProperties` 枚举
`层库` 中的所有层。
  - 此函数永远不会失败。
  - 当 `层库` 仅包含一个层时，此函数可能是
   该层的 `vkEnumerateInstanceLayerProperties` 的别名。
- `vkEnumerateInstanceExtensionProperties` 枚举
   `层库` 中层的实例扩展。
  - "pLayerName" 始终是有效的层名称。
  - 此函数永远不会失败。
  - 当 `层库` 仅包含一个层时，此函数可能是
   该层的 `vkEnumerateInstanceExtensionProperties` 的别名。
- `vkEnumerateDeviceLayerProperties` 枚举 `层库` 中层的子集（可以是完整、
   适当或空子集）。
  - "physicalDevice" 始终是 `VK_NULL_HANDLE`。
  - 此函数永远不会失败。
  - 如果此函数未枚举层，它将不会参与
   设备函数拦截。
- `vkEnumerateDeviceExtensionProperties` 枚举 `层库` 中层的设备扩展。
  - "physicalDevice" 始终是 `VK_NULL_HANDLE`。
  - "pLayerName" 始终是有效的层名称。
  - 此函数永远不会失败。

它还必须为库中的每个层定义并导出这些函数一次
：

- `<layerName>GetInstanceProcAddr(instance, pName)` 的行为与
层的 vkGetInstanceProcAddr 完全相同，除了它是导出的。

   当 `层库` 仅包含一个层时，此函数可以
   替代命名为 `vkGetInstanceProcAddr`。

- `<layerName>GetDeviceProcAddr`  的行为与层的
vkGetDeviceProcAddr 完全相同，除了它是导出的。

   当 `层库` 仅包含一个层时，此函数可以
   替代命名为 `vkGetDeviceProcAddr`。

库中包含的所有层必须支持 `vk_layer.h`。
它们不需要实现它们不拦截的函数。
建议它们不要导出任何函数。


## 加载器与层接口策略

本节旨在定义加载器
和层之间预期的正确行为。
本节的大部分内容是对 Vulkan 规范的补充，对于
跨平台维护一致性是必要的。
实际上，大部分语言可以在本文档中找到，但为了
方便起见，在此处进行了总结。
此外，应该有一种方法来识别层中的不良或不符合规范的行为
，并尽快纠正它。
因此，提供了策略编号系统，以唯一的方式清楚地识别每个
策略声明。

最后，基于使加载器高效和高性能的目标，
一些定义正确层行为的策略声明可能
不可测试（因此加载器无法强制执行）。
但是，这不应影响为了向最终用户和开发人员提供
最佳体验而提供的要求。


### 编号格式

加载器/层策略项以前缀 `LLP_` 开头（
加载器/层策略的缩写），后跟基于
策略针对的组件的标识符。
在这种情况下，只有两个可能的组件：
 - 层：策略编号中将包含字符串 `LAYER_`。
 - 加载器：策略编号中将包含字符串 `LOADER_`
   作为策略编号的一部分。


### Android 差异

如前所述，Android 加载器实际上与 Khronos
加载器是分开的。
因此，由于此和其他平台要求，并非所有这些策略
声明都适用于 Android。
每个表还有一个标题为"适用于 Android？"的列
，它指示哪些策略声明适用于仅专注于
Android 支持的层。
有关 Android 加载器的更多信息，请参阅
<a href="https://source.android.com/devices/graphics/implement-vulkan">
Android Vulkan 文档</a>。


### 行为良好的层的要求

<table style="width:100%">
  <tr>
    <th>要求编号</th>
    <th>要求描述</th>
    <th>不符合的后果</th>
    <th>适用于 Android？</th>
    <th>可由加载器强制执行？</th>
    <th>参考部分</th>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_1</b></small></td>
    <td>当层插入到其他符合规范的 Vulkan
        环境中时，它<b>必须</b>仍然产生符合规范的 Vulkan 环境
        ，除非它打算模拟不符合规范的行为（例如设备
        模拟层）。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否<br/>
        对于加载器来说，在层链中找到失败的原因
        不是一项简单的任务。</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_2</b></small></td>
    <td>层<b>不得</b>导致其他层或驱动程序失败、崩溃或
        其他方式行为异常。<br/>
        它<b>不得</b>对其下方的层或驱动程序进行无效调用，也不得依赖其未定义的行为
        。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否<br/>
        对于加载器来说，在层链中找到失败的原因
        不是一项简单的任务。</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_3</b></small></td>
    <td>任何新开发的层<b>应该</b>遵循
        "层约定和规则"部分中定义的命名规则，这些规则也对应于
        Vulkan 风格指南第 3.4 节中定义的命名规则
        "版本、扩展和层命名约定"。
    </td>
    <td>层开发人员可能产生冲突的名称，如果在
        用户平台上存在多个同名层，则会导致意外
        行为。
    </td>
    <td>是</td>
    <td>是<br/>
        不能立即强制执行，因为它会导致某些已发布的层
        停止工作。</td>
    <td><small>
        <a href="https://www.khronos.org/registry/vulkan/specs/1.2/styleguide.html#extensions-naming-conventions">
            Vulkan 风格指南第 3.4 节</a> <br/>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_4</b></small></td>
    <td>层<b>应该</b>导出
        <i>vkNegotiateLoaderLayerInterfaceVersion</i> 入口点以协商
        接口版本。<br/>
        使用接口 2 或更新版本的层<b>必须</b>导出此函数。<br/>
    </td>
    <td>层将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#层版本协商">层版本协商</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_5</b></small></td>
    <td>层<b>必须</b>能够根据规定的
        协商过程与加载器协商支持的加载器/层接口版本。
    </td>
    <td>层将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#加载器和层接口协商">
        接口协商</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_6</b></small></td>
    <td>层<b>必须</b>具有有效的 JSON 清单文件供
        加载器处理，该文件以 ".json" 后缀结尾。
        建议在发布之前根据
        <a href="https://github.com/LunarG/VulkanTools/blob/main/vkconfig_core/layers/layers_schema.json">
        层模式</a> 验证层清单文件。</br>
        <b>唯一的</b>例外是在 Android 上，它通过
        <a href="#层库-api-版本-0">层库 API 版本 0</a>
        部分和
        <a href="#层清单文件格式">层清单文件格式</a>
        表中定义的内省函数确定层
        功能。
    </td>
    <td>层将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#层清单文件的使用">清单文件的使用</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_7</b></small></td>
    <td>如果层是元层，其清单文件中的每个组件层
        <b>必须</b>存在于系统上。
    </td>
    <td>层将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#元层">元层</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_8</b></small></td>
    <td>如果层是元层，其清单文件中的每个组件层
        <b>必须</b>报告与元层相同或更新的 Vulkan API 主版本和次版本
        。
    </td>
    <td>层将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#元层">元层</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_9</b></small></td>
    <td>作为隐式层安装的层<b>必须</b>定义禁用
        环境变量，以便可以全局禁用它。
    </td>
    <td>如果层未定义环境
        变量，则不会被加载。
    </td>
    <td>是</td>
    <td>是</td>
    <td><small>
        <a href="#层清单文件格式">清单文件格式</a>，请参阅
        "disable_environment" 变量</small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_10</b></small></td>
    <td>如果层包装单个对象句柄，它<b>必须</b>在将句柄向下传递到链中的下一层时解包这些
        句柄。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否</td>
    <td><small>
      <a href="#关于包装的注意事项">关于包装的注意事项</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_11</b></small></td>
    <td>与驱动程序一起提供的任何层<b>必须</b>针对
        与相应驱动程序的符合性进行验证。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否</td>
    <td><small>
        <a href="https://github.com/KhronosGroup/VK-GL-CTS/blob/main/external/openglcts/README.md">
        Vulkan CTS 文档</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_12</b></small></td>
    <td> 在 <i>vkCreateInstance</i> 期间，层<b>必须</b>适当地处理
         <i>VkLayerInstanceCreateInfo</i> 链链接。<br/>
         这包括获取下一层的 <i>vkGetInstanceProcAddr</i>
         函数以构建调度表，以及更新
         <i>VkLayerInstanceCreateInfo</i> 链链接以指向链中的下一个
         结构，用于下一层，然后再向下调用
         下一层的 <i>vkCreateInstance</i> 函数。<br/>
         此类用法的示例在
         <a href=#createinstance-示例代码>CreateInstance 示例代码
         </a> 部分中详细显示。
    </td>
    <td>行为将导致崩溃或损坏，因为任何后续
        层将访问不正确的内容。</td>
    <td>是</td>
    <td>否<br/>
        使用当前的加载器/层设计，加载器很难
        在不添加可能影响
        性能的额外开销的情况下诊断此问题。<br/>
        这是因为加载器一次调用所有层，并且没有
        <i>pNext</i> 链内容的中间状态数据。
        这可以在将来完成，但需要重新设计层
        初始化过程。
    </td>
    <td><small>
        <a href="#层调度初始化">
           层调度初始化</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_13</b></small></td>
    <td> 在 <i>vkCreateDevice</i> 期间，层<b>必须</b>适当地处理
         <i>VkLayerDeviceCreateInfo</i> 链链接。<br/>
         这包括更新 <i>VkLayerDeviceCreateInfo</i> 链链接以
         指向链中的下一个结构，用于下一层，然后再
         向下调用下一层的 <i>vkCreateDevice</i> 函数。<br/>
         此类用法的示例在
         <a href="#createdevice-示例代码">CreateDevice 示例代码
         </a> 部分中详细显示。
    </td>
    <td>行为将导致崩溃或损坏，因为任何后续
        层将访问不正确的内容。</td>
    <td>是</td>
    <td>否<br/>
        使用当前的加载器/层设计，加载器很难
        在不添加可能影响
        性能的额外开销的情况下诊断此问题。</td>
    <td><small>
        <a href="#层调度初始化">
           层调度初始化</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_14</b></small></td>
    <td>层<b>应该</b>在提供时使用应用程序提供的内存分配器
        函数，以便应用程序可以跟踪
        分配的内存。
    </td>
    <td>分配器函数可能用于限制
        或跟踪 Vulkan 组件使用的内存。
        因此，如果层忽略这些分配器，它可能导致
        未定义的行为，可能包括崩溃或损坏。
    </td>
    <td>是</td>
    <td>否</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_15</b></small></td>
    <td>层<b>必须</b>在
        <i>pLayerName</i> 引用它自身时，在调用 <i>vkEnumerateInstanceExtensionProperties</i> 期间仅枚举其自己的扩展属性。<br/>
        否则，它<b>必须</b>返回 <i>VK_ERROR_LAYER_NOT_PRESENT</i>，
        包括当 <i>pLayerName</i> 是 <b>NULL</b> 时。
    </td>
    <td>加载器可能会对特定层中存在哪些支持感到困惑，这将导致未定义的行为，可能
        包括崩溃或损坏。
    </td>
    <td>是</td>
    <td>否</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_16</b></small></td>
    <td>层<b>必须</b>在
        <i>pLayerName</i> 引用它自身时，在调用 <i>vkEnumerateDeviceExtensionProperties</i> 期间仅枚举其自己的扩展属性。<br/>
        否则，它<b>必须</b>忽略调用，除了将其向下传递
        标准调用链。
    </td>
    <td>加载器可能会对特定层中存在哪些支持感到困惑，这将导致未定义的行为，可能
        包括崩溃或损坏。
    </td>
    <td>是</td>
    <td>否</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_17</b></small></td>
    <td>层的 <i>vkCreateInstance</i> <b>不得</b>为
        无法识别的扩展名称生成错误，因为扩展可能由
        较低层或驱动程序实现。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>是</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_18</b></small></td>
    <td>层<b>必须</b>从 <i>vkGetInstanceProcAddr</i>
        或 <i>vkGetDeviceProcAddr</i> 为它不支持的入口点
        或未正确启用的入口点（例如，未启用某些入口点关联的扩展
        应该导致
        <i>vkGetInstanceProcAddr</i> 在请求
        它们时返回 <b>NULL</b>）返回 <b>NULL</b>。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否<br/>
        使用当前的加载器/层设计，加载器很难
        在不添加可能影响
        性能的额外开销的情况下确定这一点。</td>
    <td><small>
        <a href="#层约定和规则">层约定和规则</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_19</b></small></td>
    <td>如果层创建可调度对象，无论是因为它
        包装对象还是实现加载器或
        底层驱动程序不支持的扩展，它<b>必须</b>为所有创建的可调度对象适当地创建调度
        表。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否</td>
    <td><small>
        <a href="#创建新的可调度对象">
          创建新的可调度对象</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_20</b></small></td>
    <td>层<b>必须</b>在卸载时删除所有清单文件和对这些
        文件的引用（即 Windows 上的注册表项）。
        <br/>
        同样，在更新层文件时，旧文件<b>必须</b>全部
        更新或删除。
    </td>
    <td>加载器忽略重复尝试加载同一清单文件，
        但如果旧文件指向不正确的库，它将
        导致未定义的行为，可能包括崩溃或损坏。
    </td>
    <td>否</td>
    <td>否<br/>
        加载器不知道哪些层文件是新、旧还是不正确的。
        任何类型的层文件验证都会很快变得非常复杂
        ，因为它需要加载器维护一个内部数据库
        ，根据层名称、版本、
        目标平台和其他可能的标准跟踪行为不良的层。
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_21</b></small></td>
    <td>在 <i>vkCreateInstance</i> 期间，层<b>不得</b>在向下调用到较低
        层之前修改
        <i>pInstance</i> 指针。<br/>
        这是因为加载器在此指针中传递信息，这些信息是
        加载器终止符
        函数中的初始化代码所必需的。<br/>
        相反，如果层正在覆盖 <i>pInstance</i> 指针，它
        <b>必须</b>仅在向下调用较低层返回后才这样做。
    </td>
    <td>加载器可能会崩溃。</td>
    <td>否</td>
    <td>是</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td><small><b>LLP_LAYER_22</b></small></td>
    <td>在 <i>vkCreateDevice</i> 期间，层<b>不得</b>在向下调用到较低
        层之前修改
        <i>pDevice</i> 指针。<br/>
        这是因为加载器在此指针中传递信息，这些信息是
        加载器终止符
        函数中的初始化代码所必需的。<br/>
        相反，如果层正在覆盖 <i>pDevice</i> 指针，它
        <b>必须</b>仅在向下调用较低层返回后才这样做。
    </td>
    <td>加载器可能会崩溃。</td>
    <td>否</td>
    <td>是</td>
    <td><small>N/A</small></td>
  </tr>
</table>


### 行为良好的加载器的要求

<table style="width:100%">
  <tr>
    <th>要求编号</th>
    <th>要求描述</th>
    <th>不符合的后果</th>
    <th>适用于 Android？</th>
    <th>参考部分</th>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_1</b></small></td>
    <td>加载器<b>必须</b>支持 Vulkan 层。</td>
    <td>用户将无法访问 Vulkan 生态系统的关键部分
        ，例如验证层、GfxReconstruct 或 RenderDoc。</td>
    <td>是</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_2</b></small></td>
    <td>加载器<b>必须</b>支持一种机制来在一个或多个
        非标准位置加载层。<br/>
        这是为了允许应用程序/引擎特定的层以及
        评估开发中的层，而无需全局安装。
    </td>
    <td>某些工具和驱动程序开发人员使用 Vulkan 加载器将更加困难。
    </td>
    <td>否</td>
    <td><small><a href="#层发现">层发现</a></small></td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_3</b></small></td>
    <td>加载器<b>必须</b>过滤掉各种
        启用列表中的重复层名称，仅保留第一次出现。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small><a href="#层发现">层发现</a></small></td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_4</b></small></td>
    <td>加载器<b>不得</b>加载定义与自身不兼容的
        API 版本的 Vulkan 层。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small><a href="#层发现">层发现</a></small></td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_5</b></small></td>
    <td>加载器<b>必须</b>忽略任何无法协商兼容接口
        版本的层。
    </td>
    <td>加载器将错误地加载层，导致未定义的行为
        ，可能包括崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#加载器和层接口协商">
        接口协商</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_6</b></small></td>
    <td>如果层是隐式的，并且它具有启用环境变量，
        则加载器<b>不得</b>认为层已启用，除非该
        启用环境变量已定义。<br/>
        如果隐式层没有启用环境变量，
        则默认认为它已启用。
    </td>
    <td>某些层可能在非预期时被使用。</td>
    <td>否</td>
    <td><small>
        <a href="#层清单文件格式">清单文件格式</a>，请参阅
        "enable_environment" 变量</small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_7</b></small></td>
    <td>如果隐式层已启用，但已被某些其他
        机制禁用（例如定义层的禁用环境
        变量或通过覆盖层的黑名单机制），
        则加载器<b>不得</b>加载该层。
    </td>
    <td>某些层可能在非预期时被使用。</td>
    <td>否</td>
    <td><small>
        <a href="#层清单文件格式">清单文件格式</a>，请参阅
        "disable_environment" 变量</small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_8</b></small></td>
    <td>加载器<b>必须</b>通过
        <i>VkInstanceCreateInfo</i> 结构的 <i>pNext</i> 字段中的 <i>VkLayerInstanceCreateInfo</i> 结构向每个层传递初始化结构的链表
        。
        这包含设置实例调用
        链所需的信息，包括提供指向下一个链接的
        <i>vkGetInstanceProcAddr</i> 的函数指针。
    </td>
    <td>层将在尝试加载无效数据时崩溃。</td>
    <td>是</td>
    <td><small>
        <a href="#层调度初始化">
           层调度初始化</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_9</b></small></td>
    <td>加载器<b>必须</b>通过
        <i>VkDeviceCreateInfo</i> 结构的 <i>pNext</i> 字段中的 <i>VkLayerDeviceCreateInfo</i> 结构向每个层传递初始化结构的链表
        。
        这包含设置设备调用链所需的信息
        ，包括提供指向下一个链接的
        <i>vkGetDeviceProcAddr</i> 的函数指针。
    <td>层将在尝试加载无效数据时崩溃。</td>
    <td>是</td>
    <td><small>
        <a href="#层调度初始化">
           层调度初始化</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_10</b></small></td>
    <td>加载器<b>必须</b>验证所有元层都包含有效的
        组件层，加载器可以在系统上找到这些组件层，并且这些组件层也
        报告与元层本身相同的 Vulkan API 版本，然后才能
        加载元层。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#元层">元层</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_11</b></small></td>
    <td>如果存在覆盖元层，加载器<b>必须</b>在所有其他隐式层都已
        添加到调用链之后加载它
        和相应的组件层。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#覆盖元层">覆盖元层</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_12</b></small></td>
    <td>如果存在覆盖元层并且有要
        删除的层的黑名单，加载器<b>必须</b>禁用黑名单中列出的所有层。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#覆盖元层">覆盖元层</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LLP_LOADER_13</b></small></td>
    <td>加载器<b>必须</b>在运行
        提升（管理员/超级用户）应用程序时，不从用户定义的路径加载（包括
        使用 <i>VK_LAYER_PATH</i>、<i>VK_ADD_LAYER_PATH</i>、<i>VK_IMPLICIT_LAYER_PATH</i>
        或 <i>VK_ADD_IMPLICIT_LAYER_PATH</i> 环境变量）。<br/>
        <b>这是出于安全原因。</b>
    </td>
    <td>行为未定义，可能导致计算机安全漏洞、
        崩溃或损坏。
    </td>
    <td>否</td>
    <td><small><a href="#层发现">层发现</a></small></td>
  </tr>
</table>

<br/>

[返回顶级 LoaderInterfaceArchitecture.md 文件。](LoaderInterfaceArchitecture.md)
