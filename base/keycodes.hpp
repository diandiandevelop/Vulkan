/*
* Key codes for multiple platforms
* 多平台键码定义
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#if defined(_WIN32)
// Windows 平台键码定义
#define KEY_ESCAPE VK_ESCAPE  // ESC 键
#define KEY_F1 VK_F1          // F1 功能键
#define KEY_F2 VK_F2          // F2 功能键
#define KEY_F3 VK_F3          // F3 功能键
#define KEY_F4 VK_F4          // F4 功能键
#define KEY_F5 VK_F5          // F5 功能键
#define KEY_W 0x57            // W 键
#define KEY_A 0x41            // A 键
#define KEY_S 0x53            // S 键
#define KEY_D 0x44            // D 键
#define KEY_P 0x50            // P 键
#define KEY_SPACE 0x20        // 空格键
#define KEY_KPADD 0x6B        // 小键盘加号键
#define KEY_KPSUB 0x6D        // 小键盘减号键
#define KEY_B 0x42            // B 键
#define KEY_F 0x46            // F 键
#define KEY_L 0x4C            // L 键
#define KEY_N 0x4E            // N 键
#define KEY_O 0x4F            // O 键
#define KEY_T 0x54            // T 键

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
// Android 平台键码定义
#define GAMEPAD_BUTTON_A 0x1000      // 游戏手柄 A 键
#define GAMEPAD_BUTTON_B 0x1001      // 游戏手柄 B 键
#define GAMEPAD_BUTTON_X 0x1002      // 游戏手柄 X 键
#define GAMEPAD_BUTTON_Y 0x1003      // 游戏手柄 Y 键
#define GAMEPAD_BUTTON_L1 0x1004     // 游戏手柄左肩键 L1
#define GAMEPAD_BUTTON_R1 0x1005     // 游戏手柄右肩键 R1
#define GAMEPAD_BUTTON_START 0x1006  // 游戏手柄开始键
#define TOUCH_DOUBLE_TAP 0x1100      // 触摸双击事件

// for textoverlay example
// 用于文本叠加层示例
#define KEY_SPACE 0x3E		// AKEYCODE_SPACE - 空格键
#define KEY_KPADD 0x9D		// AKEYCODE_NUMPAD_ADD - 小键盘加号键

#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
#if !defined(VK_EXAMPLE_XCODE_GENERATED)
// For iOS and macOS pre-configured Xcode example project: Use character keycodes
// - Use numeric keys as optional alternative to function keys
// 对于 iOS 和 macOS 预配置的 Xcode 示例项目：使用字符键码
// - 使用数字键作为功能键的可选替代
#define KEY_DELETE 0x7F      // 删除键
#define KEY_ESCAPE 0x1B      // ESC 键
#define KEY_F1 0xF704		// NSF1FunctionKey - F1 功能键
#define KEY_F2 0xF705		// NSF2FunctionKey - F2 功能键
#define KEY_F3 0xF706		// NSF3FunctionKey - F3 功能键
#define KEY_F4 0xF707		// NSF4FunctionKey - F4 功能键
#define KEY_1 '1'            // 数字键 1
#define KEY_2 '2'            // 数字键 2
#define KEY_3 '3'            // 数字键 3
#define KEY_4 '4'            // 数字键 4
#define KEY_W 'w'            // W 键
#define KEY_A 'a'            // A 键
#define KEY_S 's'            // S 键
#define KEY_D 'd'            // D 键
#define KEY_P 'p'            // P 键
#define KEY_SPACE ' '        // 空格键
#define KEY_KPADD '+'        // 小键盘加号键
#define KEY_KPSUB '-'        // 小键盘减号键
#define KEY_B 'b'            // B 键
#define KEY_F 'f'            // F 键
#define KEY_L 'l'            // L 键
#define KEY_N 'n'            // N 键
#define KEY_O 'o'            // O 键
#define KEY_Q 'q'            // Q 键
#define KEY_T 't'            // T 键
#define KEY_Z 'z'            // Z 键

#else // defined(VK_EXAMPLE_XCODE_GENERATED)
// For cross-platform cmake-generated Xcode project: Use ANSI keyboard keycodes
// - Use numeric keys as optional alternative to function keys
// - Use main keyboard plus/minus instead of keypad plus/minus
// 对于跨平台 cmake 生成的 Xcode 项目：使用 ANSI 键盘键码
// - 使用数字键作为功能键的可选替代
// - 使用主键盘的加号/减号而不是小键盘的加号/减号
#include <Carbon/Carbon.h>
#define KEY_DELETE kVK_Delete        // 删除键
#define KEY_ESCAPE kVK_Escape         // ESC 键
#define KEY_F1 kVK_F1                 // F1 功能键
#define KEY_F2 kVK_F2                 // F2 功能键
#define KEY_F3 kVK_F3                 // F3 功能键
#define KEY_F4 kVK_F4                 // F4 功能键
#define KEY_1 kVK_ANSI_1             // ANSI 数字键 1
#define KEY_2 kVK_ANSI_2             // ANSI 数字键 2
#define KEY_3 kVK_ANSI_3             // ANSI 数字键 3
#define KEY_4 kVK_ANSI_4             // ANSI 数字键 4
#define KEY_W kVK_ANSI_W             // ANSI W 键
#define KEY_A kVK_ANSI_A             // ANSI A 键
#define KEY_S kVK_ANSI_S             // ANSI S 键
#define KEY_D kVK_ANSI_D             // ANSI D 键
#define KEY_P kVK_ANSI_P             // ANSI P 键
#define KEY_SPACE kVK_Space          // 空格键
#define KEY_KPADD kVK_ANSI_Equal     // ANSI 等号键（用作加号）
#define KEY_KPSUB kVK_ANSI_Minus     // ANSI 减号键
#define KEY_B kVK_ANSI_B             // ANSI B 键
#define KEY_F kVK_ANSI_F             // ANSI F 键
#define KEY_L kVK_ANSI_L             // ANSI L 键
#define KEY_N kVK_ANSI_N             // ANSI N 键
#define KEY_O kVK_ANSI_O             // ANSI O 键
#define KEY_Q kVK_ANSI_Q             // ANSI Q 键
#define KEY_T kVK_ANSI_T             // ANSI T 键
#define KEY_Z kVK_ANSI_Z             // ANSI Z 键
#endif

#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
// DirectFB 平台键码定义
#define KEY_ESCAPE DIKS_ESCAPE        // ESC 键
#define KEY_F1 DIKS_F1                // F1 功能键
#define KEY_F2 DIKS_F2                // F2 功能键
#define KEY_F3 DIKS_F3                // F3 功能键
#define KEY_F4 DIKS_F4                // F4 功能键
#define KEY_W DIKS_SMALL_W            // 小写 W 键
#define KEY_A DIKS_SMALL_A            // 小写 A 键
#define KEY_S DIKS_SMALL_S            // 小写 S 键
#define KEY_D DIKS_SMALL_D            // 小写 D 键
#define KEY_P DIKS_SMALL_P            // 小写 P 键
#define KEY_SPACE DIKS_SPACE          // 空格键
#define KEY_KPADD DIKS_PLUS_SIGN      // 加号键
#define KEY_KPSUB DIKS_MINUS_SIGN     // 减号键
#define KEY_B DIKS_SMALL_B            // 小写 B 键
#define KEY_F DIKS_SMALL_F            // 小写 F 键
#define KEY_L DIKS_SMALL_L            // 小写 L 键
#define KEY_N DIKS_SMALL_N            // 小写 N 键
#define KEY_O DIKS_SMALL_O            // 小写 O 键
#define KEY_T DIKS_SMALL_T            // 小写 T 键

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
// Wayland 平台键码定义
#include <linux/input.h>

// todo: hack for bloom example
// TODO: 为 bloom 示例的临时解决方案
#define KEY_ESCAPE KEY_ESC        // ESC 键
#define KEY_KPADD KEY_KPPLUS      // 小键盘加号键
#define KEY_KPSUB KEY_KPMINUS     // 小键盘减号键

#elif defined(__linux__) || defined(__FreeBSD__)
// Linux / FreeBSD 平台键码定义
#define KEY_ESCAPE 0x9     // ESC 键
#define KEY_F1 0x43        // F1 功能键
#define KEY_F2 0x44        // F2 功能键
#define KEY_F3 0x45        // F3 功能键
#define KEY_F4 0x46        // F4 功能键
#define KEY_W 0x19         // W 键
#define KEY_A 0x26         // A 键
#define KEY_S 0x27         // S 键
#define KEY_D 0x28         // D 键
#define KEY_P 0x21         // P 键
#define KEY_SPACE 0x41     // 空格键
#define KEY_KPADD 0x56     // 小键盘加号键
#define KEY_KPSUB 0x52     // 小键盘减号键
#define KEY_B 0x38         // B 键
#define KEY_F 0x29         // F 键
#define KEY_L 0x2E         // L 键
#define KEY_N 0x39         // N 键
#define KEY_O 0x20         // O 键
#define KEY_T 0x1C         // T 键

#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
// QNX Screen 平台键码定义
#include <sys/keycodes.h>

#define KEY_ESCAPE KEYCODE_ESCAPE     // ESC 键
#define KEY_F1     KEYCODE_F1         // F1 功能键
#define KEY_F2     KEYCODE_F2         // F2 功能键
#define KEY_F3     KEYCODE_F3         // F3 功能键
#define KEY_F4     KEYCODE_F4         // F4 功能键
#define KEY_W      KEYCODE_W           // W 键
#define KEY_A      KEYCODE_A           // A 键
#define KEY_S      KEYCODE_S           // S 键
#define KEY_D      KEYCODE_D           // D 键
#define KEY_P      KEYCODE_P           // P 键
#define KEY_SPACE  KEYCODE_SPACE      // 空格键
#define KEY_KPADD  KEYCODE_KP_PLUS    // 小键盘加号键
#define KEY_KPSUB  KEYCODE_KP_MINUS   // 小键盘减号键
#define KEY_B      KEYCODE_B           // B 键
#define KEY_F      KEYCODE_F           // F 键
#define KEY_L      KEYCODE_L           // L 键
#define KEY_N      KEYCODE_N           // N 键
#define KEY_O      KEYCODE_O           // O 键
#define KEY_T      KEYCODE_T           // T 键

#endif
