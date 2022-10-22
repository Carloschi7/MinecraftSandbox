#pragma once
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

	//All the chunks need to be loaded in order to use this function
	void InitBlockNormals();
	void SetBlockSelected(bool selected) const;
	[[nodiscard]] float BlockCollisionLogic(const GameDefs::ChunkBlockLogicData& ld);
	void UpdateBlocks(const GameDefs::ChunkBlockLogicData& ld);

	const glm::vec2& GetChunkOrigin() const;
	void Draw(const GameDefs::RenderData& rd) const;
	void AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check = false);
private:
	//Father
	World* m_RelativeWorld;

	std::vector<Block> m_LocalBlocks;
	GlCore::ChunkStructure m_ChunkStructure;
	//front-bottom-left block position
	glm::vec2 m_ChunkOrigin;
	//Block aimed by the player
	uint32_t m_SelectedBlock;
	//determining if side chunk exists
	std::vector<Chunk>::iterator m_PlusX, m_MinusX, m_PlusZ, m_MinusZ;
	//Determines whether the block selection belongs to this chunk
	mutable bool m_IsSelectionHere;
	static constexpr uint32_t m_ChunkWidthAndHeight = 16;
	static constexpr uint32_t m_ChunkDepth = 50;
};