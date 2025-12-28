<!-- markdownlint-disable MD041 -->
[![Khronos Vulkan][1]][2]

[1]: images/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

# 调试 Vulkan 桌面加载器 <!-- omit from toc -->
[![Creative Commons][3]][4]

<!-- Copyright &copy; 2015-2023 LunarG, Inc. -->

[3]: images/Creative_Commons_88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/
## 目录 <!-- omit from toc -->

- [调试问题](#调试问题)
- [加载器日志](#加载器日志)
- [调试可能的层问题](#调试可能的层问题)
  - [启用层日志](#启用层日志)
  - [禁用层](#禁用层)
  - [选择性重新启用层](#选择性重新启用层)
- [允许特定层忽略 VK\_LOADER\_LAYERS\_DISABLE](#允许特定层忽略-vk_loader_layers_disable)
- [调试可能的驱动程序问题](#调试可能的驱动程序问题)
  - [启用驱动程序日志](#启用驱动程序日志)
  - [选择性启用特定驱动程序](#选择性启用特定驱动程序)

## 调试问题

如果您的应用程序崩溃或行为异常，加载器提供了几种机制来帮助您调试问题。

**注意**：此功能全部特定于桌面 Vulkan 加载器，不适用于 Android 加载器。

## 加载器日志

Vulkan 桌面加载器添加了日志功能，可以通过使用 `VK_LOADER_DEBUG` 环境变量来启用。
结果将输出到标准输出，但也会传递给任何存在的 `VK_EXT_debug_utils` 消息接收器。
该变量可以设置为逗号分隔的调试级别选项列表，包括：

  * error&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;报告加载器遇到的任何错误
  * warn&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;报告加载器遇到的任何警告
  * info&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;报告加载器生成的信息级别消息
  * debug&nbsp;&nbsp;&nbsp;&nbsp;报告加载器生成的调试级别消息
  * layer&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;报告加载器生成的所有层特定消息
  * driver&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;报告加载器生成的所有驱动程序特定消息
  * all&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;报告加载器生成的所有消息（包括上述所有内容）

如果您不确定问题来自哪里，至少将其设置为输出通过 "info" 级别的所有消息：

```
set VK_LOADER_DEBUG=error,warn,info
```

然后，您可以搜索列表以查找可能提供问题原因的提示的任何错误或警告。

有关启用加载器日志的更多信息，请参阅[启用加载器调试层输出](LoaderApplicationInterface.md#enable-loader-debug-layer-output)和下面的[调试环境变量表](LoaderInterfaceArchitecture.md#table-of-debug-environment-variables)。

## 调试可能的层问题

### 启用层日志

如果您怀疑层有问题，请将加载器日志设置为除了警告和错误之外还专门输出层消息：

```
set VK_LOADER_DEBUG=error,warn,layer
```

大多数重要的层消息应该在设置了错误或警告级别时输出，但这也会提供更多层特定信息，例如：
  * 找到了哪些层
  * 在哪里找到它们
  * 如果它们是隐式的，可以使用哪些环境变量来禁用它们
  * 如果与给定层存在任何不兼容性，这可能包括：
    * 找不到层库文件（.so/.dll）
    * 层库的位深度与执行应用程序不匹配（即 32 位与 64 位）
    * 层本身不支持应用程序所需的 Vulkan 版本
  * 是否有任何环境变量正在禁用任何层

例如，加载器查找隐式层的输出可能如下所示：

```
[Vulkan Loader] LAYER: Searching for implicit layer manifest files
[Vulkan Loader] LAYER:  In following locations:
[Vulkan Loader] LAYER:   /home/${USER}/.config/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /etc/xdg/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /usr/local/etc/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /etc/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/flatpak/exports/share/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /var/lib/flatpak/exports/share/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /usr/local/share/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:   /usr/share/vulkan/implicit_layer.d
[Vulkan Loader] LAYER:  Found the following files:
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/vulkan/implicit_layer.d/renderdoc_capture.json
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/vulkan/implicit_layer.d/steamfossilize_i386.json
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/vulkan/implicit_layer.d/steamfossilize_x86_64.json
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/vulkan/implicit_layer.d/steamoverlay_i386.json
[Vulkan Loader] LAYER:   /home/${USER}/.local/share/vulkan/implicit_layer.d/steamoverlay_x86_64.json
[Vulkan Loader] LAYER:   /usr/share/vulkan/implicit_layer.d/nvidia_layers.json
[Vulkan Loader] LAYER:   /usr/share/vulkan/implicit_layer.d/VkLayer_MESA_device_select.json
```

然后，层库的加载会类似于此报告：

```
[Vulkan Loader] DEBUG | LAYER : Loading layer library libVkLayer_khronos_validation.so
[Vulkan Loader] INFO | LAYER : Insert instance layer VK_LAYER_KHRONOS_validation (libVkLayer_khronos_validation.so)
[Vulkan Loader] DEBUG | LAYER : Loading layer library libVkLayer_MESA_device_select.so
[Vulkan Loader] INFO | LAYER : Insert instance layer VK_LAYER_MESA_device_select (libVkLayer_MESA_device_select.so)
```

最后，当创建 Vulkan 实例时，您可以从功能角度看到完整的实例调用链，输出如下：

```
[Vulkan Loader] LAYER: vkCreateInstance layer callstack setup to:
[Vulkan Loader] LAYER:  <Application>
[Vulkan Loader] LAYER:    ||
[Vulkan Loader] LAYER:  <Loader>
[Vulkan Loader] LAYER:    ||
[Vulkan Loader] LAYER:  VK_LAYER_MESA_device_select
[Vulkan Loader] LAYER:      Type: Implicit
[Vulkan Loader] LAYER:      Enabled By: Implicit Layer
[Vulkan Loader] LAYER:         Disable Env Var:  NODEVICE_SELECT
[Vulkan Loader] LAYER:      Manifest: /usr/share/vulkan/implicit_layer.d/VkLayer_MESA_device_select.json
[Vulkan Loader] LAYER:      Library:  libVkLayer_MESA_device_select.so
[Vulkan Loader] LAYER:    ||
[Vulkan Loader] LAYER:  VK_LAYER_KHRONOS_validation
[Vulkan Loader] LAYER:      Type: Explicit
[Vulkan Loader] LAYER:      Enabled By: By the Application
[Vulkan Loader] LAYER:      Manifest: /usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json
[Vulkan Loader] LAYER:      Library:  libVkLayer_khronos_validation.so
[Vulkan Loader] LAYER:    ||
[Vulkan Loader] LAYER:  <Drivers>
```

在这种情况下，使用了两个层（与之前加载的两个相同）：
* `VK_LAYER_MESA_device_select`
* `VK_LAYER_KHRONOS_validation`

此信息现在向我们显示 `VK_LAYER_MESA_device_select` 首先加载，然后是 `VK_LAYER_KHRONOS_validation`，然后继续进入任何可用的驱动程序。
它还显示 `VK_LAYER_MESA_device_select` 是一个隐式层，这意味着它不是由应用程序直接启用的。
另一方面，`VK_LAYER_KHRONOS_validation` 显示为显式层，这表明它可能是由应用程序启用的。

### 禁用层

**注意：** 此功能仅在构建时使用版本 1.3.234 或更高版本的 Vulkan 头文件的加载器中可用。

有时，隐式层可能会导致应用程序出现问题。
因此，下一步是尝试禁用列出的隐式层中的一个或多个。
您可以使用过滤环境变量（`VK_LOADER_LAYERS_ENABLE` 和 `VK_LOADER_LAYERS_DISABLE`）来选择性启用或禁用各种层。
如果您不确定该做什么，请尝试通过将 `VK_LOADER_LAYERS_DISABLE` 设置为 `~implicit~` 来手动禁用所有隐式层。

```
  set VK_LOADER_LAYERS_DISABLE=~implicit~
```

这将禁用所有隐式层，当启用层日志时，加载器将以以下方式向日志输出报告任何禁用的层：

```
[Vulkan Loader] WARNING | LAYER:  Implicit layer "VK_LAYER_MESA_device_select" forced disabled because name matches filter of env var 'VK_LOADER_LAYERS_DISABLE'.
[Vulkan Loader] WARNING | LAYER:  Implicit layer "VK_LAYER_AMD_switchable_graphics_64" forced disabled because name matches filter of env var 'VK_LOADER_LAYERS_DISABLE'.
[Vulkan Loader] WARNING | LAYER:  Implicit layer "VK_LAYER_Twitch_Overlay" forced disabled because name matches filter of env var 'VK_LOADER_LAYERS_DISABLE'.
```

### 选择性重新启用层

**注意：** 此功能仅在构建时使用版本 1.3.234 或更高版本的 Vulkan 头文件的加载器中可用。

在尝试诊断由层引起的问题时，首先禁用所有层，然后逐个重新启用每个层是有用的。
如果问题再次出现，那么立即清楚哪个层是问题的根源。

例如，从上面给出的禁用层列表中，让我们选择性地重新启用一个：

```
set VK_LOADER_LAYERS_DISABLE=~implicit~
set VK_LOADER_LAYERS_ENABLE=*AMD*
```

这将保持 "VK_LAYER_MESA_device_select" 和 "VK_LAYER_Twitch_Overlay" 层禁用，同时启用 "VK_LAYER_AMD_switchable_graphics_64" 层。
如果一切继续工作，那么证据似乎表明问题可能与 AMD 层无关。
这将导致启用另一个层并再次尝试：

```
set VK_LOADER_LAYERS_DISABLE=~implicit~
set VK_LOADER_LAYERS_ENABLE=*AMD*,*twitch*
```

依此类推。

有关如何使用过滤环境变量的更多信息，请参阅 [LoaderLayerInterface](LoaderLayerInterface.md) 文档的[层过滤](LoaderLayerInterface.md#layer-filtering)部分。

## 允许特定层忽略 VK_LOADER_LAYERS_DISABLE

**注意：** VK_LOADER_LAYERS_DISABLE 仅在构建时使用版本 1.3.262 或更高版本的 Vulkan 头文件的加载器中可用。

当使用 `VK_LOADER_LAYERS_DISABLE` 禁用隐式层时，可以使用 `VK_LOADER_LAYERS_ENABLE` 允许特定层启用。
但是，这具有*强制*启用层的效果，这并不总是期望的。
隐式层具有仅在设置了层指定的环境变量时才启用的能力，允许上下文相关的启用。
`VK_LOADER_LAYERS_ENABLE` 忽略了该上下文。

因此，需要一个不同的环境变量：`VK_LOADER_LAYERS_ALLOW`

`VK_LOADER_LAYERS_ALLOW` 的行为类似于 `VK_LOADER_LAYERS_ENABLE`，只是它不强制启用层。
思考此环境变量的方式是，匹配 `VK_LOADER_LAYERS_ALLOW` 的每个层都被排除在由 `VK_LOADER_LAYERS_DISABLE` 强制禁用之外。
这允许依赖于上下文的隐式层根据相关上下文启用，而不是强制启用它们。

示例：禁用所有隐式层，除了名称中包含 steam 或 mesa 的任何层。
```
set VK_LOADER_LAYERS_DISABLE=~implicit~
set VK_LOADER_LAYERS_ALLOW=*steam*,*Mesa*
```

## 调试可能的驱动程序问题

### 启用驱动程序日志

**注意：** 此功能仅在构建时使用版本 1.3.234 或更高版本的 Vulkan 头文件的加载器中可用。

如果您怀疑驱动程序有问题，请将加载器日志设置为专门输出驱动程序消息：

```
set VK_LOADER_DEBUG=error,warn,driver
```

大多数重要的驱动程序消息应该在设置了错误或警告级别时输出，但这也会提供更多驱动程序特定信息，例如：
  * 找到了哪些驱动程序
  * 在哪里找到它们
  * 如果与给定驱动程序存在任何不兼容性
  * 是否有任何环境变量正在禁用任何驱动程序

例如，加载器在 Linux 系统上查找驱动程序的输出可能如下所示（注意：为了便于阅读，已从输出中删除了额外的空格）：

```
[Vulkan Loader] DRIVER: Searching for driver manifest files
[Vulkan Loader] DRIVER:    In following folders:
[Vulkan Loader] DRIVER:       /home/$(USER)/.config/vulkan/icd.d
[Vulkan Loader] DRIVER:       /etc/xdg/vulkan/icd.d
[Vulkan Loader] DRIVER:       /etc/vulkan/icd.d
[Vulkan Loader] DRIVER:       /home/$(USER)/.local/share/vulkan/icd.d
[Vulkan Loader] DRIVER:       /home/$(USER)/.local/share/flatpak/exports/share/vulkan/icd.d
[Vulkan Loader] DRIVER:       /var/lib/flatpak/exports/share/vulkan/icd.d
[Vulkan Loader] DRIVER:       /usr/local/share/vulkan/icd.d
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d
[Vulkan Loader] DRIVER:    Found the following files:
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/intel_icd.x86_64.json
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/lvp_icd.x86_64.json
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/radeon_icd.x86_64.json
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/lvp_icd.i686.json
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/radeon_icd.i686.json
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/intel_icd.i686.json
[Vulkan Loader] DRIVER:       /usr/share/vulkan/icd.d/nvidia_icd.json
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/intel_icd.x86_64.json, version "1.0.0"
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/lvp_icd.x86_64.json, version "1.0.0"
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/radeon_icd.x86_64.json, version "1.0.0"
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/lvp_icd.i686.json, version "1.0.0"
[Vulkan Loader] DRIVER: Requested driver /usr/lib/libvulkan_lvp.so was wrong bit-type. Ignoring this JSON
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/radeon_icd.i686.json, version "1.0.0"
[Vulkan Loader] DRIVER: Requested driver /usr/lib/libvulkan_radeon.so was wrong bit-type. Ignoring this JSON
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/intel_icd.i686.json, version "1.0.0"
[Vulkan Loader] DRIVER: Requested driver /usr/lib/libvulkan_intel.so was wrong bit-type. Ignoring this JSON
[Vulkan Loader] DRIVER: Found ICD manifest file /usr/share/vulkan/icd.d/nvidia_icd.json, version "1.0.0"
```

然后，当应用程序选择要使用的设备时，您将看到以以下方式报告的 Vulkan 设备调用链（注意：为了便于阅读，已从输出中删除了额外的空格）：

```
[Vulkan Loader] DRIVER: vkCreateDevice layer callstack setup to:
[Vulkan Loader] DRIVER:    <Application>
[Vulkan Loader] DRIVER:      ||
[Vulkan Loader] DRIVER:    <Loader>
[Vulkan Loader] DRIVER:      ||
[Vulkan Loader] DRIVER:    <Device>
[Vulkan Loader] DRIVER:        Using "Intel(R) UHD Graphics 630 (CFL GT2)" with driver: "/usr/lib64/libvulkan_intel.so"
```

### 选择性启用特定驱动程序

**注意：** 此功能仅在构建时使用版本 1.3.234 或更高版本的 Vulkan 头文件的加载器中可用。

您现在可以使用过滤环境变量（`VK_LOADER_DRIVERS_SELECT` 和 `VK_LOADER_DRIVERS_DISABLE`）来控制加载器将尝试加载哪些驱动程序。
对于驱动程序，传递给上述环境变量的字符串 glob 将与驱动程序 JSON 文件名进行比较，因为在 Vulkan 初始化过程的后期之前，加载器不知道驱动程序名称。

例如，要禁用除 Nvidia 之外的所有驱动程序，您可以执行以下操作：

```
set VK_LOADER_DRIVERS_DISABLE=*
set VK_LOADER_DRIVERS_SELECT=*nvidia*
```

当使用环境变量时，加载器会输出如下消息：

```
[Vulkan Loader] WARNING | DRIVER: Driver "intel_icd.x86_64.json" ignored because not selected by env var 'VK_LOADER_DRIVERS_SELECT'
[Vulkan Loader] WARNING | DRIVER: Driver "radeon_icd.x86_64.json" ignored because it was disabled by env var 'VK_LOADER_DRIVERS_DISABLE'
```

有关如何使用过滤环境变量的更多信息，请参阅 [LoaderDriverInterface](LoaderDriverInterface.md) 文档的[驱动程序过滤](LoaderDriverInterface.md#driver-filtering)部分。

