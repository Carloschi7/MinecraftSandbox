#pragma once
#include <optional>

#include "Block.h"
#include "glm/glm.hpp"

class World;

class Chunk
{
public:
	//For now the chunk generates 16x16x16 blocks in the world
	//Origin specifies the front-bottom-left coordinate from which
	//generating blocks(only dirt generated for now)
	//the y axis is not considered, so the y component of the origin vector
	//will be used as an alias of z
	Chunk(World* father, glm::vec2 origin);
	//Construct by deserialization
	Chunk(World* father, const Utils::Serializer& sz, uint32_t index);
	Chunk(const Chunk&) = delete;
	Chunk(Chunk&& rhs) noexcept;
	~Chunk();

	Chunk& operator=(const Chunk&) = delete;
	Chunk& operator=(Chunk&& rhs) noexcept;

	//Normals loaded as the chunk spawns
	void InitGlobalNorms();
	//Add new normals for a newly placed block
	void AddFreshNormals(Block& b);
	//Add new normals after a block deletion
	void AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check = false);

	[[nodiscard]] float BlockCollisionLogic(bool left_click, bool right_click);
	void UpdateBlocks();
	//Checks if this chunk is near enough to the player to be rendered
	bool IsChunkRenderable() const;
	//Checks if this chunk (within the renderable space) is visible by the player
	bool IsChunkVisible() const;
	//Determines if the chunk is visible by the shadow shader
	bool IsChunkVisibleByShadow() const;
	//Removes the defined normal from all chunk blocks which border
	void RemoveBorderNorm(const glm::vec3& norm);

	const glm::vec2& GetChunkOrigin() const;
	//When loaded from the relative world, returns the indexed position of the adjacent chunks
	const std::optional<uint32_t>& GetLoadedChunk(const Gd::ChunkLocation& cl) const;
	void SetLoadedChunk(const Gd::ChunkLocation& cl, uint32_t value);
	void Draw(bool depth_buf_draw, bool selected = false) const;
	uint32_t LastSelectedBlock() const;

	uint32_t SectorIndex() const;
	uint32_t Index() const;

	//Sum this with the chunk origin to get chunk's center
	static glm::vec3 GetHalfWayVector();

	//Serializing & and deserializing % function
	const Utils::Serializer& operator&(const Utils::Serializer& sz);
	const Utils::Serializer& operator%(const Utils::Serializer& sz);
public:
	//Variable used to determine which chunk holds the selected block
	//used instead of Gd::g_SelectedChunk in multiple iterations so we
	//access the atomic variable only once
	static uint32_t s_InternalSelectedBlock;

private:
	//Returns if there is a block at the location pos
	//The last two attributes can be used to make the searching faster
	bool IsBlock(const glm::vec3& pos, int32_t starting_index = 0, bool search_towards_end = true, uint32_t* block_index = nullptr) const;
	Block& GetBlock(uint32_t index);
	const Block& GetBlock(uint32_t index) const;
private:
	//Father world
	World* m_RelativeWorld;

	//Chunk progressive index
	uint32_t m_ChunkIndex;

	VecType<Block> m_LocalBlocks;
	//front-bottom-left block position
	glm::vec2 m_ChunkOrigin;
	glm::vec3 m_ChunkCenter;
	//Block aimed by the player
	uint32_t m_SelectedBlock;
	//determining if side chunk exists
	std::optional<uint32_t> m_PlusX, m_MinusX, m_PlusZ, m_MinusZ;
	//Sector index
	uint32_t m_SectorIndex;


	static float s_DiagonalLenght;
	static constexpr uint32_t s_ChunkWidthAndHeight = 16;
	static constexpr uint32_t s_ChunkDepth = 50;
};