#pragma once
#include "MainIncl.h"
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

//Game defintions
namespace Gd
{
	//Game global variables
	extern const float g_ChunkSpawningDistance;
	extern const float g_ChunkRenderingDistance;
	extern const float g_CameraCompensation;
	extern const float g_RenderDistance;
	extern const float g_FramedPlayerSpeed;
	extern const float g_SectionDimension;
	//Personal index for each chunk
	extern uint32_t g_ChunkProgIndex;
	extern const int32_t g_SpawnerBegin;
	extern const int32_t g_SpawnerEnd;
	extern const int32_t g_SpawnerIncrement;
	extern const glm::vec3 g_LightDirection;
	//Variables for block selection
	extern std::atomic_uint32_t g_SelectedBlock;
	extern std::atomic_uint32_t g_SelectedChunk;
	extern bool g_BlockDestroyed;
	//Used to track how many sections have been pushed
	extern std::unordered_set<uint32_t> g_PushedSections;
	extern std::string g_SerializedFileFormat;

    enum class BlockType : uint8_t
    {
        DIRT = 0, GRASS, SAND, WOOD, LEAVES
    };

	enum class HitDirection : uint8_t
	{
		NONE = 0, POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z,
	};

	struct MouseInput
	{
		bool right_click;
		bool left_click;
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

	//Handles section data
	struct SectionData
	{
		uint32_t index;
		glm::vec2 central_position;
		//Determines whether or not the data is loaded in memory
		bool loaded = true;
	};

	enum class ChunkLocation {NONE = 0, PLUS_X, MINUS_X, PLUS_Z, MINUS_Z};
	enum class Biome {PLAINS = 0, DESERT};

	void KeyboardFunction(const Window& window, Camera* camera, double time);
    void MouseFunction(const Window& window, Camera* camera, double x, double y, double dpi, double time);
	HitDirection ViewBlockCollision(const glm::vec3& camera_pos, const glm::vec3& camera_dir, const glm::vec3& block_pos, float& dist);
	//Chunks will also be assigned a value in order to be grouped
	//with other chunks. This is done in order to save RAM, so not
	//every chunk of the world is loaded at once.
	//If the player is far enough from a chunk sector, the chunks
	//will be serialized on the disk
	uint32_t ChunkSectorIndex(const glm::vec2& pos);

	//Perlin noise related funcions namespace, very little overhead used
	namespace PerlNoise
	{
		struct Generation
		{
			float altitude;
			Biome biome;
		};

		void InitSeedMap(WorldSeed& seed);
		float Interpolate(float a0, float a1, float w);
		glm::vec2 GenRandomVecFrom(int32_t n1, int32_t n2, const uint64_t& seed);
		float PerformDot(int32_t a, int32_t b, float x, float y, const uint64_t& seed);
		float GenerateSingleNoise(float x, float y, const uint64_t& seed);
		Generation GetBlockAltitude(float x, float y, const WorldSeed& seed);
	}
}