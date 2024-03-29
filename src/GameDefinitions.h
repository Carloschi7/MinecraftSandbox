#pragma once
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <optional>
#include <random>
#include "MainIncl.h"
#include "Utils.h"

//Game defintions
namespace Defs
{
	//Game view state
	enum class ViewMode {
		WorldInteraction,
		Inventory
	};

	enum class MovementType : u8 {
		Normal, Creative
	};

	enum class Item : u8
	{
		//Blocks
		Dirt = 0,
		Grass,
		Stone,
		Sand,
		Wood,
		WoodPlanks,
		Leaves,
		CraftingTable,
		//Items
		WoodStick,
		WoodPickaxe
	};

	static bool IsBlock(Item sprite) {
		return !(sprite >= Item::WoodStick);
	}

	enum class HitDirection : u8
	{
		None = 0, PosX, NegX, PosY, NegY, PosZ, NegZ,
	};

	enum class TextureBinding : u8
	{
		GlobalTexture = 0,
		TextureInventory,
		TextureCraftingTableInventory,
		TextureScreenInventory,
		TextureScreenInventorySelector,
		TextureScreenFramebuffer,
		TextureDepthFramebuffer,
		TextureText
	};

	struct WorldSeed
	{
		//Chosed world seed
		u64 seed_value;
		//Related seeds computed from the main one, used to generate
		//overlapping perlin noise to give a much direct sense of realism
		//in the scene
		Utils::Vector<u64> secundary_seeds;
	};

	//Handles section data
	struct SectionData
	{
		u32 index;
		glm::vec2 central_position;
		//Determines whether or not the data is loaded in memory
		bool loaded = true;
	};

	struct WaterArea
	{
		glm::vec2 pos_xz;
		glm::vec2 neg_xz;
		f32 water_height;

		f32 Length() const
		{
			return glm::length(pos_xz - neg_xz);
		}

		bool Contains(f32 x, f32 y) const
		{
			return x >= neg_xz.x && y >= neg_xz.y &&
				x <= pos_xz.x && y <= pos_xz.y;
		}
	};

	enum class ChunkLocation { None = 0, PlusX, MinusX, PlusZ, MinusZ };
	enum class Biome { Plains = 0, Desert };

	//-----------------Variables
	extern std::atomic<ViewMode> g_ViewMode;
	extern u32 g_ScreenWidth, g_ScreenHeight;

	//Game global variables
	extern u16 g_InventoryKey;
	extern MovementType g_MovementType;

	//Helps the update of the player position when he is colliding with world objects
	extern glm::vec3 g_PlayerAxisMapping;
	extern f32 g_PlayerSpeed;
	extern const f32 g_ChunkSpawningDistance;
	extern const f32 g_ChunkRenderingDistance;
	extern const f32 g_CameraCompensation;
	extern const f32 g_RenderDistance;
	extern const f32 g_SectionDimension;
	//Personal index for each chunk
	extern u32 g_ChunkProgIndex;
	extern const s32 g_SpawnerBegin;
	extern const s32 g_SpawnerEnd;
	extern const s32 g_SpawnerIncrement;
	extern const glm::vec3 g_LightDirection;
	//Variables for block selection
	extern std::atomic<u32> g_SelectedBlock;
	extern std::atomic<u32> g_SelectedChunk;
	extern bool g_EnvironmentChange;
	//Inventory
	static constexpr u8 g_InventoryInternalSlotsCount = 27;
	static constexpr u8 g_InventoryScreenSlotsCount = 9;
	static constexpr u8 g_CraftingSlotsMaxCount = 9;
	static constexpr u8 g_MaxItemsPerSlot = 64;
	extern Defs::Item g_InventorySelectedBlock;
	//Used to track how many sections have been pushed
	extern std::unordered_set<u32> g_PushedSections;
	extern std::string g_SerializedFileFormat;

	//Perlin variables
	extern f32 water_limit;
	static f32 landmap_density = 1000.0f;
	static f32 watermap_density = 900.0f;

	extern std::pair<f32, bool> jump_data;
	
	void ReadWasd(const Window& window, bool& w, bool& a, bool& s, bool& d);
	void KeyboardFunction(const Window& window, Camera* camera, f64 time);
    void MouseFunction(const Window& window, Camera* camera, f64 x, f64 y, f64 dpi, f64 time);
	HitDirection ViewBlockCollision(const glm::vec3& camera_pos, const glm::vec3& camera_dir, const glm::vec3& block_pos, f32& dist);
	//Chunks will also be assigned a value in order to be grouped
	//with other chunks. This is done in order to save RAM, so not
	//every chunk of the world is loaded at once.
	//If the player is far enough from a chunk sector, the chunks
	//will be serialized on the disk
	u32 ChunkSectorIndex(const glm::vec2& pos);
	//Given an underwater tile coordinate, determines and caches the water level for that
	//water region. The return value is internally cached to avoid computing the value
	//for each tile in the water region
	f32 WaterRegionLevel(f32 sx, f32 sy, const WorldSeed& seed, Utils::Vector<WaterArea>& pushed_areas);
	Utils::Vector<glm::vec3> GenerateRandomFoliage(Utils::Vector<glm::vec3>& possible_positions, std::mt19937& rand_engine);
	
	//Perlin noise related funcions namespace, very little overhead used
	namespace PerlNoise
	{
		struct Generation
		{
			f32 altitude;
			Biome biome;
			bool in_water;
		};

		void InitSeedMap(WorldSeed& seed);
		f32 Interpolate(f32 a0, f32 a1, f32 w);
		glm::vec2 GenRandomVecFrom(s32 n1, s32 n2, const u64& seed);
		f32 PerformDot(s32 a, s32 b, f32 x, f32 y, const u64& seed);
		f32 GenerateSingleNoise(f32 x, f32 y, const u64& seed);
		Generation GetBlockAltitude(f32 x, f32 y, const WorldSeed& seed);
	}
}

namespace Physics {
	void HandlePlayerMovement(f32 elapsed_time);
	void HandlePlayerGravity(f32 elapsed_time);
	void ProcessPlayerAxisMovement(f32 elapsed_time);
}