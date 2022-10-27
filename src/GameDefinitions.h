#pragma once
#include "MainIncl.h"
#include <array>

namespace GameDefs
{
	//Game global variables
	extern float g_ChunkSpawningDistance;
	extern int32_t g_SpawnerBegin;
	extern int32_t g_SpawnerEnd;
	extern int32_t g_SpawnerIncrement;

    enum class BlockType
    {
        DIRT = 0, GRASS
    };

	struct MouseInput
	{
		bool right_click;
		bool left_click;
	};

    struct RenderData
    {
		glm::vec3 camera_position;
        glm::mat4 proj_matrix;
        glm::mat4 view_matrix;
		//Debug purposes
		bool p_key;
    };
	//Logic for chunk updating
	struct ChunkLogicData
	{
		MouseInput mouse_input;
		glm::vec3 camera_position;
		glm::vec3 camera_direction;
	};

	enum class ChunkLocation {NONE = 0, PLUS_X, MINUS_X, PLUS_Z, MINUS_Z};

	inline void KeyboardFunction(const Window& window, Camera* camera)
	{
		float fScalar = 0.6f;

		if (window.IsKeyboardEvent({ GLFW_KEY_W, GLFW_PRESS }))
			camera->MoveTowardsFront(fScalar);
		if (window.IsKeyboardEvent({ GLFW_KEY_S, GLFW_PRESS }))
			camera->MoveTowardsFront(-fScalar);
		if (window.IsKeyboardEvent({ GLFW_KEY_A, GLFW_PRESS }))
			camera->StrafeX(-fScalar);
		if (window.IsKeyboardEvent({ GLFW_KEY_D, GLFW_PRESS }))
			camera->StrafeX(fScalar);
		if (window.IsKeyboardEvent({ GLFW_KEY_E, GLFW_PRESS }))
			camera->StrafeY(fScalar);
		if (window.IsKeyboardEvent({ GLFW_KEY_C, GLFW_PRESS }))
			camera->StrafeY(-fScalar);
	}

	inline void MouseFunction(const Window& window, Camera* camera, double x, double y, double dpi, double time)
	{
		double localx, localy;
		window.GetCursorCoord(localx, localy);

		camera->RotateX((localx - x) * dpi * time);
		camera->RotateY((y - localy) * dpi * time);
	}

	inline bool ViewBlockCollision(const glm::vec3& camera_pos, const glm::vec3& camera_dir, const glm::vec3& block_pos, float& dist)
	{
		//Calculating distance
		dist = glm::length(camera_pos - block_pos);
		if (dist > 10.0f)
			return false;

		static float half_cube_diag = 0.5f;
		glm::vec3 cube_min = block_pos - glm::vec3(half_cube_diag);
		glm::vec3 cube_max = block_pos + glm::vec3(half_cube_diag);

		float t0x = (cube_min.x - camera_pos.x) / camera_dir.x;
		float t1x = (cube_max.x - camera_pos.x) / camera_dir.x;

		if (t0x > t1x)
			std::swap(t0x, t1x);

		float t0y = (cube_min.y - camera_pos.y) / camera_dir.y;
		float t1y = (cube_max.y - camera_pos.y) / camera_dir.y;

		if (t0y > t1y)
			std::swap(t0y, t1y);

		if (t1x < t0y || t1y < t0x)
			return false;

		float t0z = (cube_min.z - camera_pos.z) / camera_dir.z;
		float t1z = (cube_max.z - camera_pos.z) / camera_dir.z;

		if (t0z > t1z)
			std::swap(t0z, t1z);

		float tc0 = std::max(t0x, t0y);
		float tc1 = std::min(t1x, t1y);

		if (t0z > tc1 || t1z < tc0)
			return false;

		return true;
	}
}