#include "Chunk.h"

Chunk::Chunk(glm::vec3 origin)
{
	for (int32_t i = origin.x; i < origin.x + m_ChunkWidthAndHeight; i++)
		for (int32_t j = origin.x; j < origin.y + m_ChunkWidthAndHeight; j++)
			for (int32_t k = origin.x; k < origin.z + m_ChunkDepth; k++)
				m_LocalBlocks.emplace_back(glm::vec3(i, j, k), GameDefs::BlockType::DIRT);
}
