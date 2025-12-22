/*
 * Simple command line parse
 *
 * Copyright (C) 2016-2022 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <string>

/**
 * @brief 简单的命令行解析器类
 */
class CommandLineParser
{
public:
	/**
	 * @brief 命令行选项结构
	 */
	struct CommandLineOption {
		std::vector<std::string> commands;  // 命令字符串列表（如 ["-h", "--help"]）
		std::string value;                  // 选项值（如果有）
		bool hasValue = false;              // 是否需要值
		std::string help;                   // 帮助文本
		bool set = false;                   // 是否已设置
	};
	std::unordered_map<std::string, CommandLineOption> options;  // 选项映射表

	/**
	 * @brief 添加命令行选项
	 * @param name 选项名称（内部标识）
	 * @param commands 命令字符串列表（如 ["-h", "--help"]）
	 * @param hasValue 是否需要值
	 * @param help 帮助文本
	 */
	void add(std::string name, std::vector<std::string> commands, bool hasValue, std::string help)
	{
		options[name].commands = commands;  // 设置命令列表
		options[name].help = help;          // 设置帮助文本
		options[name].set = false;          // 初始化为未设置
		options[name].hasValue = hasValue;   // 设置是否需要值
		options[name].value = "";            // 初始化值为空
	}

	/**
	 * @brief 打印帮助信息
	 */
	void printHelp()
	{
		std::cout << "Available command line options:\n";  // 输出标题
		for (auto option : options) {  // 遍历所有选项
			std::cout << " ";  // 输出缩进
			for (size_t i = 0; i < option.second.commands.size(); i++) {  // 遍历命令列表
				std::cout << option.second.commands[i];  // 输出命令
				if (i < option.second.commands.size() - 1) {  // 如果不是最后一个命令
					std::cout << ", ";  // 输出逗号和空格
				}
			}
			std::cout << ": " << option.second.help << "\n";  // 输出帮助文本
		}
		std::cout << "Press any key to close...";  // 输出提示信息
	}

	/**
	 * @brief 解析命令行参数（字符串向量版本）
	 * @param arguments 参数字符串向量
	 */
	void parse(std::vector<const char*> arguments)
	{
		bool printHelp = false;  // 是否需要打印帮助
		// Known arguments
		// 已知参数
		for (auto& option : options) {  // 遍历所有选项
			for (auto& command : option.second.commands) {  // 遍历选项的命令列表
				for (size_t i = 0; i < arguments.size(); i++) {  // 遍历参数字符串
					if (strcmp(arguments[i], command.c_str()) == 0) {  // 如果找到匹配的命令
						option.second.set = true;  // 标记选项已设置
						// Get value
						// 获取值
						if (option.second.hasValue) {  // 如果选项需要值
							if (arguments.size() > i + 1) {  // 如果还有下一个参数
								option.second.value = arguments[i + 1];  // 获取下一个参数作为值
							}
							if (option.second.value == "") {  // 如果值为空
								printHelp = true;  // 标记需要打印帮助
								break;  // 跳出循环
							}
						}
					}
				}
			}
		}
		// Print help for unknown arguments or missing argument values
		// 为未知参数或缺少参数值打印帮助
		if (printHelp) {  // 如果需要打印帮助
			options["help"].set = true;  // 设置 help 选项为已设置
		}
	}

	/**
	 * @brief 解析命令行参数（argc/argv 版本）
	 * @param argc 参数数量
	 * @param argv 参数字符串数组
	 */
	void parse(int argc, char* argv[])
	{
		std::vector<const char*> args;  // 创建参数字符串向量
		for (int i = 0; i < argc; i++) {  // 遍历所有参数
			args.push_back(argv[i]);  // 添加到向量
		};
		parse(args);  // 调用向量版本解析函数
	}

	/**
	 * @brief 检查选项是否已设置
	 * @param name 选项名称
	 * @return 如果已设置返回 true
	 */
	bool isSet(std::string name)
	{
		return ((options.find(name) != options.end()) && options[name].set);  // 检查选项是否存在且已设置
	}

	/**
	 * @brief 获取选项值（字符串）
	 * @param name 选项名称
	 * @param defaultValue 默认值
	 * @return 选项值或默认值
	 */
	std::string getValueAsString(std::string name, std::string defaultValue)
	{
		assert(options.find(name) != options.end());  // 断言选项存在
		std::string value = options[name].value;  // 获取选项值
		return (value != "") ? value : defaultValue;  // 如果值不为空返回值，否则返回默认值
	}

	/**
	 * @brief 获取选项值（整数）
	 * @param name 选项名称
	 * @param defaultValue 默认值
	 * @return 选项值或默认值
	 */
	int32_t getValueAsInt(std::string name, int32_t defaultValue)
	{
		assert(options.find(name) != options.end());  // 断言选项存在
		std::string value = options[name].value;  // 获取选项值
		if (value != "") {  // 如果值不为空
			char* numConvPtr;  // 数字转换指针（用于检查转换是否成功）
			int32_t intVal = (int32_t)strtol(value.c_str(), &numConvPtr, 10);  // 将字符串转换为整数（十进制）
			return (intVal > 0) ? intVal : defaultValue;  // 如果转换值大于 0 返回转换值，否则返回默认值
		}
		else {  // 如果值为空
			return defaultValue;  // 返回默认值
		}
		return int32_t();  // 永远不会执行（代码冗余）
	}

};