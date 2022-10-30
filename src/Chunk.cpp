#include "Chunk.h"

#include "World.h"
#include <algorithm>
#include <chrono>

//Half a chunk's diagonal
float Chunk::s_DiagonalLenght = 0.0f;

Chunk::Chunk(World* father, glm::vec2 origin)
	:m_RelativeWorld(father), m_ChunkOrigin(origin), m_IsSelectionHere(false), 
	m_SelectedBlock(static_cast<uint32_t>(-1)), m_ChunkCenter(0.0f)
{
	glm::vec3 chunk_origin_3d(m_ChunkOrigin.x, 0.0f, m_ChunkOrigin.y);
	m_ChunkCenter = chunk_origin_3d + GetHalfWayVector();

	if (s_DiagonalLenght == 0.0f)
	{
		float lower_diag_squared = 2 * glm::pow(s_ChunkWidthAndHeight, 2);
		s_DiagonalLenght = glm::sqrt(lower_diag_squared + glm::pow(s_ChunkDepth, 2)) * 0.5f;
	}

	for (int32_t i = origin.x; i < origin.x + s_ChunkWidthAndHeight; i++)
		for (int32_t j = 0; j < s_ChunkDepth; j++)
			for (int32_t k = origin.y; k < origin.y + s_ChunkWidthAndHeight; k++)
				m_LocalBlocks.emplace_back(glm::vec3(i, j, k), (j == s_ChunkDepth -1) ?
					GameDefs::BlockType::GRASS : GameDefs::BlockType::DIRT);		
}

Chunk::~Chunk()
{
}

void Chunk::InitBlockNormals()
{
	//Determining if side chunk exist
	m_PlusX = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::PLUS_X);
	m_MinusX = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::MINUS_X);
	m_PlusZ = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::PLUS_Z);
	m_MinusZ = m_RelativeWorld->IsChunk(*this, GameDefs::ChunkLocation::MINUS_Z);
	int32_t i, j, k;

	//Checking also for adjacent chunks
	for (auto& block : m_LocalBlocks)
	{
		i = block.GetPosition().x;
		j = block.GetPosition().y;
		k = block.GetPosition().z;

		auto& norm_vec = block.ExposedNormals();

		if (!m_MinusX.has_value() && i == m_ChunkOrigin.x)
			norm_vec.emplace_back(-1.0f, 0.0f, 0.0f);
		if (!m_PlusX.has_value() && i == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1)
			norm_vec.emplace_back(1.0f, 0.0f, 0.0f);
		if (j == 0.0f)
			norm_vec.emplace_back(0.0f, -1.0f, 0.0f);
		if (j == s_ChunkDepth - 1)
			norm_vec.emplace_back(0.0f, 1.0f, 0.0f);
		if (!m_MinusZ.has_value() && k == m_ChunkOrigin.y)
			norm_vec.emplace_back(0.0f, 0.0f, -1.0f);
		if (!m_PlusZ.has_value() && k == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1)
			norm_vec.emplace_back(0.0f, 0.0f, 1.0f);
	}
}

void Chunk::SetBlockSelected(bool selected) const
{
	m_IsSelectionHere = selected;
}

float Chunk::BlockCollisionLogic(const GameDefs::ChunkLogicData& ld)
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

void Chunk::UpdateBlocks(const GameDefs::ChunkLogicData& ld)
{
	//Update each single block
	for (auto& block : m_LocalBlocks)
		block.UpdateRenderableSides(ld.camera_position);
}

bool Chunk::IsChunkRenderable(const GameDefs::ChunkLogicData& rd) const
{
	//This algorithm does not take account for the player altitude in space
	glm::vec2 cam_pos(rd.camera_position.x, rd.camera_position.z);
	glm::vec2 chunk_center_pos(m_ChunkCenter.x, m_ChunkCenter.z);
	return (glm::length(cam_pos - chunk_center_pos) < 100.0f);
}

bool Chunk::IsChunkVisible(const GameDefs::ChunkLogicData& rd) const
{
	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - rd.camera_position);
	return (glm::dot(camera_to_midway, rd.camera_direction) > 0.0f ||
		glm::length(rd.camera_position - m_ChunkCenter) < s_DiagonalLenght + 5.0f);

	//The 5.0f is just an arbitrary value to fix drawing issues that would be
	//to unnecessarily complex to fix precisely
}

void Chunk::RemoveBorderNorm(const glm::vec3& norm)
{
	if (norm == glm::vec3(1.0f, 0.0f, 0.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.GetPosition().x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1)
				block.ExposedNormals().erase(NormalAt(block, norm));
	}
	else if (norm == glm::vec3(-1.0f, 0.0f, 0.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.GetPosition().x == m_ChunkOrigin.x)
				block.ExposedNormals().erase(NormalAt(block, norm));
	}
	else if (norm == glm::vec3(0.0f, 0.0f, 1.0f))
	{
		//I remind you that the y component in the chunk origin is actually the world z component
		for (auto& block : m_LocalBlocks)
			if (block.GetPosition().z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1)
				block.ExposedNormals().erase(NormalAt(block, norm));
	}
	else if (norm == glm::vec3(0.0f, 0.0f, -1.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.GetPosition().z == m_ChunkOrigin.y)
				block.ExposedNormals().erase(NormalAt(block, norm));
	}

}

const glm::vec2& Chunk::GetChunkOrigin() const
{
	return m_ChunkOrigin;
}

const std::optional<uint32_t>& Chunk::GetLoadedChunk(const GameDefs::ChunkLocation& cl) const
{
	switch (cl)
	{
	case GameDefs::ChunkLocation::PLUS_X:
		return m_PlusX;
	case GameDefs::ChunkLocation::MINUS_X:
		return m_MinusX;
	case GameDefs::ChunkLocation::PLUS_Z:
		return m_PlusZ;
	case GameDefs::ChunkLocation::MINUS_Z:
		return m_MinusZ;
	}

	return std::nullopt;
}

void Chunk::SetLoadedChunk(const GameDefs::ChunkLocation& cl, uint32_t value)
{
	switch (cl)
	{
	case GameDefs::ChunkLocation::PLUS_X:
		m_PlusX = value;
		break;
	case GameDefs::ChunkLocation::MINUS_X:
		m_MinusX = value;
		break;
	case GameDefs::ChunkLocation::PLUS_Z:
		m_PlusZ = value;
		break;
	case GameDefs::ChunkLocation::MINUS_Z:
		m_MinusZ = value;
		break;
	}
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
				if (m_PlusX.has_value() && norm == glm::vec3(1.0f, 0.0f, 0.0f))
					m_RelativeWorld->GetChunk(m_PlusX.value()).AddNewExposedNormals(pos, true);
				if (m_MinusX.has_value() && norm == glm::vec3(-1.0f, 0.0f, 0.0f))
					m_RelativeWorld->GetChunk(m_MinusX.value()).AddNewExposedNormals(pos, true);
				if (m_PlusZ.has_value() && norm == glm::vec3(0.0f, 0.0f, 1.0f))
					m_RelativeWorld->GetChunk(m_PlusZ.value()).AddNewExposedNormals(pos, true);
				if (m_MinusZ.has_value() && norm == glm::vec3(0.0f, 0.0f, -1.0f))
					m_RelativeWorld->GetChunk(m_MinusZ.value()).AddNewExposedNormals(pos, true);
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

glm::vec3 Chunk::GetHalfWayVector()
{
	return glm::vec3(s_ChunkWidthAndHeight / 2.0f, s_ChunkDepth / 2.0f, s_ChunkWidthAndHeight / 2.0f);
}

std::vector<glm::vec3>::const_iterator Chunk::NormalAt(const Block& b, const glm::vec3& norm)
{
	return std::find_if(b.ExposedNormals().begin(), b.ExposedNormals().end(),
		[&](const glm::vec3& v) {return v == norm; });
}
