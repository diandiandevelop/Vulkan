<!-- markdownlint-disable MD041 -->
[![Khronos Vulkan][1]][2]

[1]: images/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

# 驱动程序与 Vulkan 加载器的接口 <!-- omit from toc -->
[![Creative Commons][3]][4]

<!-- Copyright &copy; 2015-2023 LunarG, Inc. -->

[3]: images/Creative_Commons_88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/


## 目录 <!-- omit from toc -->

- [概述](#概述)
- [驱动程序发现](#驱动程序发现)
  - [覆盖默认驱动程序发现](#覆盖默认驱动程序发现)
  - [附加驱动程序发现](#附加驱动程序发现)
  - [驱动程序过滤](#驱动程序过滤)
    - [驱动程序选择过滤](#驱动程序选择过滤)
    - [驱动程序禁用过滤](#驱动程序禁用过滤)
  - [提升权限的例外情况](#提升权限的例外情况)
    - [示例](#示例)
      - [在 Windows 上](#在-windows-上)
      - [在 Linux 上](#在-linux-上)
      - [在 macOS 上](#在-macos-上)
  - [驱动程序清单文件的使用](#驱动程序清单文件的使用)
  - [Windows 上的驱动程序发现](#windows-上的驱动程序发现)
  - [Linux 上的驱动程序发现](#linux-上的驱动程序发现)
    - [Linux 驱动程序搜索路径示例](#linux-驱动程序搜索路径示例)
  - [Fuchsia 上的驱动程序发现](#fuchsia-上的驱动程序发现)
  - [macOS 上的驱动程序发现](#macos-上的驱动程序发现)
    - [macOS 驱动程序搜索路径示例](#macos-驱动程序搜索路径示例)
    - [驱动程序调试的附加设置](#驱动程序调试的附加设置)
  - [使用 `VK_LUNARG_direct_driver_loading` 扩展进行驱动程序发现](#使用-vk_lunarg_direct_driver_loading-扩展进行驱动程序发现)
    - [如何使用 `VK_LUNARG_direct_driver_loading`](#如何使用-vk_lunarg_direct_driver_loading)
    - [与其他驱动程序发现机制的交互](#与其他驱动程序发现机制的交互)
    - [`VK_LUNARG_direct_driver_loading` 的限制](#vk_lunarg_direct_driver_loading-的限制)
  - [使用预生产 ICD 或软件驱动程序](#使用预生产-icd-或软件驱动程序)
  - [Android 上的驱动程序发现](#android-上的驱动程序发现)
- [驱动程序清单文件格式](#驱动程序清单文件格式)
  - [驱动程序清单文件版本](#驱动程序清单文件版本)
    - [驱动程序清单文件版本 1.0.0](#驱动程序清单文件版本-100)
    - [驱动程序清单文件版本 1.0.1](#驱动程序清单文件版本-101)
- [驱动程序 Vulkan 入口点发现](#驱动程序-vulkan-入口点发现)
- [驱动程序 API 版本](#驱动程序-api-版本)
- [混合驱动程序实例扩展支持](#混合驱动程序实例扩展支持)
  - [过滤实例扩展名称](#过滤实例扩展名称)
  - [加载器实例扩展模拟支持](#加载器实例扩展模拟支持)
- [驱动程序未知物理设备扩展](#驱动程序未知物理设备扩展)
  - [添加 `vk_icdGetPhysicalDeviceProcAddr` 的原因](#添加-vk_icdgetphysicaldeviceprocaddr-的原因)
- [物理设备排序](#物理设备排序)
- [驱动程序可调度对象创建](#驱动程序可调度对象创建)
- [WSI 扩展中处理 KHR Surface 对象](#wsi-扩展中处理-khr-surface-对象)
- [加载器与驱动程序接口协商](#加载器与驱动程序接口协商)
  - [Windows、Linux 和 macOS 驱动程序协商](#windowslinux-和-macos-驱动程序协商)
    - [加载器与驱动程序之间的版本协商](#加载器与驱动程序之间的版本协商)
    - [与旧版驱动程序或加载器的接口](#与旧版驱动程序或加载器的接口)
    - [加载器与驱动程序接口版本 7 要求](#加载器与驱动程序接口版本-7-要求)
    - [加载器与驱动程序接口版本 6 要求](#加载器与驱动程序接口版本-6-要求)
    - [加载器与驱动程序接口版本 5 要求](#加载器与驱动程序接口版本-5-要求)
    - [加载器与驱动程序接口版本 4 要求](#加载器与驱动程序接口版本-4-要求)
    - [加载器与驱动程序接口版本 3 要求](#加载器与驱动程序接口版本-3-要求)
    - [加载器与驱动程序接口版本 2 要求](#加载器与驱动程序接口版本-2-要求)
    - [加载器与驱动程序接口版本 1 要求](#加载器与驱动程序接口版本-1-要求)
    - [加载器与驱动程序接口版本 0 要求](#加载器与驱动程序接口版本-0-要求)
    - [附加接口说明：](#附加接口说明)
  - [Android 驱动程序协商](#android-驱动程序协商)
- [加载器对 VK_KHR_portability_enumeration 的实现](#加载器对-vk_khr_portability_enumeration-的实现)
- [加载器与驱动程序策略](#加载器与驱动程序策略)
  - [编号格式](#编号格式)
  - [Android 差异](#android-差异)
  - [行为良好的驱动程序的要求](#行为良好的驱动程序的要求)
    - [已移除的驱动程序策略](#已移除的驱动程序策略)
  - [行为良好的加载器的要求](#行为良好的加载器的要求)


## 概述

这是从驱动程序角度与 Vulkan 加载器协作的视图。
有关加载器所有部分的完整概述，请参阅
[LoaderInterfaceArchitecture.md](LoaderInterfaceArchitecture.md) 文件。

**注意：** 虽然许多接口仍使用 "icd" 子字符串来
标识与驱动程序相关的各种行为，但这纯粹是
历史原因，不应表示实现代码通过
传统的 ICD 接口来实现。
诚然，迄今为止大多数驱动程序都是 ICD 驱动程序
针对特定的 GPU 硬件。

## 驱动程序发现

Vulkan 允许多个驱动程序，每个驱动程序支持一个或多个设备
（由 Vulkan `VkPhysicalDevice` 对象表示）共同使用。
加载器负责发现系统上可用的 Vulkan 驱动程序。
给定可用驱动程序列表，加载器可以枚举所有
可用于应用程序的物理设备，并将此信息返回给
应用程序。
加载器在系统上发现可用驱动程序的过程
是平台相关的。
Windows、Linux、Android 和 macOS 驱动程序发现的详细信息如下
所示。

### 覆盖默认驱动程序发现

有时开发者可能希望强制加载器使用特定的
驱动程序。
这可能出于多种原因，包括使用测试版驱动程序，或强制
加载器跳过有问题的驱动程序。
为了支持这一点，可以通过 `VK_DRIVER_FILES` 或较旧的 `VK_ICD_FILENAMES`
环境变量强制加载器查看特定的
驱动程序。
这两个环境变量的行为相同，但 `VK_ICD_FILENAMES`
应被视为已弃用。
如果同时存在 `VK_DRIVER_FILES` 和 `VK_ICD_FILENAMES` 环境变量，
则使用较新的 `VK_DRIVER_FILES`，而
`VK_ICD_FILENAMES` 中的值将被忽略。

`VK_DRIVER_FILES` 环境变量是驱动程序清单
文件路径的列表，包含驱动程序 JSON 清单文件的完整路径，和/或包含
驱动程序清单文件的文件夹路径。
在 Linux 和 macOS 上，此列表以冒号分隔，在
Windows 上以分号分隔。
通常，`VK_DRIVER_FILES` 只包含一个驱动程序的单个信息文件的完整路径名。
只有在需要多个驱动程序时才使用分隔符（冒号或分号）。

### 附加驱动程序发现

有时开发者可能希望强制加载器使用特定的
驱动程序，同时保留标准驱动程序（不替换标准
搜索路径）。
`VK_ADD_DRIVER_FILES` 环境变量可用于添加
驱动程序清单文件列表，包含驱动程序 JSON 清单
文件的完整路径，和/或包含驱动程序清单文件的文件夹路径。
在 Linux 和 macOS 上，此列表以冒号分隔，在
Windows 上以分号分隔。
它将在标准驱动程序搜索文件之前添加。
如果存在 `VK_DRIVER_FILES` 或 `VK_ICD_FILENAMES`，则
加载器不会使用 `VK_ADD_DRIVER_FILES`，任何值都将被
忽略。

### 驱动程序过滤

**注意：** 此功能仅在构建时使用版本
1.3.234 的 Vulkan 头文件的加载器中可用。

加载器支持过滤环境变量，可以强制选择和
禁用已知驱动程序。
已知驱动程序清单是加载器已经找到的文件
，考虑了默认搜索路径和其他环境变量（如
`VK_ICD_FILENAMES` 或 `VK_ADD_DRIVER_FILES`）。

过滤变量将与驱动程序的清单文件名进行比较。

过滤器还必须遵循
[过滤环境变量行为](LoaderInterfaceArchitecture.md#filter-environment-variable-behaviors)
部分中定义的行为，该部分位于 [LoaderLayerInterface](LoaderLayerInterface.md) 文档中。

#### 驱动程序选择过滤

驱动程序选择环境变量 `VK_LOADER_DRIVERS_SELECT` 是一个
逗号分隔的 glob 列表，用于在已知驱动程序中搜索。

如果使用 `VK_LOADER_DRIVERS_SELECT` 过滤器时未选择驱动程序，
并且加载器日志设置为发出警告或驱动程序消息，则会
为每个被忽略的驱动程序显示一条消息。
此消息将如下所示：

```
[Vulkan Loader] WARNING | DRIVER: Driver "intel_icd.x86_64.json" ignored because not selected by env var 'VK_LOADER_DRIVERS_SELECT'
```

如果未找到清单文件名与任何提供的
glob 匹配的驱动程序，则不会启用任何驱动程序，这可能导致
运行的任何 Vulkan 应用程序失败。

#### 驱动程序禁用过滤

驱动程序禁用环境变量 `VK_LOADER_DRIVERS_DISABLE` 是一个
逗号分隔的 glob 列表，用于在已知驱动程序中搜索。

当使用 `VK_LOADER_DRIVERS_DISABLE` 过滤器禁用驱动程序时，如果
加载器日志设置为发出警告或驱动程序消息，则会
为每个被强制禁用的驱动程序显示一条消息。
此消息将如下所示：

```
[Vulkan Loader] WARNING | DRIVER: Driver "radeon_icd.x86_64.json" ignored because it was disabled by env var 'VK_LOADER_DRIVERS_DISABLE'
```

如果未找到清单文件名与任何提供的
glob 匹配的驱动程序，则不会禁用任何驱动程序。

### 提升权限的例外情况

出于安全原因，如果以提升权限运行 Vulkan 应用程序，
`VK_ICD_FILENAMES`、`VK_DRIVER_FILES` 和
`VK_ADD_DRIVER_FILES` 都将被忽略。
这是因为它们可能会将新的库插入到可执行进程中，而这些库
通常不会被加载器找到。
因此，这些环境变量只能用于
不使用提升权限的应用程序。

有关更多信息，请参阅
顶级 [LoaderInterfaceArchitecture.md](LoaderInterfaceArchitecture.md) 文档中的
[提升权限注意事项](LoaderInterfaceArchitecture.md#elevated-privilege-caveats)。

#### 示例

为了使用此设置，只需将其设置为正确分隔的
驱动程序清单文件列表。
在这种情况下，请提供这些文件的全局路径以减少问题。

例如：

##### 在 Windows 上

```
set VK_DRIVER_FILES=\windows\system32\nv-vk64.json
```

这是在 Windows 上使用 `VK_DRIVER_FILES` 覆盖的示例，用于
指向 Nvidia Vulkan 驱动程序的清单文件。

```
set VK_ADD_DRIVER_FILES=\windows\system32\nv-vk64.json
```

这是在 Windows 上使用 `VK_ADD_DRIVER_FILES` 的示例，用于
指向 Nvidia Vulkan 驱动程序的清单文件，该文件将在所有其他驱动程序之前
首先加载。

##### 在 Linux 上

```
export VK_DRIVER_FILES=/home/user/dev/mesa/share/vulkan/icd.d/intel_icd.x86_64.json
```

这是在 Linux 上使用 `VK_DRIVER_FILES` 覆盖的示例，用于
指向 Intel Mesa 驱动程序的清单文件。

```
export VK_ADD_DRIVER_FILES=/home/user/dev/mesa/share/vulkan/icd.d/intel_icd.x86_64.json
```

这是在 Linux 上使用 `VK_ADD_DRIVER_FILES` 的示例，用于
指向 Intel Mesa 驱动程序的清单文件，该文件将在所有其他驱动程序之前
首先加载。

##### 在 macOS 上

```
export VK_DRIVER_FILES=/home/user/MoltenVK/Package/Latest/MoltenVK/macOS/MoltenVK_icd.json
```

这是在 macOS 上使用 `VK_DRIVER_FILES` 覆盖的示例，用于
指向包含 MoltenVK 驱动程序的 MoltenVK GitHub 存储库的安装和构建。

有关更多详细信息，请参阅
[LoaderInterfaceArchitecture.md 文档](LoaderInterfaceArchitecture.md)中的
[调试环境变量表](LoaderInterfaceArchitecture.md#table-of-debug-environment-variables)


### 驱动程序清单文件的使用

与层一样，在 Windows、Linux 和 macOS 系统上，使用 JSON 格式的清单
文件来存储驱动程序信息。
为了找到系统安装的驱动程序，Vulkan 加载器将读取 JSON
文件以识别每个驱动程序的名称和属性。
请注意，驱动程序清单文件比相应的
层清单文件简单得多。

有关更多详细信息，请参阅
[当前驱动程序清单文件格式](#驱动程序清单文件格式)
部分。


### Windows 上的驱动程序发现

为了找到可用的驱动程序（包括已安装的 ICD），
加载器会扫描特定于显示适配器和所有软件
组件的注册表项，这些组件与这些适配器关联，以查找 JSON 清单
文件的位置。
这些键位于驱动程序安装期间创建设备键中，
包含基本设置的配置信息，包括 OpenGL 和
Direct3D 位置。

设备适配器和软件组件键路径将首先通过
枚举 DXGI 适配器获得。
如果失败，它将使用 PnP 配置管理器 API。
`000X` 键将是一个编号键，其中每个设备被分配不同的
编号。

```
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Adapter GUID}\000X\VulkanDriverName
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{SoftwareComponent GUID}\000X\VulkanDriverName
```

此外，在 64 位系统上可能还有另一组注册表值，
如下所列。
这些值记录 64 位操作系统上 32 位层的位置，
与 Windows-on-Windows 功能相同。

```
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{Adapter GUID}\000X\VulkanDriverNameWow
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{SoftwareComponent GUID}\000X\VulkanDriverNameWow
```

如果上述任何值存在且类型为 `REG_SZ`，加载器将打开
由键值指定的 JSON 清单文件。
每个值必须是 JSON 清单文件的完整绝对路径。
值也可以是 `REG_MULTI_SZ` 类型，在这种情况下，该值将被
解释为 JSON 清单文件路径的列表。

此外，Vulkan 加载器将扫描以下 Windows
注册表项中的值：

```
HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\Drivers
```

对于 64 位 Windows 上的 32 位应用程序，加载器扫描 32 位
注册表位置：

```
HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Khronos\Vulkan\Drivers
```

这些位置中的每个驱动程序都应作为 DWORD 给出，值为 0，其中
值的名称是 JSON 清单文件的完整路径。
Vulkan 加载器将尝试打开每个清单文件以获取
驱动程序共享库（".dll"）文件的信息。

例如，假设注册表包含以下数据：

```
[HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\Drivers\]

"C:\vendor a\vk_vendor_a.json"=dword:00000000
"C:\windows\system32\vendor_b_vk.json"=dword:00000001
"C:\windows\system32\vendor_c_icd.json"=dword:00000000
```

在这种情况下，加载器将遍历每个条目，并检查值。
如果值为 0，则加载器将尝试加载该文件。
在这种情况下，加载器将打开第一个和最后一个列表，但不打开
中间的那个。
这是因为 vendor_b_vk.json 的值为 1 会禁用该驱动程序。

此外，Vulkan 加载器将扫描系统以查找已知的 Windows
AppX/MSIX 包。
如果找到包，加载器将扫描此已安装
包的根目录以查找 JSON 清单文件。目前，唯一已知的包是
Microsoft 的
[OpenCL™、OpenGL® 和 Vulkan® 兼容包](https://apps.microsoft.com/store/detail/9NQPSL29BFFF?hl=en-us&gl=US)。

Vulkan 加载器将打开找到的每个已启用的清单文件，以获取
驱动程序共享库（".DLL"）文件的名称或路径名。

驱动程序应尽可能使用来自 PnP 配置
管理器的注册表位置。
通常，这对驱动程序最重要，该位置清楚地
将驱动程序与给定设备关联。
`SOFTWARE\Khronos\Vulkan\Drivers` 位置是查找
驱动程序的较旧方法，但它是基于软件的驱动程序的主要位置。

有关更多详细信息，请参阅
[驱动程序清单文件格式](#驱动程序清单文件格式)
部分。


### Linux 上的驱动程序发现

在 Linux 上，Vulkan 加载器将使用
环境变量或相应的回退值扫描驱动程序清单文件，如果相应的
环境变量未定义：

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
        环境变量是非恶意的并不安全。<br/>
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
    <td>编译时选项，设置为可能从非 Linux 发行版提供的包安装的驱动程序位置。
    </td>
  </tr>
  <tr>
    <td>3</td>
    <td>EXTRASYSCONFDIR</td>
    <td>/etc</td>
    <td>编译时选项，设置为可能从非 Linux 发行版提供的包安装的驱动程序位置。
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
        环境变量是非恶意的并不安全。<br/>
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
然后加载器选择每个路径，并将 "/vulkan/icd.d" 后缀应用到
每个路径，并在该特定文件夹中查找清单文件。

Vulkan 加载器将打开找到的每个清单文件，以获取
驱动程序共享库（".so"）文件的名称或路径名。

**注意** 虽然搜索清单文件的文件夹顺序是明确定义的，
但加载器在每个目录中读取内容的顺序是
[由于 readdir 的行为而随机](https://www.ibm.com/support/pages/order-directory-contents-returned-calls-readdir)。

有关更多详细信息，请参阅
[驱动程序清单文件格式](#驱动程序清单文件格式)
部分。

同样重要的是要注意，虽然 `VK_DRIVER_FILES` 会将加载器指向
查找清单文件，但它不能保证清单中提到的库文件
会立即被找到。
通常，驱动程序清单文件将使用
相对或绝对路径指向库文件。
当使用相对或绝对路径时，加载器通常可以找到
库文件，而无需查询操作系统。
但是，如果库仅按名称列出，加载器可能找不到它，
除非驱动程序安装在将库放置在操作系统
可搜索的默认位置。
如果查找与驱动程序关联的库文件时出现问题，请尝试更新
`LD_LIBRARY_PATH` 环境变量以指向
相应的 `.so` 文件的位置。


#### Linux 驱动程序搜索路径示例

对于虚构用户 "me"，驱动程序清单搜索路径可能
如下所示：

```
  /home/me/.config/vulkan/icd.d
  /etc/xdg/vulkan/icd.d
  /usr/local/etc/vulkan/icd.d
  /etc/vulkan/icd.d
  /home/me/.local/share/vulkan/icd.d
  /usr/local/share/vulkan/icd.d
  /usr/share/vulkan/icd.d
```


### Fuchsia 上的驱动程序发现

在 Fuchsia 上，Vulkan 加载器将使用环境
变量或相应的回退值扫描清单文件，如果相应的环境
变量未定义，则与
[Linux](#linux-驱动程序发现) 相同。
**唯一的**区别是 Fuchsia 不允许
*$XDG_DATA_DIRS* 或 *$XDG_HOME_DIRS* 的回退值。


### macOS 上的驱动程序发现

在 macOS 上，Vulkan 加载器将使用
应用程序资源文件夹以及环境变量或
相应的回退值（如果相应的环境变量未定义）扫描驱动程序清单文件。
顺序类似于 Linux 上的搜索路径，但
应用程序的包资源首先被搜索：
`(bundle)/Contents/Resources/`。

如果在应用程序包内找到驱动程序，将忽略系统安装的驱动程序。
这是因为没有标准机制来区分
恰好是重复的驱动程序。
例如，MoltenVK 通常放置在应用程序包内。
如果存在系统安装的 MoltenVK，加载器将加载应用程序包内的
和系统安装的 MoltenVK，这可能导致问题或崩溃。
通过环境变量（如 `VK_DRIVER_FILES`）找到的驱动程序将
被使用，无论是否存在捆绑的驱动程序。


#### macOS 驱动程序搜索路径示例

对于虚构用户 "Me"，驱动程序清单搜索路径可能
如下所示：

```
  <bundle>/Contents/Resources/vulkan/icd.d
  /Users/Me/.config/vulkan/icd.d
  /etc/xdg/vulkan/icd.d
  /usr/local/etc/vulkan/icd.d
  /etc/vulkan/icd.d
  /Users/Me/.local/share/vulkan/icd.d
  /usr/local/share/vulkan/icd.d
  /usr/share/vulkan/icd.d
```


#### 驱动程序调试的附加设置

有时，驱动程序在加载时可能会遇到问题。
一个有用的选项可能是启用 `LD_BIND_NOW` 环境变量
来调试问题。
这强制每个动态库的符号在加载时完全解析。
如果当前系统上的驱动程序缺少符号存在问题，这将
暴露它并导致 Vulkan 加载器在加载驱动程序时失败。
建议使用 `LD_BIND_NOW` 以及 `VK_LOADER_DEBUG=error,warn`
来暴露任何问题。

### 使用 `VK_LUNARG_direct_driver_loading` 扩展进行驱动程序发现

`VK_LUNARG_direct_driver_loading` 扩展允许应用程序在 vkCreateInstance 期间
向加载器提供一个或多个驱动程序。
这允许驱动程序与应用程序一起提供，而无需
安装，并且可以在任何执行环境中使用，例如
以提升权限运行的进程。

当使用
`VK_LUNARG_direct_driver_loading` 扩展启用调用 `vkEnumeratePhysicalDevices` 时，来自系统安装的驱动程序和环境变量指定的驱动程序的 `VkPhysicalDevice`s
将出现在来自
`VkDirectDriverLoadingListLUNARG::pDrivers` 列表的驱动程序的任何 `VkPhysicalDevice`s 之前。

#### 如何使用 `VK_LUNARG_direct_driver_loading`

要使用此扩展，必须首先在 VkInstance 上启用它。
这需要通过
`VkInstanceCreateInfo` 的 `enabledExtensionCount` 和 `ppEnabledExtensionNames` 成员启用 `VK_LUNARG_direct_driver_loading` 扩展。

```c
const char* extensions[] = {VK_LUNARG_DIRECT_DRIVER_LOADING_EXTENSION_NAME, <other extensions>};
VkInstanceCreateInfo instance_create_info = {};
instance_create_info.enabledExtensionCount = <size of extension list>;
instance_create_info.ppEnabledExtensionNames = extensions;
```

`VkDirectDriverLoadingInfoLUNARG` 结构包含一个
`VkDirectDriverLoadingFlagsLUNARG` 成员（保留供将来使用）和一个
`PFN_vkGetInstanceProcAddrLUNARG` 成员，它向加载器提供
驱动程序的 `vkGetInstanceProcAddr` 的函数指针。

`VkDirectDriverLoadingListLUNARG` 结构包含计数和指针
成员，它们提供应用程序提供的
`VkDirectDriverLoadingInfoLUNARG` 结构数组的大小和指针。

创建这些结构如下所示
```c
VkDirectDriverLoadingInfoLUNARG direct_loading_info = {};
direct_loading_info.sType = VK_STRUCTURE_TYPE_DIRECT_DRIVER_LOADING_INFO_LUNARG
direct_loading_info.pfnGetInstanceProcAddr = <在此处放置驱动程序的 PFN_vkGetInstanceProcAddr>

VkDirectDriverLoadingListLUNARG direct_driver_list = {};
direct_driver_list.sType = VK_STRUCTURE_TYPE_DIRECT_DRIVER_LOADING_LIST_LUNARG;
direct_driver_list.mode = VK_DIRECT_DRIVER_LOADING_MODE_INCLUSIVE_LUNARG; // 或 VK_DIRECT_DRIVER_LOADING_MODE_EXCLUSIVE_LUNARG
direct_driver_list.driverCount = 1;
direct_driver_list.pDrivers = &direct_loading_info; // 如果需要，可以在此处包含多个驱动程序
```

`VkDirectDriverLoadingListLUNARG` 结构包含枚举
`VkDirectDriverLoadingModeLUNARG`。
有两种模式：
* `VK_DIRECT_DRIVER_LOADING_MODE_EXCLUSIVE_LUNARG` - 指定唯一要加载的驱动程序
将来自 `VkDirectDriverLoadingListLUNARG` 结构。
* `VK_DIRECT_DRIVER_LOADING_MODE_INCLUSIVE_LUNARG` - 指定来自
`VkDirectDriverLoadingModeLUNARG` 结构的驱动程序将与
任何系统安装的驱动程序和环境变量指定的驱动程序一起使用。



然后，`VkDirectDriverLoadingListLUNARG` 结构*必须*附加到
`VkInstanceCreateInfo` 的 `pNext` 链。

```c
instance_create_info.pNext = (const void*)&direct_driver_list;
```

最后，像往常一样创建实例。

#### 与其他驱动程序发现机制的交互

如果在
`VkDirectDriverLoadingListLUNARG` 结构中指定了 `VK_DIRECT_DRIVER_LOADING_MODE_EXCLUSIVE_LUNARG` 模式，则不会加载系统安装的驱动程序。
这同样适用于所有平台。
此外，以下环境变量无效：

* `VK_DRIVER_FILES`
* `VK_ICD_FILENAMES`
* `VK_ADD_DRIVER_FILES`
* `VK_LOADER_DRIVERS_SELECT`
* `VK_LOADER_DRIVERS_DISABLE`

独占模式还将禁用 MacOS 包清单对驱动程序的发现。

#### `VK_LUNARG_direct_driver_loading` 的限制

因为 `VkDirectDriverLoadingListLUNARG` 在实例
创建时提供给加载器，所以没有机制让加载器在
`vkEnumerateInstanceExtensionProperties` 期间查询来自 `VkDirectDriverLoadingListLUNARG` 驱动程序的实例
扩展列表。
应用程序可以改为手动加载 `vkEnumerateInstanceExtensionProperties`
函数指针，直接从应用程序提供给加载器的驱动程序
使用每个驱动程序的 `pfnGetInstanceProcAddrLUNARG`。
然后应用程序可以调用每个驱动程序的
`vkEnumerateInstanceExtensionProperties` 并将非重复条目附加到
来自加载器的 `vkEnumerateInstanceExtensionProperties` 的列表，以获取完整的
支持的实例扩展列表。
或者，因为应用程序正在提供驱动程序，所以应用程序已经
知道提供的驱动程序可用的实例扩展是合理的，
从而无需手动查询它们。

但是，存在限制。
如果有任何活动的隐式层拦截
`vkEnumerateInstanceExtensionProperties` 以删除不支持的扩展，则
这些层将无法从应用程序提供的驱动程序中删除不支持的扩展。
这是由于 `vkEnumerateInstanceExtensionProperties` 没有扩展它的机制。


### 使用预生产 ICD 或软件驱动程序

软件和预生产 ICD 都可以使用替代机制来
检测它们的驱动程序。
独立硬件供应商（IHV）可能不想完全安装预生产
ICD，因此无法在标准位置找到它。
例如，预生产 ICD 可能只是
开发者构建树中的共享库。
在这种情况下，应该有一种方法允许开发者指向这样的
ICD，而无需修改系统上已安装的 ICD。

通过使用 `VK_DRIVER_FILES` 环境变量来满足此需求，
这将覆盖用于查找系统安装的
驱动程序的机制。

换句话说，只有 `VK_DRIVER_FILES` 中列出的驱动程序才会被
使用。

有关此内容的更多信息，请参阅
[覆盖默认驱动程序发现](#覆盖默认驱动程序发现)。


### Android 上的驱动程序发现

Android 加载器位于系统库文件夹中。
位置无法更改。
加载器将通过 `hw_get_module` 加载驱动程序，ID 为 "vulkan"。
**由于 Android 中的安全策略，在**
**正常使用下无法修改这些内容。**


## 驱动程序清单文件格式

以下部分讨论驱动程序清单 JSON 文件
格式的详细信息。
JSON 文件本身对命名没有任何要求。
唯一的要求是文件的扩展后缀是 ".json"。

以下是驱动程序 JSON 清单文件的示例：

```json
{
   "file_format_version": "1.0.1",
   "ICD": {
      "library_path": "path to driver library",
      "api_version": "1.2.205",
      "library_arch" : "64",
      "is_portability_driver": false
   }
}
```

<table style="width:100%">
  <tr>
    <th>字段名称</th>
    <th>字段值</th>
  </tr>
  <tr>
    <td>"file_format_version"</td>
    <td>此文件的 JSON 格式主版本.次版本.补丁版本号。<br/>
        支持的版本有：1.0.0 和 1.0.1。</td>
  </tr>
  <tr>
    <td>"ICD"</td>
    <td>用于将所有驱动程序信息组合在一起的标识符。
        <br/>
        <b>注意：</b> 即使这被标记为 <i>ICD</i>，它也是历史性的
        并且同样适用于其他驱动程序。</td>
  </tr>
  <tr>
    <td>"library_path"</td>
    <td>"library_path" 指定驱动程序共享库文件的文件名、相对路径名或
        完整路径名。<br />
        如果 "library_path" 指定相对路径名，则它相对于
        JSON 清单文件的路径。<br />
        如果 "library_path" 指定文件名，则库必须位于
        系统的共享对象搜索路径中。<br />
        关于驱动程序共享库文件的名称没有规则，
        除了它应该以适当的后缀结尾（在
        Windows 上为 ".DLL"，在 Linux 上为 ".so"，在 macOS 上为 ".dylib"）。</td>
  </tr>
  <tr>
    <td>"library_arch"</td>
    <td>可选字段，指定与
        "library_path" 关联的二进制文件的架构。<br />
        允许加载器快速确定驱动程序的架构
        是否与正在运行的应用程序的架构匹配。<br />
        唯一有效的值是 "32" 和 "64"。</td>
  </tr>
  <tr>
    <td>"api_version" </td>
    <td>驱动程序支持的最大 Vulkan API 的主版本.次版本.补丁版本号。
        但是，仅仅因为驱动程序支持特定的 Vulkan API
        版本，并不能保证用户系统上的硬件可以
        支持该版本。
        关于底层物理设备可以支持什么的信息必须
        由用户使用 <i>vkGetPhysicalDeviceProperties</i> API
        调用来查询。<br/>
        例如：1.0.33。</td>
  </tr>
  <tr>
    <td>"is_portability_driver" </td>
    <td>定义驱动程序是否包含任何实现 VK_KHR_portability_subset 扩展的 VkPhysicalDevices。<br/>
    </td>
  </tr>
</table>

**注意：** 如果同一个驱动程序共享库支持多个不兼容的
文本清单文件格式版本，它必须为每个版本都有单独的 JSON 文件
（所有这些文件可能指向同一个共享库）。

### 驱动程序清单文件版本

当前支持的最高层清单文件格式是 1.0.1。
有关每个版本的信息在以下子节中详细说明：

#### 驱动程序清单文件版本 1.0.0

驱动程序清单文件的初始版本指定了基本的
格式和层 JSON 文件的字段。
文件格式版本 1.0.0 支持的字段包括：
 * "file\_format\_version"
 * "ICD"
 * "library\_path"
 * "api\_version"

#### 驱动程序清单文件版本 1.0.1

添加了 `is_portability_driver` 布尔字段，供驱动程序自我报告它们
包含支持 VK_KHR_portability_subset 扩展的 VkPhysicalDevices。这是一个可选字段。省略该字段与
将字段设置为 `false` 具有相同的效果。

添加了 "library\_arch" 字段到驱动程序清单，以允许加载器
快速确定驱动程序是否与当前运行的应用程序的架构匹配。此字段是可选的。

##  驱动程序 Vulkan 入口点发现

驱动程序导出的 Vulkan 符号不得与加载器的
导出的 Vulkan 符号冲突。
因此，所有驱动程序必须导出以下函数，该函数
用于发现驱动程序 Vulkan 入口点。
此入口点不是 Vulkan API 本身的一部分，只是版本 1 及更高版本
接口的加载器和驱动程序之间的私有
接口。

```cpp
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(
      VkInstance instance,
      const char* pName);
```

此函数与 `vkGetInstanceProcAddr` 具有非常相似的语义。
`vk_icdGetInstanceProcAddr` 为所有
全局级和实例级 Vulkan 函数以及
`vkGetDeviceProcAddr` 返回有效的函数指针。
全局级函数是那些第一个参数不包含可调度对象的函数，例如 `vkCreateInstance` 和
`vkEnumerateInstanceExtensionProperties`。
驱动程序必须支持通过使用 NULL `VkInstance` 参数调用
`vk_icdGetInstanceProcAddr` 来查询全局级入口点。
实例级函数是那些第一个参数可调度对象是 `VkInstance` 或
`VkPhysicalDevice` 的函数。
核心入口点和驱动程序支持的任何实例扩展入口点
都应该通过 `vk_icdGetInstanceProcAddr` 可用。
未来的 Vulkan 实例扩展可能定义并使用新的实例级
可调度对象，而不是 `VkInstance` 和 `VkPhysicalDevice`，在这种情况下
使用这些新定义的可调度对象的扩展入口点必须
可通过 `vk_icdGetInstanceProcAddr` 查询。

所有其他 Vulkan 入口点必须：
 * 不从驱动程序库直接导出
 * 或者如果导出，则不使用官方 Vulkan 函数名称

此要求适用于包含其他功能（如
OpenGL）的驱动程序库，因此可能由应用程序在 Vulkan 加载器库
由应用程序加载之前加载。

如果使用官方 Vulkan 名称，请注意动态操作系统库加载器的插入。
在 Linux 上，如果使用官方名称，驱动程序库必须与
`-Bsymbolic` 链接。


## 驱动程序 API 版本

当应用程序调用 `vkCreateInstance` 时，它可以可选地包含一个
`VkApplicationInfo` 结构，其中包括一个 `apiVersion` 字段。
Vulkan 1.0 驱动程序需要返回 `VK_ERROR_INCOMPATIBLE_DRIVER`，如果它
不支持用户传递的 API 版本。
从 Vulkan 1.1 开始，驱动程序不允许为任何 `apiVersion` 值返回此错误。
这在处理多个驱动程序时会产生问题，其中一个驱动程序是
1.0 驱动程序，另一个是较新的驱动程序。

比 1.0 更新的加载器在
应用程序调用 `vkEnumerateInstanceVersion` 时，将始终给出它支持的版本，无论系统上驱动程序支持的 API
版本如何。
这意味着当应用程序调用 `vkCreateInstance` 时，加载器将
被迫将 `VkApplicationInfo` 结构的副本传递给任何 1.0 驱动程序，其中 `apiVersion` 是
1.0，以防止错误。
为了确定是否必须这样做，加载器将执行以下步骤：

1. 检查驱动程序的 JSON 清单文件中的 "api_version" 字段。
2. 如果 JSON 版本大于或等于 1.1，加载驱动程序的
动态库
3. 调用驱动程序的 `vkGetInstanceProcAddr` 命令以获取指向
`vkEnumerateInstanceVersion` 的指针
4. 如果指向 `vkEnumerateInstanceVersion` 的指针不是 `NULL`，将
调用它以获取驱动程序支持的 API 版本

如果满足以下任何条件，驱动程序将被视为 1.0 驱动程序：

- JSON 清单的 "api_version" 字段小于版本 1.1
- 指向 `vkEnumerateInstanceVersion` 的函数指针为 `NULL`
- `vkEnumerateInstanceVersion` 返回的版本小于 1.1
- `vkEnumerateInstanceVersion` 返回除 `VK_SUCCESS` 之外的任何内容

如果驱动程序仅支持 Vulkan 1.0，加载器将确保传递给
驱动程序的任何 `VkApplicationInfo` 结构都将具有
设置为 Vulkan 1.0 的 `apiVersion` 字段。
否则，加载器将在不进行任何
更改的情况下将结构传递给驱动程序。


## 混合驱动程序实例扩展支持

在具有多个驱动程序的系统上，可能会出现特殊情况。
某些驱动程序可能公开加载器已经
知道的实例扩展。
同一系统上的其他驱动程序可能不支持相同的实例
扩展。

在这种情况下，加载器有一些额外的职责：


### 过滤实例扩展名称

在调用 `vkCreateInstance` 期间，请求的实例扩展
列表被传递给每个驱动程序。
由于驱动程序可能不支持这些实例扩展中的一个或多个，加载器将过滤掉驱动程序不支持的
任何实例扩展。
这是针对每个驱动程序完成的，因为不同的驱动程序可能支持不同的实例
扩展。


### 加载器实例扩展模拟支持

在相同的情况下，加载器必须模拟实例扩展
入口点，尽其所能，为每个不直接支持
实例扩展的驱动程序。
这必须与调用支持扩展本机的其他
驱动程序正确配合工作。
通过这种方式，应用程序将不知道哪些驱动程序
缺少对此扩展的支持。


## 驱动程序未知物理设备扩展

实现以 `VkPhysicalDevice` 作为第一个
参数的入口点的驱动程序*应该*支持 `vk_icdGetPhysicalDeviceProcAddr`。
此函数已添加到加载器和驱动程序接口版本 4，
允许加载器区分以 `VkDevice`
和 `VkPhysicalDevice` 作为第一个参数的入口点。
这允许加载器优雅地正确支持它未知的入口点。
此入口点不是 Vulkan API 本身的一部分，只是
加载器和驱动程序之间的私有接口。
注意：加载器和驱动程序接口版本 7 使导出
`vk_icdGetPhysicalDeviceProcAddr` 成为可选的。
相反，驱动程序*必须*通过 `vk_icdGetInstanceProcAddr` 公开它。

```cpp
PFN_vkVoidFunction
   vk_icdGetPhysicalDeviceProcAddr(
      VkInstance instance,
      const char* pName);
```

此函数的行为类似于 `vkGetInstanceProcAddr` 和
`vkGetDeviceProcAddr`，但它应该只返回物理设备
扩展入口点的值。
通过这种方式，它将 "pName" 与驱动程序支持的每个物理设备函数进行比较。

该函数的实现应具有以下行为：
* 如果 `pName` 是 Vulkan API 入口点的名称，该入口点以
  `VkPhysicalDevice` 作为其主要调度句柄，并且驱动程序支持该
  入口点，则驱动程序**必须**返回指向该入口点的驱动程序实现的
  有效函数指针。
* 如果 `pName` 是 Vulkan API 入口点的名称，该入口点以
  除 `VkPhysicalDevice` 之外的其他内容作为其主要调度句柄，则驱动程序**必须**返回 `NULL`。
* 如果驱动程序不知道名为 `pName` 的任何入口点，它**必须**
  返回 `NULL`。

如果驱动程序打算支持以 VkPhysicalDevice 作为
调度参数的函数，则驱动程序应该支持
`vk_icdGetPhysicalDeviceProcAddr`。这是因为如果这些函数
不为加载器所知，例如来自未发布的扩展或因为
加载器是较旧的构建，因此_尚未_知道它们，加载器
将无法区分这是设备函数还是物理设备
函数。

如果驱动程序确实实现了此支持，它必须从驱动程序库导出该函数，使用名称 `vk_icdGetPhysicalDeviceProcAddr`，以便
可以通过平台的动态链接实用程序定位符号，或者如果
驱动程序支持加载器和驱动程序接口版本 7，则通过
`vk_icdGetInstanceProcAddr` 公开它。

加载器的 `vkGetInstanceProcAddr` 对
`vk_icdGetPhysicalDeviceProcAddr` 函数的支持行为如下：
 1. 检查是否是核心函数：
    - 如果是，返回函数指针
 2. 检查是否是已知的实例或设备扩展函数：
    - 如果是，返回函数指针
 3. 调用层/驱动程序的 `GetPhysicalDeviceProcAddr`
    - 如果它返回 `non-NULL`，返回指向通用物理设备
函数的跳板，并设置一个通用终止符，该终止符将把它传递给适当的
驱动程序。
 4. 使用 `GetInstanceProcAddr` 向下调用
    - 如果它返回 non-NULL，将其视为未知逻辑设备命令。
这意味着设置一个通用跳板函数，该函数接受 `VkDevice`
作为第一个参数，并调整调度表以在从
`VkDevice` 获取调度表后调用
驱动程序/层的函数。
然后，返回指向相应跳板函数的指针。
 5. 返回 `NULL`

结果是，如果命令稍后提升为 Vulkan 核心，它将不再
使用 `vk_icdGetPhysicalDeviceProcAddr` 设置。
此外，如果加载器添加对扩展的直接支持，它将不再
到达步骤 3，因为步骤 2 将返回有效的函数指针。
但是，驱动程序应该继续支持通过
`vk_icdGetPhysicalDeviceProcAddr` 查询命令，至少直到 Vulkan 版本更新，因为
较旧的加载器可能仍在使用这些命令。

### 添加 `vk_icdGetPhysicalDeviceProcAddr` 的原因

最初，当调用加载器的 `vkGetInstanceProcAddr` 时，它会导致
以下行为：
 1. 加载器将检查它是否是核心函数：
    - 如果是，它将返回函数指针
 2. 加载器将检查它是否是已知的扩展函数：
    - 如果是，它将返回函数指针
 3. 如果加载器对它一无所知，它将使用
`GetInstanceProcAddr` 向下调用
    - 如果它返回 `non-NULL`，将其视为未知逻辑设备命令。
    - 这意味着设置一个通用跳板函数，该函数接受
VkDevice 作为第一个参数，并调整调度表以在从
`VkDevice` 获取调度表后调用
驱动程序/层的函数。
 4. 如果上述所有操作都失败，加载器将向应用程序返回 `NULL`。

这导致当驱动程序尝试公开加载器不知道的新物理设备
扩展时出现问题，但应用程序知道。
因为加载器对它一无所知，加载器将到达上述过程中的步骤 3，并将该函数视为未知逻辑设备命令。
问题是，这将创建一个通用 `VkDevice` 跳板函数
，在第一次调用时，将尝试将 VkPhysicalDevice 解引用为
`VkDevice`。
这将导致崩溃或损坏。

## 物理设备排序

当应用程序选择要使用的 GPU 时，它必须枚举物理设备或
物理设备组。
这些 API 函数不指定物理设备或物理
设备组将按什么顺序呈现。
在 Windows 上，加载器将尝试对这些对象进行排序，以便系统
首选项将首先列出。
此机制不会强制应用程序使用任何特定的 GPU &mdash;
它只是改变它们呈现的顺序。

此机制要求驱动程序提供加载器和驱动程序接口
版本 6。
此版本定义了一个新的导出函数 `vk_icdEnumerateAdapterPhysicalDevices`，
如下所述，驱动程序可以在 Windows 上提供。
此入口点不是 Vulkan API 本身的一部分，只是
加载器和驱动程序之间的私有接口。
注意：加载器和驱动程序接口版本 7 使导出
`vk_icdEnumerateAdapterPhysicalDevices` 成为可选的。
相反，驱动程序*必须*通过 `vk_icdGetInstanceProcAddr` 公开它。

```c
VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdEnumerateAdapterPhysicalDevices(
      VkInstance instance,
      LUID adapterLUID,
      uint32_t* pPhysicalDeviceCount,
      VkPhysicalDevice* pPhysicalDevices);
```


此函数接受适配器 LUID 作为输入，并枚举所有与该
LUID 关联的 Vulkan 物理设备。
这与其他 Vulkan 枚举的工作方式相同 &mdash; 如果
`pPhysicalDevices` 为 `NULL`，则将提供计数。
否则，将提供与查询的适配器关联的物理设备。
当 LUID 引用链接适配器时，该函数必须提供多个物理设备。
这允许加载器将适配器转换为 Vulkan 物理设备
组。

虽然加载器尝试匹配系统的 GPU 排序首选项，
但存在一些限制。
因为此功能需要新的驱动程序接口，只有来自
支持此函数的驱动程序的物理设备才会被排序。
所有未排序的物理设备将列在列表的末尾，顺序
不确定。
此外，只有对应于适配器的物理设备可能被排序。
这意味着软件驱动程序可能不会被排序。
最后，此 API 仅适用于 Windows 系统，并且仅在
支持通过操作系统进行 GPU 选择的 Windows 10 版本上工作。
将来可能包括其他平台，但它们将需要单独的
平台特定接口。

`vk_icdEnumerateAdapterPhysicalDevices` 的要求是它*必须*
为与 `vkEnumeratePhysicalDevices` 返回的相同物理
设备返回相同的 `VkPhysicalDevice` 句柄值。
这是因为加载器在驱动程序上调用两个函数，然后
使用 `VkPhysicalDevice` 句柄对物理设备进行去重。
由于驱动程序中的并非所有物理设备都有 LUID，例如
软件实现，此步骤对于允许驱动程序
枚举所有可用物理设备是必要的。

## 驱动程序可调度对象创建

如前所述，加载器要求调度表在
Vulkan 可调度对象中可访问，例如：`VkInstance`、`VkPhysicalDevice`、
`VkDevice`、`VkQueue` 和 `VkCommandBuffer`。
驱动程序创建的所有可调度对象的
具体要求如下：

- 驱动程序创建的所有可调度对象可以转换为 void \*\*
- 加载器将用指向调度表的指针替换第一个条目
，该调度表由加载器拥有。
这对驱动程序意味着三件事：
  1. 驱动程序必须返回不透明可调度对象句柄的指针
  2. 此指针指向一个常规 C 结构，第一个条目是
  指针。
   * **注意：** 对于任何直接实现 VK 对象的 C\++ 驱动程序
作为 C\++ 类：
     * 如果类由于使用虚函数而非 POD，C\++ 编译器可能会在偏移零处放置 vtable。
     * 在这种情况下，使用常规 C 结构（见下文）。
  3. 加载器检查所有创建的可调度对象中的魔数值（ICD\_LOADER\_MAGIC），如下所示（参见 `include/vulkan/vk_icd.h`）：

```cpp
#include "vk_icd.h"

union _VK_LOADER_DATA {
  uintptr loadermagic;
  void *  loaderData;
} VK_LOADER_DATA;

vkObj
   alloc_icd_obj()
{
  vkObj *newObj = alloc_obj();
  ...
  // 使用 ICD_LOADER_MAGIC 初始化指向加载器调度表的指针

  set_loader_magic_value(newObj);
  ...
  return newObj;
}
```


## WSI 扩展中处理 KHR Surface 对象

通常，驱动程序处理各种 Vulkan
对象的对象创建和销毁。
适用于 Linux、Windows、macOS 和 QNX 的 WSI surface 扩展
（"VK\_KHR\_win32\_surface"、"VK\_KHR\_xcb\_surface"、"VK\_KHR\_xlib\_surface"、
"VK\_KHR\_wayland\_surface"、"VK\_MVK\_macos\_surface"、
"VK\_QNX\_screen\_surface" 和 "VK\_KHR\_surface"）的处理方式不同。
对于这些扩展，`VkSurfaceKHR` 对象的创建和销毁可能由
加载器或驱动程序处理。

如果加载器处理 `VkSurfaceKHR` 对象的管理：
 1. 加载器将处理对 `vkCreateXXXSurfaceKHR` 和
`vkDestroySurfaceKHR`
    函数的调用，而不涉及驱动程序。
    * 其中 XXX 代表窗口系统名称：
      * Wayland
      * XCB
      * Xlib
      * Windows
      * Android
      * MacOS (`vkCreateMacOSSurfaceMVK`)
      * QNX (`vkCreateScreenSurfaceQNX`)
 2. 加载器为相应的
`vkCreateXXXSurfaceKHR` 调用创建一个 `VkIcdSurfaceXXX` 对象。
    * `VkIcdSurfaceXXX` 结构在 `include/vulkan/vk_icd.h` 中定义。
 3. 驱动程序可以将任何 `VkSurfaceKHR` 对象转换为指向
适当的 `VkIcdSurfaceXXX` 结构的指针。
 4. 所有 `VkIcdSurfaceXXX` 结构的第一个字段是
`VkIcdSurfaceBase` 枚举，指示
    surface 对象是 Win32、XCB、Xlib、Wayland 还是 Screen。

驱动程序可以选择改为处理 `VkSurfaceKHR` 对象创建。
如果驱动程序希望处理创建和销毁，它必须执行以下操作：
 1. 支持加载器和驱动程序接口版本 3 或更新版本。
 2. 公开并处理所有接受 `VkSurfaceKHR` 对象的函数，
包括：
     * `vkCreateXXXSurfaceKHR`
     * `vkGetPhysicalDeviceSurfaceSupportKHR`
     * `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`
     * `vkGetPhysicalDeviceSurfaceFormatsKHR`
     * `vkGetPhysicalDeviceSurfacePresentModesKHR`
     * `vkCreateSwapchainKHR`
     * `vkDestroySurfaceKHR`

因为 `VkSurfaceKHR` 对象是实例级对象，一个对象可以
与多个驱动程序关联。
因此，当加载器收到 `vkCreateXXXSurfaceKHR` 调用时，它仍然
创建一个内部 `VkSurfaceIcdXXX` 对象。
此对象充当每个驱动程序的 `VkSurfaceKHR` 对象版本的
容器。
如果驱动程序不支持创建自己的 `VkSurfaceKHR` 对象，则
加载器的容器为该驱动程序存储 NULL。
另一方面，如果驱动程序确实支持 `VkSurfaceKHR` 创建，则
加载器将对驱动程序进行适当的 `vkCreateXXXSurfaceKHR` 调用，并将返回的指针存储在其容器对象中。
然后加载器将 `VkSurfaceIcdXXX` 作为 `VkSurfaceKHR` 对象返回调用链。
最后，当加载器收到 `vkDestroySurfaceKHR` 调用时，它
随后为每个内部
`VkSurfaceKHR` 对象不为 NULL 的驱动程序调用 `vkDestroySurfaceKHR`。
然后加载器在返回之前销毁容器对象。


## 加载器与驱动程序接口协商

通常，对于应用程序发出的函数，加载器可以被视为
传递。
也就是说，加载器通常不会修改函数或其参数，
而只是调用该函数的驱动程序入口点。
驱动程序需要遵守的特定附加接口要求
不是 Vulkan 规范中任何要求的一部分。
这些附加要求已版本化，以便将来具有灵活性。


### Windows、Linux 和 macOS 驱动程序协商


#### 加载器与驱动程序之间的版本协商

支持加载器和驱动程序接口版本 2 或更高版本的所有驱动程序必须
导出以下函数，该函数用于确定将使用的接口
版本。
此入口点不是 Vulkan API 本身的一部分，只是
加载器和驱动程序之间的私有接口。
注意：加载器和驱动程序接口版本 7 使导出
`vk_icdNegotiateLoaderICDInterfaceVersion` 成为可选的。
相反，驱动程序*必须*通过 `vk_icdGetInstanceProcAddr` 公开它。

```cpp
VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdNegotiateLoaderICDInterfaceVersion(
      uint32_t* pSupportedVersion);
```

此函数允许加载器和驱动程序就使用的接口版本达成一致。
"pSupportedVersion" 参数既是输入参数也是输出参数。
"pSupportedVersion" 由加载器填充，其中包含加载器支持的所需最新接口
版本（通常是最新的）。
驱动程序接收此值并在同一
字段中返回它期望的版本。
因为它正在设置加载器和
驱动程序之间的接口版本，这应该是加载器对驱动程序进行的第一次调用（甚至在
任何对 `vk_icdGetInstanceProcAddr` 的调用之前）。

如果接收调用的驱动程序不再支持加载器提供的接口
版本（由于弃用），则它应该报告
`VK_ERROR_INCOMPATIBLE_DRIVER` 错误。
否则，它将 "pSupportedVersion" 指向的值设置为最新
接口版本，该版本由驱动程序和加载器共同支持，并返回
`VK_SUCCESS`。

如果加载器提供的接口版本比驱动程序支持的更新，驱动程序应该报告 `VK_SUCCESS`，因为确定它是否可以支持驱动程序支持的较旧接口版本是加载器的责任。
如果驱动程序的接口版本大于加载器的接口版本，驱动程序也应该报告 `VK_SUCCESS`，但返回加载器的版本。
因此，在返回 `VK_SUCCESS` 时，"pSupportedVersion" 将包含
驱动程序要使用的所需接口版本。

如果加载器从驱动程序接收到加载器不再支持的接口版本（由于弃用），或者它接收到
`VK_ERROR_INCOMPATIBLE_DRIVER` 错误而不是 `VK_SUCCESS`，则加载器
将驱动程序视为不兼容，并且不会加载它以供使用。
在这种情况下，应用程序在枚举期间不会看到驱动程序的 `vkPhysicalDevice`。


#### 与旧版驱动程序或加载器的接口

如果加载器看到驱动程序不导出或公开
`vk_icdNegotiateLoaderICDInterfaceVersion` 函数，则加载器假定
相应的驱动程序仅支持接口版本 0 或 1。

从接口的另一端，如果驱动程序看到在
调用 `vk_icdNegotiateLoaderICDInterfaceVersion` 之前调用
`vk_icdGetInstanceProcAddr`，则加载器要么是仅支持接口版本 0 或 1 的旧版
加载器，要么加载器正在使用
接口版本 7 或更新版本。

如果对 `vk_icdGetInstanceProcAddr` 的第一次调用是查询
`vk_icdNegotiateLoaderICDInterfaceVersion`，则这意味着加载器正在使用
接口版本 7。
这仅在驱动程序不导出
`vk_icdNegotiateLoaderICDInterfaceVersion` 时发生。
导出 `vk_icdNegotiateLoaderICDInterfaceVersion` 的驱动程序将首先调用它。

如果对 `vk_icdGetInstanceProcAddr` 的第一次调用**不是**查询
`vk_icdNegotiateLoaderICDInterfaceVersion`，则加载器是仅
支持版本 0 或 1 的旧版加载器。
在这种情况下，如果加载器首先调用 `vk_icdGetInstanceProcAddr`，它支持
至少接口版本 1。
否则，加载器仅支持版本 0。

#### 加载器与驱动程序接口版本 7 要求

版本 7 放宽了加载器和驱动程序接口函数
必须导出的要求。
相反，它只要求这些函数可以通过
`vk_icdGetInstanceProcAddr` 查询。
这些函数是：
    `vk_icdNegotiateLoaderICDInterfaceVersion`
    `vk_icdGetPhysicalDeviceProcAddr`
    `vk_icdEnumerateAdapterPhysicalDevices`（仅 Windows）
这些函数在检索目的上被视为全局的，因此
`vk_icdGetInstanceProcAddr` 的 `VkInstance` 参数将为 **NULL**。
虽然导出这些函数不再是要求，但驱动程序仍可能
导出它们以与旧版加载器兼容。
此版本中的更改允许通过
`VK_LUNARG_direct_driver_loading` 扩展提供的驱动程序支持整个加载器和
驱动程序接口。

#### 加载器与驱动程序接口版本 6 要求

版本 6 提供了一种机制，允许加载器对物理设备进行排序。
加载器仅在支持接口版本 6
时才会尝试对驱动程序上的物理设备进行排序。
此版本提供了本文档前面定义的 `vk_icdEnumerateAdapterPhysicalDevices` 函数。

#### 加载器与驱动程序接口版本 5 要求

此接口版本对实际接口没有更改。
如果加载器请求接口版本 5 或更高版本，它只是
向驱动程序表明加载器现在正在评估传递给 vkCreateInstance 的 API
版本信息是否是加载器的有效版本。
如果不是，加载器将在 vkCreateInstance 期间捕获此情况，并以
`VK_ERROR_INCOMPATIBLE_DRIVER` 错误失败。

另一方面，如果加载器未请求版本 5 或更新版本，则它
向驱动程序表明加载器不知道正在请求的 API 版本。
因此，由驱动程序验证 API 版本是否
不大于主版本 = 1 和次版本 = 0。
如果是，则驱动程序应自动失败，并返回
`VK_ERROR_INCOMPATIBLE_DRIVER` 错误，因为加载器是 1.0 加载器，并且
不知道该版本。

以下是预期行为表：

<table style="width:100%">
  <tr>
    <th>加载器支持的接口版本</th>
    <th>驱动程序支持的接口版本</th>
    <th>结果</th>
  </tr>
  <tr>
    <td>4 或更早</td>
    <td>任何版本</td>
    <td>驱动程序<b>必须失败</b>，返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>
        ，对于所有 apiVersion 设置为 > Vulkan 1.0 的 vkCreateInstance 调用
        ，因为加载器仍处于接口版本 <= 4。<br/>
        否则，驱动程序应正常行为。
    </td>
  </tr>
  <tr>
    <td>5 或更新</td>
    <td>4 或更早</td>
    <td>如果加载器无法处理 apiVersion，加载器<b>必须失败</b>，返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>。
        驱动程序可能对所有 apiVersions 通过，但由于其接口是
        <= 4，最好假设它需要拒绝
        任何 > Vulkan 1.0 的内容，并返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>。
        <br/>
        否则，驱动程序应正常行为。
    </td>
  </tr>
  <tr>
    <td>5 或更新</td>
    <td>5 或更新</td>
    <td>如果加载器无法处理 apiVersion，加载器<b>必须失败</b>，返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>，驱动程序应仅在无法支持
        指定的 apiVersion 时返回
        <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>。<br/>
        否则，驱动程序应正常行为。
    </td>
  </tr>
</table>

#### 加载器与驱动程序接口版本 4 要求

此接口版本 4 的主要更改是支持
[未知物理设备扩展](#驱动程序未知物理设备扩展)
使用 `vk_icdGetPhysicalDeviceProcAddr` 函数。
此函数纯粹是可选的。
但是，如果驱动程序支持物理设备扩展，它必须提供
`vk_icdGetPhysicalDeviceProcAddr` 函数。
否则，加载器将继续将任何未知函数视为 VkDevice
函数并导致无效行为。


#### 加载器与驱动程序接口版本 3 要求

此接口版本的主要更改是允许驱动程序
处理自己的 KHR_surfaces 的创建和销毁。
在此之前，加载器创建了一个由所有
驱动程序使用的 surface 对象。
但是，某些驱动程序*可能*希望提供自己的 surface 句柄。
如果驱动程序选择启用此支持，它必须支持加载器和驱动程序
接口版本 3，以及任何使用 `VkSurfaceKHR`
句柄的 Vulkan 函数，例如：
- `vkCreateXXXSurfaceKHR`（其中 XXX 是平台特定的标识符 [即
Windows 的 `vkCreateWin32SurfaceKHR`]）
- `vkDestroySurfaceKHR`
- `vkCreateSwapchainKHR`
- `vkGetPhysicalDeviceSurfaceSupportKHR`
- `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`
- `vkGetPhysicalDeviceSurfaceFormatsKHR`
- `vkGetPhysicalDeviceSurfacePresentModesKHR`

不参与此功能的驱动程序可以通过
简单地不公开上述 `vkCreateXXXSurfaceKHR` 和
`vkDestroySurfaceKHR` 函数来选择退出。


#### 加载器与驱动程序接口版本 2 要求

接口版本 2 要求驱动程序导出
`vk_icdNegotiateLoaderICDInterfaceVersion`。
有关更多信息，请参阅[加载器与驱动程序之间的版本协商](#加载器与驱动程序之间的版本协商)。

此外，版本 2 要求驱动程序创建的 Vulkan 可调度对象必须按照
[驱动程序可调度对象创建](#驱动程序可调度对象创建)
部分创建。


#### 加载器与驱动程序接口版本 1 要求

接口版本 1 添加了驱动程序特定的入口点
`vk_icdGetInstanceProcAddr`。
由于这是在创建
`vk_icdNegotiateLoaderICDInterfaceVersion` 入口点之前，加载器没有
确定驱动程序支持什么接口版本的协商过程。
因此，加载器通过缺少协商函数但存在
`vk_icdGetInstanceProcAddr` 来检测对接口版本 1 的支持。
驱动程序不需要导出其他入口点，因为加载器将使用该函数查询
适当的函数指针。


#### 加载器与驱动程序接口版本 0 要求

版本 0 不支持 `vk_icdGetInstanceProcAddr` 或
`vk_icdNegotiateLoaderICDInterfaceVersion`。
因此，加载器将假定驱动程序仅支持接口版本 0，除非存在这些函数之一。

此外，对于版本 0，驱动程序必须至少公开以下核心
Vulkan 入口点，以便加载器可以构建与驱动程序的接口：

- 函数 `vkGetInstanceProcAddr` **必须从驱动程序
库导出**，并为所有 Vulkan API 入口点返回有效的函数指针。
- `vkCreateInstance` **必须从驱动程序库导出**。
- `vkEnumerateInstanceExtensionProperties` **必须从驱动程序
库导出**。


#### 附加接口说明：

- 加载器将在调用驱动程序之前过滤掉 `vkCreateInstance` 和
`vkCreateDevice` 中请求的扩展；过滤将针对由与驱动程序不同的实体（例如层）公开的扩展。
- 加载器不会为驱动程序调用 `vkEnumerate*LayerProperties`
，因为层属性是从层库和层 JSON 文件获得的。
- 如果驱动程序库作者想要实现层，可以通过让
相应的层 JSON 清单文件引用驱动程序库文件来实现。
- 如果 "pLayerName" 不等于 `NULL`，加载器不会为驱动程序调用 `vkEnumerate*ExtensionProperties`。
- 通过设备扩展创建新可调度对象的驱动程序需要
初始化创建的可调度对象。
加载器为未知设备扩展提供了通用*跳板*代码。
此通用*跳板*代码不会初始化
新创建对象内的调度表。
有关如何为非加载器已知的扩展初始化创建的可调度对象的更多信息，请参阅
[驱动程序可调度对象创建](#驱动程序可调度对象创建)
部分。


### Android 驱动程序协商

Android 加载器使用与上述相同的协议来初始化调度表。
唯一的区别是 Android 加载器直接从相应的库查询层和扩展
信息，不使用 Windows、Linux 和 macOS 加载器使用的 JSON
清单文件。


## 加载器对 VK_KHR_portability_enumeration 的实现

加载器实现了 `VK_KHR_portability_enumeration` 实例扩展，
它过滤掉任何报告支持可移植性子集
设备扩展的驱动程序。除非应用程序通过在
VkInstanceCreateInfo::flags 中设置
`VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` 位来显式请求枚举
可移植性设备，否则加载器不会加载任何声明
自己为可移植性驱动程序的驱动程序。

驱动程序在驱动程序
清单 Json 文件中声明它们是否为可移植性驱动程序，使用 `is_portability_driver` 布尔字段。
[更多信息请参见此处](#驱动程序清单文件版本-101)

此扩展的初始支持仅在应用程序
未启用可移植性枚举功能时报告错误。它没有过滤掉
可移植性驱动程序。这样做是为了给应用程序一个宽限期，以便在
应用程序完全中断之前更新其实例创建逻辑。

## 加载器与驱动程序策略

本节旨在定义加载器
和驱动程序之间预期的正确行为。
本节的大部分内容是对 Vulkan 规范的补充，对于
跨平台维护一致性是必要的。
实际上，大部分语言可以在本文档中找到，但为了
方便起见，在此处进行了总结。
此外，应该有一种方法来识别驱动程序中的不良或不符合规范的行为
，并尽快纠正它。
因此，提供了策略编号系统，以唯一的方式清楚地识别每个
策略声明。

最后，基于使加载器高效和高性能的目标，
一些定义正确驱动程序行为的策略声明可能
不可测试（因此加载器无法强制执行）。
但是，这不应影响为了向最终用户和开发人员提供
最佳体验而提供的要求。


### 编号格式

加载器和驱动程序策略项以前缀 `LDP_` 开头（
加载器和驱动程序策略的缩写），后跟基于
策略针对的组件的标识符。
在这种情况下，只有两个可能的组件：
 - 驱动程序：策略编号中将包含字符串 `DRIVER_`。
 - 加载器：策略编号中将包含字符串 `LOADER_`
   作为策略编号的一部分。


### Android 差异

如前所述，Android 加载器实际上与 Khronos
加载器是分开的。
因此，由于此和其他平台要求，并非所有这些策略
声明都适用于 Android。
每个表还有一个标题为"适用于 Android？"的列
，它指示哪些策略声明适用于仅专注于
Android 支持的驱动程序。
有关 Android 加载器的更多信息，请参阅
<a href="https://source.android.com/devices/graphics/implement-vulkan">
Android Vulkan 文档</a>。


### 行为良好的驱动程序的要求

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
    <td><small><b>LDP_DRIVER_1</b></small></td>
    <td>驱动程序<b>不得</b>导致其他驱动程序失败、崩溃或
        其他方式行为异常。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_2</b></small></td>
    <td>驱动程序<b>不得</b>在检测到系统上没有支持的
        Vulkan 物理设备（<i>VkPhysicalDevice</i>）时崩溃，当使用任何 Vulkan 实例或物理设备
        API 对该驱动程序进行调用时。<br/>
        这是因为某些设备可以热插拔。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td>否<br/>
        加载器不知道给定驱动程序可能支持哪些设备（虚拟或物理）。
    </td>
    <td><small>N/A</small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_3</b></small></td>
    <td>驱动程序<b>必须</b>能够根据规定的
        协商过程与加载器协商支持的加载器和驱动程序接口版本。
    </td>
    <td>驱动程序将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#加载器与驱动程序接口协商">
        接口协商</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_4</b></small></td>
    <td>驱动程序<b>必须</b>具有有效的 JSON 清单文件供加载器
        处理，该文件以 ".json" 后缀结尾。
    </td>
    <td>驱动程序将不会被加载。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#驱动程序清单文件格式">清单文件格式</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_5</b></small></td>
    <td>驱动程序<b>必须</b>通过 Khronos 提交、验证和批准的结果通过一致性测试
        ，然后才能通过 Vulkan 提供的任何机制报告一致性版本
        （示例包括在
        <i>VkPhysicalDeviceVulkan12Properties</i> 和
        <i>VkPhysicalDeviceDriverProperties</i> 结构内部）。<br/>
        否则，当遇到包含一致性版本的此类结构时，驱动程序<b>必须</b>返回一致性版本
        为 0.0.0.0，以表明它尚未经过如此验证和批准。
    </td>
    <td>是</td>
    <td>否</td>
    <td>加载器和/或应用程序可能对驱动程序的
        功能做出假设，导致未定义的行为
        ，可能包括崩溃或损坏。
    </td>
    <td><small>
        <a href="https://github.com/KhronosGroup/VK-GL-CTS/blob/main/external/openglcts/README.md">
        Vulkan CTS 文档</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_6</b></small></td>
    <td>已移除 - 请参阅
        <a href="#已移除的驱动程序策略">已移除的驱动程序策略</a>
    </td>
    <td>-</td>
    <td>-</td>
    <td>-</td>
    <td>-</td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_7</b></small></td>
    <td>如果驱动程序希望支持 Vulkan API 1.1 或更新版本，它<b>必须</b>
        公开对加载器和驱动程序接口版本 5 或更新版本的支持。
    </td>
    <td>驱动程序将在不应使用时被使用，并将导致
        未定义的行为，可能包括崩溃或损坏。
    </td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#加载器版本-5-接口要求">
        版本 5 接口要求</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_8</b></small></td>
    <td>如果驱动程序希望处理自己的 <i>VkSurfaceKHR</i> 对象
        创建，它<b>必须</b>实现加载器和驱动程序接口版本 3 或
        更新版本，并支持通过
        <i>vk_icdGetInstanceProcAddr</i> 查询所有相关的 surface 函数。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td>是</td>
    <td><small>
        <a href="#wsi-扩展中处理-khr-surface-对象">
        处理 KHR Surface 对象</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_9</b></small></td>
    <td>如果版本协商导致驱动程序使用加载器
        和驱动程序接口版本 4 或更早版本，驱动程序<b>必须</b>验证
        传递给 <i>vkCreateInstance</i> 的 Vulkan API 版本（通过
        <i>VkInstanceCreateInfo</i> 的 <i>VkApplicationInfo</i> 的
        <i>apiVersion</i>）是否受支持。
        如果请求的 Vulkan API 版本无法由驱动程序支持，
        它<b>必须</b>返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>。<br/>
        如果接口版本是 5 或更新版本，则不需要这样做，因为
        加载器负责此检查。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td>否</td>
    <td><small>
        <a href="#加载器版本-5-接口要求">
        版本 5 接口要求</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_10</b></small></td>
    <td>如果版本协商导致驱动程序使用加载器和驱动程序接口
        版本 5 或更新版本，驱动程序<b>不得</b>返回
        <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>，如果传递给 <i>vkCreateInstance</i> 的 Vulkan API 版本
        （通过
        <i>VkInstanceCreateInfo</i> 的 <i>VkApplicationInfo</i> 的
        <i>apiVersion</i>）不受驱动程序支持。此检查由
        加载器代表驱动程序执行。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td>否</td>
    <td><small>
        <a href="#加载器版本-5-接口要求">
        版本 5 接口要求</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_11</b></small></td>
    <td>驱动程序<b>必须</b>在卸载时删除所有清单文件和对这些
        文件的引用（即 Windows 上的注册表项）。
        <br/>
        同样，在更新驱动程序文件时，旧文件<b>必须</b>
        全部更新或删除。
    </td>
    <td>如果旧文件指向不正确的库，将
        导致未定义的行为，可能包括崩溃或损坏。
    </td>
    <td>否</td>
    <td>否<br/>
        加载器不知道哪些驱动程序文件是新、旧还是不正确的。
        任何类型的驱动程序文件验证都会很快变得非常复杂
        ，因为它需要加载器维护一个内部数据库
        ，根据驱动程序供应商、驱动程序
        版本、目标平台和其他可能的标准跟踪行为不良的驱动程序。
    </td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_12</b></small></td>
    <td>为了与公共 Khronos 加载器正常工作，驱动程序
        <b>不得</b>在首先与 Khronos 发布它们之前公开平台接口扩展。<br/>
        正在开发中的平台可以使用 Khronos 加载器的修改版本
        ，直到设计变得稳定和/或公开。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是（特别是对于 Android 扩展）</td>
    <td>否</td>
    <td><small>N/A</small></td>
  </tr>
</table>

#### 已移除的驱动程序策略

这些策略在加载器源代码中的某个时刻存在，但后来被移除。
为了参考，在此处记录了它们。

<table>
  <tr>
    <th>要求编号</th>
    <th>要求描述</th>
    <th>移除原因</th>
  </tr>
  <tr>
    <td><small><b>LDP_DRIVER_6</b></small></td>
    <td>支持加载器和驱动程序接口版本 1 或更新版本的驱动程序<b>必须
        不</b>直接导出标准 Vulkan 入口点。
        <br/>
        相反，它<b>必须</b>仅导出
        它支持的接口版本所需的加载器接口函数
        （例如
        <i>vk_icdGetInstanceProcAddr</i>）。<br/>
        这是因为某些平台上的动态链接在
        过去一直存在问题，有时会错误地链接到
        来自错误动态库的导出函数。<br/>
        <b>注意：</b> 这实际上适用于所有导出。
        如有疑问，不要从可能在其他库中导致
        冲突的驱动程序中导出任何项目。<br/>
    </td>
    <td>
        由于驱动程序导出核心入口点存在有效情况，此策略已被移除。
        此外，未发现动态链接在实践中会导致许多
        问题。
    </td>
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
    <td><small><b>LDP_LOADER_1</b></small></td>
    <td>加载器<b>必须</b>在无法在系统上找到并加载有效的 Vulkan 驱动程序时返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small>N/A</small></td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_2</b></small></td>
    <td>加载器<b>必须</b>尝试加载它
        发现并确定符合本文档格式的任何驱动程序的清单文件。
        <br/>
        <b>唯一的</b>例外是在通过某些其他机制确定驱动程序
        位置和功能的平台上。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small>
        <a href="#驱动程序发现">驱动程序发现</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_3</b></small></td>
    <td>加载器<b>必须</b>支持一种机制来在一个或多个
        非标准位置加载驱动程序。<br/>
        这是为了允许支持完全软件驱动程序以及
        评估开发中的 ICD。<br/>
        <b>唯一的</b>例外是如果操作系统由于安全策略而不希望
        支持此功能。
    </td>
    <td>某些工具和驱动程序开发人员使用 Vulkan 加载器将更加困难。</td>
    <td>否</td>
    <td><small>
        <a href="#使用预生产-icd-或软件驱动程序">
        预生产 ICD 或 SW</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_4</b></small></td>
    <td>加载器<b>不得</b>加载定义与自身不兼容的 API
        版本的 Vulkan 驱动程序。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small>
        <a href="#驱动程序发现">驱动程序发现</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_5</b></small></td>
    <td>加载器<b>必须</b>忽略任何无法协商兼容的
        加载器和驱动程序接口版本的驱动程序。
    </td>
    <td>加载器将错误地加载驱动程序，导致未定义
        的行为，可能包括崩溃或损坏。
    </td>
    <td>否</td>
    <td><small>
        <a href="#加载器与驱动程序接口协商">
        接口协商</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_6</b></small></td>
    <td>如果驱动程序协商导致加载器使用加载器和驱动程序
        接口版本 5 或更新版本，加载器<b>必须</b>验证传递给 <i>vkCreateInstance</i> 的 Vulkan
        API 版本（通过
        <i>VkInstanceCreateInfo</i> 的 <i>VkApplicationInfo</i> 的
        <i>apiVersion</i>）是否至少由一个驱动程序支持。
        如果请求的 Vulkan API 版本无法由任何
        驱动程序支持，加载器<b>必须</b>返回
        <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>。<br/>
        如果加载器和驱动程序接口版本是 4 或
        更早版本，则不需要这样做，因为此检查的责任落在驱动程序上。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#加载器版本-5-接口要求">
        版本 5 接口要求</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_7</b></small></td>
    <td>如果系统上存在多个驱动程序，并且其中一些
        驱动程序支持<i>仅</i> Vulkan API 版本 1.0，而其他驱动程序
        支持较新的 Vulkan API 版本，则加载器<b>必须</b>调整
        <i>VkInstanceCreateInfo</i> 的 <i>VkApplicationInfo</i> 的
        <i>apiVersion</i> 字段为版本 1.0，对于所有仅
        知道 Vulkan API 版本 1.0 的驱动程序。<br/>
        否则，支持 Vulkan API 版本 1.0 的驱动程序将
        在
        <i>vkCreateInstance</i> 期间返回 <b>VK_ERROR_INCOMPATIBLE_DRIVER</b>，因为 1.0 驱动程序不知道未来的
        版本。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#驱动程序-api-版本">驱动程序 API 版本</a>
        </small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_8</b></small></td>
    <td>如果存在多个驱动程序，并且至少一个驱动程序<i>不支持</i>
        其他驱动程序支持的实例级功能；
        则加载器<b>必须</b>以某种方式为不支持
        的驱动程序支持实例级功能。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#加载器实例扩展模拟支持">
        加载器实例扩展模拟支持</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_9</b></small></td>
    <td>加载器<b>必须</b>从
        <i>VkInstanceCreateInfo</i> 结构的 <i>ppEnabledExtensionNames</i>
        字段中过滤掉驱动程序在调用驱动程序的
        <i>vkCreateInstance</i> 期间不支持的实例扩展。<br/>
        这是因为应用程序无法知道哪些
        驱动程序支持哪些扩展。<br/>
        这与上面的 <i>LDP_LOADER_8</i> 直接相关。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#过滤实例扩展名称">
        过滤实例扩展名称</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_10</b></small></td>
    <td>加载器<b>必须</b>支持创建<i>VkSurfaceKHR</i> 句柄
        ，这些句柄<b>可能</b>由所有底层驱动程序共享。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small>
        <a href="#wsi-扩展中处理-khr-surface-对象">
        处理 KHR Surface 对象</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_11</b></small></td>
    <td>如果驱动程序公开适当的 <i>VkSurfaceKHR</i>
        创建/处理入口点，加载器<b>必须</b>支持创建
        驱动程序特定的 surface 对象句柄，并在请求时提供它，而不是
        共享的 <i>VkSurfaceKHR</i> 句柄，返回给该驱动程序。
        <br/>
        否则，加载器<b>必须</b>提供加载器创建的
        <i>VkSurfaceKHR</i> 句柄。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>否</td>
    <td><small>
        <a href="#wsi-扩展中处理-khr-surface-对象">
        处理 KHR Surface 对象</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_12</b></small></td>
    <td>加载器<b>不得</b>在驱动程序上调用任何 <i>vkEnumerate*ExtensionProperties</i>
        入口点，如果 <i>pLayerName</i> 不是 <b>NULL</b>。
    </td>
    <td>行为未定义，可能导致崩溃或损坏。</td>
    <td>是</td>
    <td><small>
        <a href="#附加接口说明">
        附加接口说明</a></small>
    </td>
  </tr>
  <tr>
    <td><small><b>LDP_LOADER_13</b></small></td>
    <td>加载器<b>必须</b>在运行提升
        （管理员/超级用户）应用程序时不从用户定义的路径加载（包括
        使用任何 <i>VK_ICD_FILENAMES</i>、<i>VK_DRIVER_FILES</i> 或
        <i>VK_ADD_DRIVER_FILES</i> 环境变量）。<br/>
        <b>这是出于安全原因。</b>
    </td>
    <td>行为未定义，可能导致计算机安全漏洞、
        崩溃或损坏。
    </td>
    <td>否</td>
    <td><small>
        <a href="#提升权限的例外情况">
          提升权限的例外情况
        </a></small>
    </td>
  </tr>
</table>

<br/>

[返回顶级 LoaderInterfaceArchitecture.md 文件。](LoaderInterfaceArchitecture.md)