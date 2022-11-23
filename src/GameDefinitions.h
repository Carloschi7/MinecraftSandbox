#pragma once
#include "MainIncl.h"
#include <array>
#include <glm/glm.hpp>

//Game defintions
namespace Gd
{
	//Game global variables
	extern const float g_ChunkSpawningDistance;
	extern const float g_ChunkRenderingDistance;
	extern const float g_CameraCompensation;
	extern const float g_RenderDistance;
	extern const int32_t g_SpawnerBegin;
	extern const int32_t g_SpawnerEnd;
	extern const int32_t g_SpawnerIncrement;

    enum class BlockType : uint8_t
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

	inline void KeyboardFunction(const Window& window, Camera* camera, double time)
	{
		float fScalar = 0.6f;

		if (window.IsKeyboardEvent({ GLFW_KEY_W, GLFW_PRESS }))
			camera->MoveTowardsFront(fScalar * time);
		if (window.IsKeyboardEvent({ GLFW_KEY_S, GLFW_PRESS }))
			camera->MoveTowardsFront(-fScalar * time);
		if (window.IsKeyboardEvent({ GLFW_KEY_A, GLFW_PRESS }))
			camera->StrafeX(-fScalar * time);
		if (window.IsKeyboardEvent({ GLFW_KEY_D, GLFW_PRESS }))
			camera->StrafeX(fScalar * time);
		if (window.IsKeyboardEvent({ GLFW_KEY_E, GLFW_PRESS }))
			camera->StrafeY(fScalar * time);
		if (window.IsKeyboardEvent({ GLFW_KEY_C, GLFW_PRESS }))
			camera->StrafeY(-fScalar * time);
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

	//Perlin noise related funcions namespace
	namespace PerlNoise
	{
		inline float Interpolate(float a0, float a1, float w)
		{
			return (a1 - a0) * w + a0;
		}

		inline glm::vec2 GenRandomVecFrom(int32_t n1, int32_t n2)
		{
			// Snippet of code i took from the internet. scrambles 
			//some values and spits out a reasonable random value
			const uint32_t w = 8 * sizeof(uint32_t);
			const uint32_t s = w / 2; // rotation width
			uint32_t a = n1, b = n2;
			a *= 3284157443; b ^= a << s | a >> w - s;
			b *= 1911520717; a ^= b << s | b >> w - s;
			a *= 2048419325;
			float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
			return { glm::cos(random), glm::sin(random) };
		}

		inline float PerformDot(int32_t a, int32_t b, float x, float y)
		{
			glm::vec2 rand_vec = GenRandomVecFrom(a, b);
			//Offset vector
			glm::vec2 offset{x - static_cast<float>(a), y - static_cast<float>(b)};
			return glm::dot(offset, rand_vec);
		}

		//Trying to use as little of an overhead as possible
		inline float Generate(float x, float y)
		{
			int32_t x0, x1, y0, y1;
			float sx, sy;
			x0 = std::floor(x);
			x1 = x0 + 1;
			y0 = std::floor(y);
			y1 = y0 + 1;

			sx = x - static_cast<float>(x0);
			sy = y - static_cast<float>(y0);

			float f1, f2, f3, f4, fr1, fr2;
			f1 = PerformDot(x0, y0, x, y);
			f2 = PerformDot(x1, y0, x, y);
			f3 = PerformDot(x0, y1, x, y);
			f4 = PerformDot(x1, y1, x, y);

			fr1 = Interpolate(f1, f2, sx);
			fr2 = Interpolate(f3, f4, sx);
			return Interpolate(fr1, fr2, sy);
		}
	}
}