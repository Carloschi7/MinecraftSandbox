#pragma once
#include "Block.h"
#include "glm/glm.hpp"

class Chunk
{
public:
	//For now the chunk generates 16x16x16 blocks in the world
	//Origin specifies the bottom-left coordinate from which
	//generating blocks(only dirt generated for now)
	Chunk(glm::vec3 origin);
private:
	std::vector<Block> m_LocalBlocks;
	static constexpr uint32_t m_ChunkWidthAndHeight = 16;
	static constexpr uint32_t m_ChunkDepth = 16;
};