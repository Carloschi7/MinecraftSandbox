#include "Chunk.h"
#include <algorithm>

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

void Chunk::BlockCollisionLogic(const GameDefs::ChunkBlockLogicData& ld)
{
	float closest_selected_block_dist = INFINITY;
	//Reset the selection each time
	m_SelectedBlock = m_LocalBlocks.cend();

	for (auto iter = m_LocalBlocks.begin(); iter != m_LocalBlocks.end(); ++iter)
	{
		auto& block = *iter;
		//Discard automatically blocks which cant be selected
		if (!block.HasNormals())
			continue;

		float dist = 0.0f;
		bool bSelected = GameDefs::ViewBlockCollision(ld.camera_position, ld.camera_direction, block.GetPosition(), dist);
		
		if (bSelected && dist < closest_selected_block_dist)
		{
			closest_selected_block_dist = dist;
			m_SelectedBlock = iter;
		}
	}

	if (m_SelectedBlock != m_LocalBlocks.cend() && ld.mouse_input.left_click)
	{
		AddNewExposedNormals(m_SelectedBlock);
		m_LocalBlocks.erase(m_SelectedBlock);
		m_SelectedBlock = m_LocalBlocks.cend();
	}
}

void Chunk::Draw(const GameDefs::RenderData& rd) const
{
	//For now blocks all share the same shader
	m_ChunkStructure.BlockRenderInit(rd, m_LocalBlocks[0].GetBlockShader());

	for (auto iter = m_LocalBlocks.begin(); iter != m_LocalBlocks.end(); ++iter)
	{
		auto& block = *iter;
		//Discard automatically blocks which cant be drawn
		if (!block.HasNormals())
			continue;

		bool bDrawable = false;
		for (auto& norm : block.ExposedNormals())
		{
			glm::vec3 block_pos_to_camera = rd.camera_position - block.GetPosition();

			if (glm::dot(block_pos_to_camera, norm) > 0.0f)
			{
				bDrawable = true;
				break;
			}
		}

		if(bDrawable)
			block.Draw(iter == m_SelectedBlock);
	}
}

void Chunk::AddNewExposedNormals(std::vector<Block>::const_iterator iter)
{
	auto compute_new_normals = [&](const glm::vec3& pos, const glm::vec3& norm)
	{
		glm::vec3 neighbor_pos = pos + norm;
		auto iter = std::find_if(m_LocalBlocks.begin(), m_LocalBlocks.end(),
			[neighbor_pos](const Block& b) {return b.GetPosition() == neighbor_pos; });

		if(iter != m_LocalBlocks.end())
			iter->ExposedNormals().push_back(-norm);
	};

	const glm::vec3& pos = iter->GetPosition();
	compute_new_normals(pos, glm::vec3(1.0f, 0.0f, 0.0f));
	compute_new_normals(pos, glm::vec3(-1.0f, 0.0f, 0.0f));
	compute_new_normals(pos, glm::vec3(0.0f, 1.0f, 0.0f));
	compute_new_normals(pos, glm::vec3(0.0f, -1.0f, 0.0f));
	compute_new_normals(pos, glm::vec3(0.0f, 0.0f, 1.0f));
	compute_new_normals(pos, glm::vec3(0.0f, 0.0f, -1.0f));
}
