/*
* Benchmark class - Can be used for automated benchmarks
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <functional>
#include <chrono>
#include <iomanip>

namespace vks
{
	/**
	 * @brief 基准测试类，用于自动化性能测试
	 */
	class Benchmark {
	private:
		FILE* stream{ nullptr };                    // 输出流（Windows 控制台）
		VkPhysicalDeviceProperties deviceProps{};   // 物理设备属性
	public:
		bool active = false;                        // 是否正在运行基准测试
		bool outputFrameTimes = false;              // 是否输出每帧时间
		int outputFrames = -1;                      // 输出帧数限制（-1 表示无限制）
		uint32_t warmup = 1;                        // 预热时间（秒），默认 1 秒
		uint32_t duration = 10;                    // 测试持续时间（秒）
		std::vector<double> frameTimes;             // 每帧时间列表（毫秒）
		std::string filename = "";                  // 结果文件名

		double runtime = 0.0;                      // 运行时间（毫秒）
		uint32_t frameCount = 0;                    // 帧计数

		/**
		 * @brief 运行基准测试
		 * @param renderFunc 渲染函数（每帧调用）
		 * @param deviceProps 物理设备属性
		 */
		void run(std::function<void()> renderFunc, VkPhysicalDeviceProperties deviceProps) {
			active = true;
			this->deviceProps = deviceProps;
#if defined(_WIN32)
			bool consoleAttached = AttachConsole(ATTACH_PARENT_PROCESS);
			if (!consoleAttached) {
				consoleAttached = AttachConsole(GetCurrentProcessId());
			}
			if (consoleAttached) {
				freopen_s(&stream, "CONOUT$", "w+", stdout);
				freopen_s(&stream, "CONOUT$", "w+", stderr);
			}
#endif
			// Warm up phase to get more stable frame rates
			// 预热阶段以获得更稳定的帧率
			{
				double tMeasured = 0.0;
				while (tMeasured < (warmup * 1000)) {
					auto tStart = std::chrono::high_resolution_clock::now();
					renderFunc();
					auto tDiff = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
					tMeasured += tDiff;
				};
			}

			// Benchmark phase
			// 基准测试阶段
			{
				while (runtime < (duration * 1000.0)) {
					auto tStart = std::chrono::high_resolution_clock::now();
					renderFunc();
					auto tDiff = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
					runtime += tDiff;
					frameTimes.push_back(tDiff);
					frameCount++;
					if (outputFrames != -1 && outputFrames == frameCount) break;
				};
				std::cout << std::fixed << std::setprecision(3);
				std::cout << "Benchmark finished\n";
				std::cout << "device : " << deviceProps.deviceName << " (driver version: " << deviceProps.driverVersion << ")" << "\n";
				std::cout << "runtime: " << (runtime / 1000.0) << "\n";
				std::cout << "frames : " << frameCount << "\n";
				std::cout << "fps    : " << frameCount / (runtime / 1000.0) << "\n";
			}
		}

		/**
		 * @brief 保存基准测试结果到文件
		 */
		void saveResults() {
			std::ofstream result(filename, std::ios::out);
			if (result.is_open()) {
				result << std::fixed << std::setprecision(4);

				// 写入 CSV 格式的汇总数据
				result << "device,driverversion,duration (ms),frames,fps" << "\n";
				result << deviceProps.deviceName << "," << deviceProps.driverVersion << "," << runtime << "," << frameCount << "," << frameCount / (runtime / 1000.0) << "\n";

				if (outputFrameTimes) {
					// 写入每帧时间数据
					result << "\n" << "frame,ms" << "\n";
					for (size_t i = 0; i < frameTimes.size(); i++) {
						result << i << "," << frameTimes[i] << "\n";
					}
					// 计算并输出统计信息
					double tMin = *std::min_element(frameTimes.begin(), frameTimes.end());
					double tMax = *std::max_element(frameTimes.begin(), frameTimes.end());
					double tAvg = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / (double)frameTimes.size();
					std::cout << "best   : " << (1000.0 / tMin) << " fps (" << tMin << " ms)" << "\n";
					std::cout << "worst  : " << (1000.0 / tMax) << " fps (" << tMax << " ms)" << "\n";
					std::cout << "avg    : " << (1000.0 / tAvg) << " fps (" << tAvg << " ms)" << "\n";
					std::cout << "\n";
				}

				result.flush();
#if defined(_WIN32)
				FreeConsole();
#endif
			}
		}
	};
}