/*
* View frustum culling class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <array>
#include <math.h>
#include <glm/glm.hpp>

namespace vks
{
	/**
	 * @brief 视锥剔除类
	 * 用于计算视锥平面并进行球体剔除测试
	 */
	class Frustum
	{
	public:
		enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };  // 视锥面枚举
		std::array<glm::vec4, 6> planes;  // 6个视锥平面（每个平面用 vec4 表示，xyz 为法线，w 为距离）

		/**
		 * @brief 从视图投影矩阵更新视锥平面
		 * 从视图投影矩阵提取 6 个视锥平面方程（左、右、上、下、前、后）
		 * 
		 * @param matrix 视图投影矩阵（通常是 view * projection）
		 * 
		 * 算法说明：
		 * 视锥平面可以通过视图投影矩阵的列向量组合得到。
		 * 每个平面用平面方程 ax + by + cz + d = 0 表示，存储为 vec4(a, b, c, d)。
		 * 平面的法线向量为 (a, b, c)，d 为到原点的距离。
		 */
		void update(glm::mat4 matrix)
		{
			// 提取左平面：matrix[3] + matrix[0]
			// 左平面由矩阵的第 3 列（齐次坐标列）和第 0 列（X 轴列）相加得到
			planes[LEFT].x = matrix[0].w + matrix[0].x;
			planes[LEFT].y = matrix[1].w + matrix[1].x;
			planes[LEFT].z = matrix[2].w + matrix[2].x;
			planes[LEFT].w = matrix[3].w + matrix[3].x;

			// 提取右平面：matrix[3] - matrix[0]
			// 右平面由矩阵的第 3 列和第 0 列相减得到
			planes[RIGHT].x = matrix[0].w - matrix[0].x;
			planes[RIGHT].y = matrix[1].w - matrix[1].x;
			planes[RIGHT].z = matrix[2].w - matrix[2].x;
			planes[RIGHT].w = matrix[3].w - matrix[3].x;

			// 提取上平面：matrix[3] - matrix[1]
			// 上平面由矩阵的第 3 列和第 1 列（Y 轴列）相减得到
			planes[TOP].x = matrix[0].w - matrix[0].y;
			planes[TOP].y = matrix[1].w - matrix[1].y;
			planes[TOP].z = matrix[2].w - matrix[2].y;
			planes[TOP].w = matrix[3].w - matrix[3].y;

			// 提取下平面：matrix[3] + matrix[1]
			// 下平面由矩阵的第 3 列和第 1 列相加得到
			planes[BOTTOM].x = matrix[0].w + matrix[0].y;
			planes[BOTTOM].y = matrix[1].w + matrix[1].y;
			planes[BOTTOM].z = matrix[2].w + matrix[2].y;
			planes[BOTTOM].w = matrix[3].w + matrix[3].y;

			// 提取后平面（远裁剪面）：matrix[3] + matrix[2]
			// 后平面由矩阵的第 3 列和第 2 列（Z 轴列）相加得到
			planes[BACK].x = matrix[0].w + matrix[0].z;
			planes[BACK].y = matrix[1].w + matrix[1].z;
			planes[BACK].z = matrix[2].w + matrix[2].z;
			planes[BACK].w = matrix[3].w + matrix[3].z;

			// 提取前平面（近裁剪面）：matrix[3] - matrix[2]
			// 前平面由矩阵的第 3 列和第 2 列相减得到
			planes[FRONT].x = matrix[0].w - matrix[0].z;
			planes[FRONT].y = matrix[1].w - matrix[1].z;
			planes[FRONT].z = matrix[2].w - matrix[2].z;
			planes[FRONT].w = matrix[3].w - matrix[3].z;

			// 归一化所有平面
			// 将平面法线向量归一化，使平面方程标准化
			for (auto i = 0; i < planes.size(); i++)
			{
				// 计算法线向量的长度
				float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
				// 归一化平面（包括距离项）
				planes[i] /= length;
			}
		}
		
		/**
		 * @brief 检查球体是否在视锥内
		 * 使用球体-平面测试进行视锥剔除
		 * 
		 * @param pos 球心位置（世界空间坐标）
		 * @param radius 球体半径
		 * 
		 * @return 如果球体在视锥内（或与视锥相交）返回 true，如果完全在视锥外返回 false
		 * 
		 * 算法说明：
		 * 对于每个视锥平面，计算球心到平面的有符号距离。
		 * 如果距离小于 -radius，说明球体完全在平面的负半空间（视锥外），可以剔除。
		 * 如果所有平面的测试都通过，说明球体在视锥内或与视锥相交。
		 */
		bool checkSphere(glm::vec3 pos, float radius)
		{
			// 遍历所有 6 个视锥平面
			for (auto i = 0; i < planes.size(); i++)
			{
				// 计算球心到平面的有符号距离
				// 平面方程：ax + by + cz + d = 0
				// 点到平面的距离：ax + by + cz + d
				float distance = (planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z) + planes[i].w;
				
				// 如果距离小于 -radius，说明球体完全在平面的负半空间（视锥外）
				// 可以安全地剔除该球体
				if (distance <= -radius)
				{
					return false;  // 球体在视锥外，剔除
				}
			}
			// 所有平面测试都通过，球体在视锥内或与视锥相交
			return true;
		}
	};
}
