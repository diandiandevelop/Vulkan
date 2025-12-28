<!-- markdownlint-disable MD041 -->
[![Khronos Vulkan][1]][2]

[1]: images/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

# 加载器设置文件 <!-- omit from toc -->

[![Creative Commons][3]][4]

<!-- Copyright &copy; 2025 LunarG, Inc. -->

[3]: images/Creative_Commons_88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/


## 目录 <!-- omit from toc -->

- [设置文件的目的](#设置文件的目的)
- [设置文件发现](#设置文件发现)
  - [Windows](#windows)
  - [Linux/MacOS/BSD/QNX/Fuchsia/GNU](#linuxmacosbsdqnxfuchsiagnu)
  - [其他平台](#其他平台)
  - [提升权限的例外情况](#提升权限的例外情况)
- [每应用程序设置文件](#每应用程序设置文件)
- [文件格式](#文件格式)
- [示例设置文件](#示例设置文件)
  - [字段](#字段)
- [行为](#行为)


## 设置文件的目的

加载器设置文件的目的是为开发人员提供对 Vulkan-Loader 行为的出色控制。
它能够增强对加载哪些层、调用链中层的顺序、日志记录以及哪些驱动程序可用的控制。

加载器设置文件旨在供 Vulkan API 的"开发人员控制面板"使用，例如 Vulkan 配置器，作为设置调试环境变量的替代方案。

## 设置文件发现

加载器设置文件通过搜索特定文件系统路径或通过平台特定机制（如 Windows 注册表）来定位。

### Windows

Vulkan 加载器首先搜索注册表项 HKEY_CURRENT_USER\SOFTWARE\Khronos\Vulkan\LoaderSettings，查找名称是名为 'vk_loader_settings.json' 的文件的有效路径的 DWORD 值。
如果没有匹配的值或文件不存在，Vulkan 加载器将对注册表项 HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\LoaderSettings 执行与上述相同的行为。

### Linux/MacOS/BSD/QNX/Fuchsia/GNU

加载器设置文件通过在以下位置搜索名为 vk_loader_settings.json 的文件来定位：

`$HOME/.local/share/vulkan/loader_settings.d/`
`$XDG_DATA_HOME/vulkan/loader_settings.d/`
`/etc/vulkan/loader_settings.d/`

其中 $HOME 和 %XDG_DATA_HOME 指的是同名环境变量中包含的值。
如果给定的环境变量不存在，则忽略该路径。

### 其他平台

由于没有适当的搜索机制，上面未列出的平台目前不支持加载器设置文件。

### 提升权限的例外情况

由于加载器设置文件包含指向层和 ICD 清单的路径，这些清单包含指向各种可执行二进制文件的路径，因此有必要在应用程序以提升权限运行时限制加载器设置文件的使用。

这是通过不使用在非特权位置找到的任何加载器设置文件来实现的。

在 Windows 上，以提升权限运行将忽略 HKEY_CURRENT_USER\SOFTWARE\Khronos\Vulkan\LoaderSettings。

在 Linux/MacOS/BSD/QNX/Fuchsia/GNU 上，以提升权限运行将使用安全方法查询 $HOME 和 $XDG_DATA_HOME，以防止恶意注入不安全的搜索目录。

## 每应用程序设置文件

## 文件格式

加载器设置文件是一个 JSON 文件，具有


## 示例设置文件


```json
{
   "file_format_version" : "1.0.1",
   "settings": {

   }
}
```

### 字段

<table style="width:100%">
  <tr>
    <th>JSON 节点</th>
    <th>描述和注释</th>
    <th>限制</th>
    <th>父节点</th>
    <th>内省查询</th>
  </tr>




## 行为

