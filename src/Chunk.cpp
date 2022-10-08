#include "Chunk.h"

Chunk::Chunk(glm::vec3 origin)
	:m_ChunkOrigin(origin)
{
	for (int32_t i = origin.x; i < origin.x + m_ChunkWidthAndHeight; i++)
		for (int32_t j = origin.y; j < origin.y + m_ChunkDepth; j++)
			for (int32_t k = origin.z; k < origin.z + m_ChunkWidthAndHeight; k++)
			{
				m_LocalBlocks.emplace_back(glm::vec3(i, j, k), GameDefs::BlockType::DIRT);
				auto& norm_vec = m_LocalBlocks.back().ExposedNormals();
				
				if (i == origin.x)
					norm_vec.emplace_back(-1.0f, 0.0f, 0.0f);
				if (i == origin.x + m_ChunkWidthAndHeight - 1)
					norm_vec.emplace_back(1.0f, 0.0f, 0.0f);
				if (j == origin.y)
					norm_vec.emplace_back(0.0f, -1.0f, 0.0f);
				if (j == origin.y + m_ChunkDepth - 1)
					norm_vec.emplace_back(0.0f, 1.0f, 0.0f);
				if (k == origin.z)
					norm_vec.emplace_back(0.0f, 0.0f, -1.0f);
				if (k == origin.z + m_ChunkWidthAndHeight - 1)
					norm_vec.emplace_back(0.0f, 0.0f, 1.0f);
			}
}

void Chunk::Draw(const GameDefs::RenderData& rd) const
{
	//For now blocks all share the same shader
	m_ChunkStructure.BlockRenderInit(rd, m_LocalBlocks[0].GetBlockShader());
	float closest_selected_block_dist = INFINITY;
	const Block* closest_block_ptr = nullptr;

	std::vector<std::pair<const Block&, bool>> ordered_renderable;

	for (auto& block : m_LocalBlocks)
	{
		//Discard automatically blocks which cant be selected
		if (!block.HasNormals())
			continue;

		float dist = 0.0f;
		bool bSelected = GameDefs::ViewBlockCollision(rd.camera_position, rd.camera_direction, block.GetPosition(), dist);
		ordered_renderable.push_back({block, false});

		for (auto& norm : block.ExposedNormals())
		{
			glm::vec3 block_pos_to_camera = rd.camera_position - block.GetPosition();

			if (glm::dot(block_pos_to_camera, norm) > 0.0f)
			{
				ordered_renderable.back().second = true;
				break;
			}
		}
		if (bSelected && dist < closest_selected_block_dist)
		{
			closest_selected_block_dist = dist;
			closest_block_ptr = &block;
		}
	}

	for(auto& rnd : ordered_renderable)
		if (rnd.second)
			rnd.first.Draw(&rnd.first == closest_block_ptr);
}
