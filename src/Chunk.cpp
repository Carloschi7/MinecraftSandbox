#include "Chunk.h"

#include "World.h"
#include <algorithm>
#include <chrono>

Chunk::Chunk(World* father, glm::vec2 origin)
	:m_RelativeWorld(father), m_ChunkOrigin(origin), m_IsSelectionHere(false)
{
	for (int32_t i = origin.x; i < origin.x + m_ChunkWidthAndHeight; i++)
		for (int32_t j = 0; j < m_ChunkDepth; j++)
			for (int32_t k = origin.y; k < origin.y + m_ChunkWidthAndHeight; k++)
				m_LocalBlocks.emplace_back(glm::vec3(i, j, k), (j == m_ChunkDepth -1) ?
					GameDefs::BlockType::GRASS : GameDefs::BlockType::DIRT);		
}

void Chunk::InitBlockNormals()
{
	//Determining if side chunk exist
	m_PlusX = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::PLUS_X);
	m_MinusX = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::MINUS_X);
	m_PlusZ = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::PLUS_Z);
	m_MinusZ = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::MINUS_Z);
	int32_t i, j, k;

	auto end_iter = m_RelativeWorld->ChunkCend();
	//Checking also for adjacent chunks
	for (auto& block : m_LocalBlocks)
	{
		i = block.GetPosition().x;
		j = block.GetPosition().y;
		k = block.GetPosition().z;

		auto& norm_vec = block.ExposedNormals();

		if (m_MinusX == end_iter && i == m_ChunkOrigin.x)
			block.ExposedNormals().emplace_back(-1.0f, 0.0f, 0.0f);
		if (m_PlusX == end_iter && i == m_ChunkOrigin.x + m_ChunkWidthAndHeight - 1)
			norm_vec.emplace_back(1.0f, 0.0f, 0.0f);
		if (j == 0.0f)
			norm_vec.emplace_back(0.0f, -1.0f, 0.0f);
		if (j == m_ChunkDepth - 1)
			norm_vec.emplace_back(0.0f, 1.0f, 0.0f);
		if (m_MinusZ == end_iter && k == m_ChunkOrigin.y)
			norm_vec.emplace_back(0.0f, 0.0f, -1.0f);
		if (m_PlusZ == end_iter && k == m_ChunkOrigin.y + m_ChunkWidthAndHeight - 1)
			norm_vec.emplace_back(0.0f, 0.0f, 1.0f);
	}
}

void Chunk::SetBlockSelected(bool selected) const
{
	m_IsSelectionHere = selected;
}

float Chunk::BlockCollisionLogic(const GameDefs::ChunkBlockLogicData& ld)
{
	float closest_selected_block_dist = INFINITY;
	//Reset the selection each time
	std::size_t vec_size = m_LocalBlocks.size();
	m_SelectedBlock = static_cast<uint32_t>(-1);

	//Checking selection
	for (std::size_t i = 0; i < vec_size; ++i)
	{
		auto& block = m_LocalBlocks[i];
		//Discard automatically blocks which cant be selected or seen
		if (!block.IsDrawable())
			continue;

		float dist = 0.0f;
		bool bSelected = GameDefs::ViewBlockCollision(ld.camera_position, ld.camera_direction, block.GetPosition(), dist);
		
		if (bSelected && dist < closest_selected_block_dist)
		{
			closest_selected_block_dist = dist;
			m_SelectedBlock = i;
		}
	}

	if (m_SelectedBlock != static_cast<uint32_t>(-1) && ld.mouse_input.left_click)
	{
		AddNewExposedNormals(m_LocalBlocks[m_SelectedBlock].GetPosition());
		m_LocalBlocks.erase(m_LocalBlocks.begin() + m_SelectedBlock);
		m_SelectedBlock = static_cast<uint32_t>(-1);
	}

	return closest_selected_block_dist;
}

void Chunk::UpdateBlocks(const GameDefs::ChunkBlockLogicData& ld)
{
	//Update each single block
	for (auto& block : m_LocalBlocks)
		block.UpdateRenderableSides(ld.camera_position);
}

const glm::vec2& Chunk::GetChunkOrigin() const
{
	return m_ChunkOrigin;
}

void Chunk::Draw(const GameDefs::RenderData& rd) const
{
	auto tp1 = std::chrono::steady_clock::now();
	std::size_t vec_size = m_LocalBlocks.size();

	for (std::size_t i = 0; i < vec_size; ++i)
	{
		auto& block = m_LocalBlocks[i];
		//Discard automatically blocks which cant be drawn
		if (!block.IsDrawable())
			continue;

		block.Draw(m_IsSelectionHere && i == m_SelectedBlock);
	}

	if (rd.p_key)
	{
		auto tp2 = std::chrono::steady_clock::now();
		std::cout << "FPS:" << 1.0f / std::chrono::duration<float>(tp2 - tp1).count() << std::endl;
	}
}

void Chunk::AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check)
{
	auto compute_new_normals = [&](const glm::vec3& pos, const glm::vec3& norm)
	{
		glm::vec3 neighbor_pos = pos + norm;
		auto local_iter = std::find_if(m_LocalBlocks.begin(), m_LocalBlocks.end(),
			[neighbor_pos](const Block& b) {return b.GetPosition() == neighbor_pos; });

		//Behaviour between chunks
		if (local_iter == m_LocalBlocks.end())
		{
			if (!side_chunk_check)
			{
				if (norm == glm::vec3(1.0f, 0.0f, 0.0f) && m_PlusX != m_RelativeWorld->ChunkCend())
					m_PlusX->AddNewExposedNormals(pos, true);
				if (norm == glm::vec3(-1.0f, 0.0f, 0.0f) && m_MinusX != m_RelativeWorld->ChunkCend())
					m_MinusX->AddNewExposedNormals(pos, true);
				if (norm == glm::vec3(0.0f, 0.0f, 1.0f) && m_PlusZ != m_RelativeWorld->ChunkCend())
					m_PlusZ->AddNewExposedNormals(pos, true);
				if (norm == glm::vec3(0.0f, 0.0f, -1.0f) && m_MinusZ != m_RelativeWorld->ChunkCend())
					m_MinusZ->AddNewExposedNormals(pos, true);
			}
		}
		else
		{
			local_iter->ExposedNormals().push_back(-norm);
		}
	};

	compute_new_normals(block_pos, glm::vec3(1.0f, 0.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3(-1.0f, 0.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3(0.0f, 1.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3(0.0f, -1.0f, 0.0f));
	compute_new_normals(block_pos, glm::vec3(0.0f, 0.0f, 1.0f));
	compute_new_normals(block_pos, glm::vec3(0.0f, 0.0f, -1.0f));
}
