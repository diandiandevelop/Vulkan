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
		 * @param matrix 视图投影矩阵（通常是 view * projection）
		 */
		void update(glm::mat4 matrix)
		{
			planes[LEFT].x = matrix[0].w + matrix[0].x;
			planes[LEFT].y = matrix[1].w + matrix[1].x;
			planes[LEFT].z = matrix[2].w + matrix[2].x;
			planes[LEFT].w = matrix[3].w + matrix[3].x;

			planes[RIGHT].x = matrix[0].w - matrix[0].x;
			planes[RIGHT].y = matrix[1].w - matrix[1].x;
			planes[RIGHT].z = matrix[2].w - matrix[2].x;
			planes[RIGHT].w = matrix[3].w - matrix[3].x;

			planes[TOP].x = matrix[0].w - matrix[0].y;
			planes[TOP].y = matrix[1].w - matrix[1].y;
			planes[TOP].z = matrix[2].w - matrix[2].y;
			planes[TOP].w = matrix[3].w - matrix[3].y;

			planes[BOTTOM].x = matrix[0].w + matrix[0].y;
			planes[BOTTOM].y = matrix[1].w + matrix[1].y;
			planes[BOTTOM].z = matrix[2].w + matrix[2].y;
			planes[BOTTOM].w = matrix[3].w + matrix[3].y;

			planes[BACK].x = matrix[0].w + matrix[0].z;
			planes[BACK].y = matrix[1].w + matrix[1].z;
			planes[BACK].z = matrix[2].w + matrix[2].z;
			planes[BACK].w = matrix[3].w + matrix[3].z;

			planes[FRONT].x = matrix[0].w - matrix[0].z;
			planes[FRONT].y = matrix[1].w - matrix[1].z;
			planes[FRONT].z = matrix[2].w - matrix[2].z;
			planes[FRONT].w = matrix[3].w - matrix[3].z;

			for (auto i = 0; i < planes.size(); i++)
			{
				float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
				planes[i] /= length;
			}
		}
		
		/**
		 * @brief 检查球体是否在视锥内
		 * @param pos 球心位置
		 * @param radius 球体半径
		 * @return 如果球体在视锥内返回 true，否则返回 false
		 */
		bool checkSphere(glm::vec3 pos, float radius)
		{
			for (auto i = 0; i < planes.size(); i++)
			{
				// 计算点到平面的距离，如果距离小于 -radius，说明球体完全在平面外
				if ((planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z) + planes[i].w <= -radius)
				{
					return false;
				}
			}
			return true;
		}
	};
}
