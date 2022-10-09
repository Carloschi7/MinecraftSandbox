#pragma once
#include "Block.h"
#include "glm/glm.hpp"

class Chunk
{
public:
	//For now the chunk generates 16x16x16 blocks in the world
	//Origin specifies the front-bottom-left coordinate from which
	//generating blocks(only dirt generated for now)
	Chunk(glm::vec3 origin);
	void BlockCollisionLogic(const GameDefs::ChunkBlockLogicData& ld);
	void Draw(const GameDefs::RenderData& rd) const;
private:
	void AddNewExposedNormals(std::vector<Block>::const_iterator iter);
private:
	std::vector<Block> m_LocalBlocks;
	GlCore::ChunkStructure m_ChunkStructure;
	//front-bottom-left block position
	glm::vec3 m_ChunkOrigin;
	//Block aimed by the player
	std::vector<Block>::const_iterator m_SelectedBlock;
	static constexpr uint32_t m_ChunkWidthAndHeight = 16;
	static constexpr uint32_t m_ChunkDepth = 16;
};