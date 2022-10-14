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
	Chunk(World* father, glm::vec3 origin);
	Chunk(const Chunk&) = delete;
	Chunk(Chunk&&) = default;

	//All the chunks need to be loaded in order to use this function
	void InitBlockNormals();
	void BlockCollisionLogic(const GameDefs::ChunkBlockLogicData& ld);
	const glm::vec3& GetChunkOrigin() const;
	void Draw(const GameDefs::RenderData& rd) const;
	void AddNewExposedNormals(std::vector<Block>::const_iterator iter, bool side_chunk_check = false);
private:
	//Father
	World* m_RelativeWorld;

	std::vector<Block> m_LocalBlocks;
	GlCore::ChunkStructure m_ChunkStructure;
	//front-bottom-left block position
	glm::vec3 m_ChunkOrigin;
	//Block aimed by the player
	std::vector<Block>::const_iterator m_SelectedBlock;
	//determining if side chunk exists
	std::vector<Chunk>::iterator m_PlusX, m_MinusX, m_PlusZ, m_MinusZ;
	static constexpr uint32_t m_ChunkWidthAndHeight = 16;
	static constexpr uint32_t m_ChunkDepth = 50;
};