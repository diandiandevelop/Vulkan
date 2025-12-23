/*
* Basic camera class providing a look-at and first-person camera
*
* Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief 基础相机类，提供观察和第一人称相机功能
 */
class Camera
{
private:
	float fov;          // 视野角度（度）
	float znear, zfar;  // 近裁剪面和远裁剪面距离

	/**
	 * @brief 更新视图矩阵
	 */
	void updateViewMatrix()
	{
		glm::mat4 currentMatrix = matrices.view;

		glm::mat4 rotM = glm::mat4(1.0f);
		glm::mat4 transM;

		rotM = glm::rotate(rotM, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::vec3 translation = position;
		if (flipY) {
			translation.y *= -1.0f;
		}
		transM = glm::translate(glm::mat4(1.0f), translation);

		if (type == CameraType::firstperson)
		{
			matrices.view = rotM * transM;
		}
		else
		{
			matrices.view = transM * rotM;
		}

		viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		if (matrices.view != currentMatrix) {
			updated = true;
		}
	};
public:
	enum CameraType { lookat, firstperson };  // 相机类型：观察模式、第一人称模式
	CameraType type = CameraType::lookat;     // 当前相机类型

	glm::vec3 rotation = glm::vec3();  // 旋转角度（欧拉角）
	glm::vec3 position = glm::vec3();  // 位置
	glm::vec4 viewPos = glm::vec4();   // 视图位置

	float rotationSpeed = 1.0f;   // 旋转速度
	float movementSpeed = 1.0f;   // 移动速度

	bool updated = true;   // 是否已更新
	bool flipY = false;     // 是否翻转 Y 轴

	struct
	{
		glm::mat4 perspective;  // 透视投影矩阵
		glm::mat4 view;         // 视图矩阵
	} matrices;

	struct
	{
		bool left = false;   // 左移键
		bool right = false;  // 右移键
		bool up = false;     // 上移键
		bool down = false;   // 下移键
	} keys;

	/**
	 * @brief 检查是否正在移动
	 * @return 如果任何方向键被按下返回 true
	 */
	bool moving() const
	{
		return keys.left || keys.right || keys.up || keys.down;
	}

	/**
	 * @brief 获取近裁剪面距离
	 * @return 近裁剪面距离
	 */
	float getNearClip() const {
		return znear;
	}

	/**
	 * @brief 获取远裁剪面距离
	 * @return 远裁剪面距离
	 */
	float getFarClip() const {
		return zfar;
	}

	/**
	 * @brief 设置透视投影
	 * @param fov 视野角度（度）
	 * @param aspect 宽高比
	 * @param znear 近裁剪面距离
	 * @param zfar 远裁剪面距离
	 */
	void setPerspective(float fov, float aspect, float znear, float zfar)
	{
		glm::mat4 currentMatrix = matrices.perspective;
		this->fov = fov;
		this->znear = znear;
		this->zfar = zfar;
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
		if (matrices.view != currentMatrix) {
			updated = true;
		}
	};

	/**
	 * @brief 更新宽高比
	 * 重新计算透视投影矩阵以适应新的宽高比
	 * 
	 * @param aspect 新的宽高比（宽度/高度）
	 */
	void updateAspectRatio(float aspect)
	{
		glm::mat4 currentMatrix = matrices.perspective;  // 保存当前矩阵用于比较
		// 使用新的宽高比重新计算透视投影矩阵
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
		// 如果启用 Y 轴翻转，翻转投影矩阵的 Y 分量
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
		// 如果矩阵发生变化，标记为已更新
		if (matrices.view != currentMatrix) {
			updated = true;
		}
	}

	/**
	 * @brief 设置相机位置
	 * 设置相机在世界空间中的位置并更新视图矩阵
	 * 
	 * @param position 新的位置向量（世界空间坐标）
	 */
	void setPosition(glm::vec3 position)
	{
		this->position = position;  // 更新位置
		updateViewMatrix();         // 更新视图矩阵
	}

	/**
	 * @brief 设置相机旋转
	 * 设置相机的欧拉角旋转并更新视图矩阵
	 * 
	 * @param rotation 新的旋转角度向量（欧拉角，度）
	 */
	void setRotation(glm::vec3 rotation)
	{
		this->rotation = rotation;  // 更新旋转角度
		updateViewMatrix();          // 更新视图矩阵
	}

	/**
	 * @brief 旋转相机
	 * 在现有旋转基础上增加旋转增量
	 * 
	 * @param delta 旋转增量向量（欧拉角，度）
	 */
	void rotate(glm::vec3 delta)
	{
		this->rotation += delta;  // 累加旋转增量
		updateViewMatrix();        // 更新视图矩阵
	}

	/**
	 * @brief 设置相机平移
	 * 设置相机在世界空间中的平移位置（与 setPosition 相同）
	 * 
	 * @param translation 新的平移位置向量（世界空间坐标）
	 */
	void setTranslation(glm::vec3 translation)
	{
		this->position = translation;  // 更新位置
		updateViewMatrix();             // 更新视图矩阵
	};

	/**
	 * @brief 平移相机
	 * 在现有位置基础上增加平移增量
	 * 
	 * @param delta 平移增量向量（世界空间坐标）
	 */
	void translate(glm::vec3 delta)
	{
		this->position += delta;  // 累加平移增量
		updateViewMatrix();        // 更新视图矩阵
	}

	/**
	 * @brief 设置旋转速度
	 * 设置相机旋转的敏感度
	 * 
	 * @param rotationSpeed 新的旋转速度值
	 */
	void setRotationSpeed(float rotationSpeed)
	{
		this->rotationSpeed = rotationSpeed;
	}

	/**
	 * @brief 设置移动速度
	 * 设置相机移动的敏感度
	 * 
	 * @param movementSpeed 新的移动速度值
	 */
	void setMovementSpeed(float movementSpeed)
	{
		this->movementSpeed = movementSpeed;
	}

	/**
	 * @brief 更新相机状态
	 * 根据输入键和经过的时间更新相机位置和视图矩阵
	 * 
	 * @param deltaTime 自上次更新以来经过的时间（秒）
	 */
	void update(float deltaTime)
	{
		updated = false;  // 重置更新标志
		// 第一人称模式：根据按键输入更新位置
		if (type == CameraType::firstperson)
		{
			if (moving())  // 如果有按键被按下
			{
				// 计算相机前方向量（基于当前旋转角度）
				// 使用球面坐标转换为笛卡尔坐标
				glm::vec3 camFront;
				camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));  // X 分量
				camFront.y = sin(glm::radians(rotation.x));                                    // Y 分量（俯仰角）
				camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));   // Z 分量
				camFront = glm::normalize(camFront);  // 归一化方向向量

				// 计算基于时间的移动速度
				float moveSpeed = deltaTime * movementSpeed;

				// 根据按键更新位置
				if (keys.up)
					position += camFront * moveSpeed;  // 向前移动
				if (keys.down)
					position -= camFront * moveSpeed;   // 向后移动
				if (keys.left)
					// 向左移动（使用前方向量与上方向的叉积得到右方向）
					position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
				if (keys.right)
					// 向右移动
					position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
			}
		}
		// 更新视图矩阵以反映新的位置和旋转
		updateViewMatrix();
	};

	/**
	 * @brief 使用游戏手柄输入更新相机
	 * 使用分离的轴数据（游戏手柄摇杆）更新相机位置和旋转
	 * 
	 * @param axisLeft 左摇杆轴数据（x: 左右，y: 前后）
	 * @param axisRight 右摇杆轴数据（x: 左右旋转，y: 上下旋转）
	 * @param deltaTime 自上次更新以来经过的时间（秒）
	 * 
	 * @return 如果视图或位置已更改返回 true，否则返回 false
	 */
	bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
	{
		bool retVal = false;  // 返回值：是否发生更改

		if (type == CameraType::firstperson)
		{
			// 使用常见的游戏手柄摇杆布局
			// 左摇杆 = 移动，右摇杆 = 视角

			const float deadZone = 0.0015f;  // 死区阈值（忽略小的摇杆输入）
			const float range = 1.0f - deadZone;  // 有效范围

			// 计算相机前方向量（基于当前旋转角度）
			glm::vec3 camFront;
			camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
			camFront.y = sin(glm::radians(rotation.x));
			camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
			camFront = glm::normalize(camFront);

			// 计算基于时间的移动和旋转速度
			float moveSpeed = deltaTime * movementSpeed * 2.0f;  // 移动速度（乘以 2 以加快游戏手柄移动）
			float rotSpeed = deltaTime * rotationSpeed * 50.0f;  // 旋转速度（乘以 50 以加快游戏手柄旋转）
			 
			// 移动：使用左摇杆
			// Y 轴：前后移动
			if (fabsf(axisLeft.y) > deadZone)
			{
				// 计算归一化的输入值（去除死区）
				float pos = (fabsf(axisLeft.y) - deadZone) / range;
				// 根据 Y 轴方向移动（负值向前，正值向后）
				position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
				retVal = true;
			}
			// X 轴：左右移动
			if (fabsf(axisLeft.x) > deadZone)
			{
				// 计算归一化的输入值
				float pos = (fabsf(axisLeft.x) - deadZone) / range;
				// 计算右方向（前方向与上方向的叉积）
				// 根据 X 轴方向移动（正值向右，负值向左）
				position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
				retVal = true;
			}

			// 旋转：使用右摇杆
			// X 轴：左右旋转（偏航角）
			if (fabsf(axisRight.x) > deadZone)
			{
				// 计算归一化的输入值
				float pos = (fabsf(axisRight.x) - deadZone) / range;
				// 更新偏航角（Y 轴旋转）
				rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
				retVal = true;
			}
			// Y 轴：上下旋转（俯仰角）
			if (fabsf(axisRight.y) > deadZone)
			{
				// 计算归一化的输入值
				float pos = (fabsf(axisRight.y) - deadZone) / range;
				// 更新俯仰角（X 轴旋转，注意使用减号以匹配常见控制方案）
				rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
				retVal = true;
			}
		}
		else
		{
			// TODO: 从示例基类移动观察模式的代码
		}

		// 如果位置或旋转发生变化，更新视图矩阵
		if (retVal)
		{
			updateViewMatrix();
		}

		return retVal;
	}

};