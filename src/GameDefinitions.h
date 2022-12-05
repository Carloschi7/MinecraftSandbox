#pragma once
#include "MainIncl.h"
#include <array>
#include <glm/glm.hpp>
#include <vector>

//Game defintions
namespace Gd
{
	//Game global variables
	extern const float g_ChunkSpawningDistance;
	extern const float g_ChunkRenderingDistance;
	extern const float g_CameraCompensation;
	extern const float g_RenderDistance;
	extern const float g_FramedPlayerSpeed;
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
	struct WorldSeed
	{
		//Chosed world seed
		uint64_t seed_value;
		//Related seeds computed from the main one, used to generate
		//overlapping perlin noise to give a much direct sense of realism
		//in the scene
		std::vector<uint64_t> secundary_seeds;
	};

	enum class ChunkLocation {NONE = 0, PLUS_X, MINUS_X, PLUS_Z, MINUS_Z};

	void KeyboardFunction(const Window& window, Camera* camera, double time);
    void MouseFunction(const Window& window, Camera* camera, double x, double y, double dpi, double time);
	bool ViewBlockCollision(const glm::vec3& camera_pos, const glm::vec3& camera_dir, const glm::vec3& block_pos, float& dist);

	//Perlin noise related funcions namespace, very little overhead used
	namespace PerlNoise
	{
		void InitSeedMap(WorldSeed& seed);
		float Interpolate(float a0, float a1, float w);
		glm::vec2 GenRandomVecFrom(int32_t n1, int32_t n2, const uint64_t& seed);
		float PerformDot(int32_t a, int32_t b, float x, float y, const uint64_t& seed);
		float GenerateSingleNoise(float x, float y, const uint64_t& seed);
		float GetBlockAltitude(float x, float y, const WorldSeed& seed);
	}
}