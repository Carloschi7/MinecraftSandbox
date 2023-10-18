#pragma once
#include <optional>
#include "glm/glm.hpp"

#include "Block.h"
#include "State.h"

class World;
class Inventory;

class Chunk
{
public:
	//For now the chunk generates 16x16x16 blocks in the world
	//Origin specifies the front-bottom-left coordinate from which
	//generating blocks(only dirt generated for now)
	//the y axis is not considered, so the y component of the origin vector
	//will be used as an alias of z
	Chunk(World& father, glm::vec2 origin);
	//Construct by deserialization
	Chunk(World& father, const Utils::Serializer& sz, u32 index);
	Chunk(const Chunk&) = delete;
	Chunk(Chunk&& rhs) noexcept;
	~Chunk();

	Chunk& operator=(const Chunk&) = delete;
	Chunk& operator=(Chunk&& rhs) noexcept;

	//Stores render data and presents it if the buffers are full
	void ForwardRenderableData(glm::vec3*& position_buf, u32*& texindex_buf, u32& count, bool depth_buf_draw, bool selected = false) const;
	void RenderDrops();

	//Normals loaded as the chunk spawns
	void InitGlobalNorms();
	//Add new normals for a newly placed block
	void AddFreshNormals(Block& b);
	//Add new normals after a block deletion
	void AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check = false);
	//Check if the player collides with the scene

	//Collision functions
	[[nodiscard]] std::pair<f32, Defs::HitDirection> RayCollisionLogic(const glm::vec3& camera_position, const glm::vec3& camera_direction);
	void BlockCollisionLogic(glm::vec3& position);


	void UpdateBlocks(Inventory& inventory, f32 elapsed_time);
	//Checks if this chunk is near enough to the player to be rendered
	bool IsChunkRenderable(const glm::vec3& camera_position) const;
	//Checks if this chunk (within the renderable space) is visible by the player
	bool IsChunkVisible(const glm::vec3& camera_position, const glm::vec3& camera_direction) const;
	//Determines if the chunk is visible by the shadow shader
	bool IsChunkVisibleByShadow(const glm::vec3& camera_position, const glm::vec3& camera_direction) const;
	//Removes the defined normal from all chunk blocks which border
	void RemoveBorderNorm(const glm::vec3& norm);

	//When loaded from the relative world, returns the indexed position of the adjacent chunks
	const std::optional<u32>& GetLoadedChunk(const Defs::ChunkLocation& cl) const;
	void SetLoadedChunk(const Defs::ChunkLocation& cl, u32 value);
	u32 LastSelectedBlock() const;

	u32 SectorIndex() const;
	u32 Index() const;

	inline const glm::vec2& ChunkOrigin() const { return m_ChunkOrigin; }
	inline const glm::vec3& ChunkCenter() const { return m_ChunkCenter; }
	inline std::vector<Block>& Blocks() { return m_LocalBlocks; }
	inline void PushDrop(const glm::vec3& position, Defs::BlockType type) { m_LocalDrops.emplace_back(position, type); }

	//Sum this with the chunk origin to get chunk's center
	static glm::vec3 GetHalfWayVector();

	//Serializing & and deserializing % function
	const Utils::Serializer& Serialize(const Utils::Serializer& sz);
	const Utils::Serializer& Deserialize(const Utils::Serializer& sz);
public:
	//Variable used to determine which chunk holds the selected block
	//used instead of Gd::g_SelectedChunk in multiple iterations so we
	//access the atomic variable only once
	static u32 s_InternalSelectedBlock;

private:
	//Returns if there is a block at the location pos
	//The last two attributes can be used to make the searching faster
	bool IsBlock(const glm::vec3& pos, s32 starting_index = 0, bool search_towards_end = true, u32* block_index = nullptr) const;
	Block& GetBlock(u32 index);
	const Block& GetBlock(u32 index) const;

	//Wrapper function that assigns normals to border blocks if there are no other blocks even in the confining chunk
	bool BorderCheck(Chunk* chunk, const glm::vec3& pos, u32 top_index, u32 bot_index, bool search_dir);
private:
	//Global OpenGL environment state
	GlCore::State& m_State;
	//Belonging world
	World& m_RelativeWorld;

	//Chunk progressive index
	u32 m_ChunkIndex;
	std::vector<Block> m_LocalBlocks;
	//Drops of brokeen blocks, the chunk which originated them is the
	//responsible for updating and drawing them
	std::vector<Drop> m_LocalDrops;

	//Eventual water layer(using a shared ptr because this ptr will also be stored in world)
	std::shared_ptr<std::vector<glm::vec3>> m_WaterLayerPositions;
	//front-bottom-left block position
	glm::vec2 m_ChunkOrigin;
	glm::vec3 m_ChunkCenter;
	//Block aimed by the player
	u32 m_SelectedBlock;
	//determining if side chunk exists
	std::optional<u32> m_PlusX, m_MinusX, m_PlusZ, m_MinusZ;
	//Sector index
	u32 m_SectorIndex;


	static f32 s_DiagonalLenght;
	static constexpr u32 s_ChunkWidthAndHeight = 16;
	static constexpr u32 s_ChunkDepth = 50;
};