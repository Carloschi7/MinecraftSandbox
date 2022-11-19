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
	Chunk(const Chunk&) = delete;
	Chunk(Chunk&&) = default;
	~Chunk();

	//All the chunks need to be loaded in order to use this function
	void InitBlockNormals();
	void SetBlockSelected(bool selected) const;
	[[nodiscard]] float BlockCollisionLogic(const GameDefs::ChunkLogicData& ld);
	void UpdateBlocks(const GameDefs::ChunkLogicData& ld);
	//Checks if this chunk is near enough to the player to be rendered
	bool IsChunkRenderable(const GameDefs::ChunkLogicData& rd) const;
	//Checks if this chunk (within the renderable space) is visible by the player
	bool IsChunkVisible(const GameDefs::ChunkLogicData& rd) const;
	//Removes the defined normal from all chunk blocks which border
	void RemoveBorderNorm(const glm::vec3& norm);

	const glm::vec2& GetChunkOrigin() const;
	//When loaded from the relative world, returns the indexed position of the adjacent chunks
	const std::optional<uint32_t>& GetLoadedChunk(const GameDefs::ChunkLocation& cl) const;
	void SetLoadedChunk(const GameDefs::ChunkLocation& cl, uint32_t value);
	void Draw(const GameDefs::RenderData& rd) const;
	void AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check = false);

	//Sum this with the chunk origin to get chunk's center
	static glm::vec3 GetHalfWayVector();
private:
	//Returns the iterator of a block exposed normal
	std::vector<glm::vec3>::const_iterator NormalAt(const Block& b, const glm::vec3& norm);
	//Returns if there is a block at the location pos
	//The last two attributes can be used to make the searching faster
	bool IsBlock(const glm::vec3& pos, int32_t starting_index = 0, bool search_towards_end = true) const;
private:
	//Father World
	World* m_RelativeWorld;

	VecType<Block> m_LocalBlocks;
	GlCore::ChunkStructure m_ChunkStructure;
	//front-bottom-left block position
	glm::vec2 m_ChunkOrigin;
	glm::vec3 m_ChunkCenter;
	//Block aimed by the player
	uint32_t m_SelectedBlock;
	//determining if side chunk exists
	std::optional<uint32_t> m_PlusX, m_MinusX, m_PlusZ, m_MinusZ;
	//Determines whether the block selection belongs to this chunk
	mutable bool m_IsSelectionHere;
	static float s_DiagonalLenght;
	static constexpr uint32_t s_ChunkWidthAndHeight = 16;
	static constexpr uint32_t s_ChunkDepth = 50;
};