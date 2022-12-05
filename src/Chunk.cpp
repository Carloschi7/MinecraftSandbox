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

	//Insert the y coordinates consecutively to allow the
	//normal insertion algorithm later
	for (int32_t i = origin.x; i < origin.x + s_ChunkWidthAndHeight; i++)
		for (int32_t k = origin.y; k < origin.y + s_ChunkWidthAndHeight; k++)
		{
			float perlin_height = Gd::PerlNoise::GetBlockAltitude(static_cast<float>(i), static_cast<float>(k), m_RelativeWorld->Seed());
			uint32_t final_height = (s_ChunkDepth - 10) + std::roundf(perlin_height * 8.0f);

			for (int32_t j = 0; j < final_height; j++)
				m_LocalBlocks.emplace_back(glm::vec3(i, j, k), (j == final_height - 1) ?
					Gd::BlockType::GRASS : Gd::BlockType::DIRT);
		}
}

Chunk::~Chunk()
{
}

void Chunk::InitBlockNormals()
{
	//Determining if side chunk exist
	m_PlusX = m_RelativeWorld->IsChunk(*this, Gd::ChunkLocation::PLUS_X);
	m_MinusX = m_RelativeWorld->IsChunk(*this, Gd::ChunkLocation::MINUS_X);
	m_PlusZ = m_RelativeWorld->IsChunk(*this, Gd::ChunkLocation::PLUS_Z);
	m_MinusZ = m_RelativeWorld->IsChunk(*this, Gd::ChunkLocation::MINUS_Z);

	//Getting the relative ptr(can be nullptr obv)
	Chunk* chunk_plus_x = m_PlusX.has_value() ? &m_RelativeWorld->GetChunk(m_PlusX.value()) : nullptr;
	Chunk* chunk_minus_x = m_MinusX.has_value() ? &m_RelativeWorld->GetChunk(m_MinusX.value()) : nullptr;
	Chunk* chunk_plus_z = m_PlusZ.has_value() ? &m_RelativeWorld->GetChunk(m_PlusZ.value()) : nullptr;
	Chunk* chunk_minus_z = m_MinusZ.has_value() ? &m_RelativeWorld->GetChunk(m_MinusZ.value()) : nullptr;

	//Get normals
	const glm::vec3& pos_x = GlCore::g_PosX;
	const glm::vec3& neg_x = GlCore::g_NegX;
	const glm::vec3& pos_z = GlCore::g_PosZ;
	const glm::vec3& neg_z = GlCore::g_NegZ;

	//The following algorithm determines with precise accuracy and pretty fast speed which faces of each
	//block of the this chunk can be seen
	//The algorithm analyses each column of blocks in the 16x16 which can be found in the chunk and
	//from the top block of that column an index descends assigning to each side a normal
	//until a side block is found
	float local_z = m_LocalBlocks[0].Position().z;
	int32_t starting_index = 0;
	//+1 because we need to parse the last column
	for (int32_t i = 0; i < m_LocalBlocks.size() + 1; i++)
	{
		if (i != m_LocalBlocks.size() && m_LocalBlocks[i].Position().z == local_z)
			continue;

		if (i != m_LocalBlocks.size())
			local_z = m_LocalBlocks[i].Position().z;

		uint32_t top_column_index = i - 1;
		const glm::vec3& block_pos = m_LocalBlocks[top_column_index].Position();

		//if the blocks confines with another chunk right left front or back
		bool conf_rlfb[4];
		conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
		conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
		conf_rlfb[2] = block_pos.z == m_ChunkOrigin.y;
		conf_rlfb[3] = block_pos.z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1;

		//Upper block of a column is always visible from the top
		m_LocalBlocks[top_column_index].AddNormal(GlCore::g_PosY);

		if (conf_rlfb[0] || conf_rlfb[1] || conf_rlfb[2] || conf_rlfb[3])
		{
			int32_t p = top_column_index;
			//Checking also for neighbor chunks
			//Right
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + pos_x, starting_index, true) &&
				(!chunk_plus_x || !chunk_plus_x->IsBlock(m_LocalBlocks[p].Position() + pos_x)))
			{
				//If the block confines with a chunk that does not exist yet, we wont
				//push any normals, they will be deleted anyway when a new chunk spawns
				if (conf_rlfb[0] && !chunk_plus_x)
					break;

				m_LocalBlocks[p].AddNormal(pos_x);
				p--;
			}
			p = top_column_index;
			//Left
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + neg_x, starting_index, false) &&
				(!chunk_minus_x || !chunk_minus_x->IsBlock(m_LocalBlocks[p].Position() + neg_x)))
			{
				if (conf_rlfb[1] && !chunk_minus_x)
					break;

				m_LocalBlocks[p].AddNormal(neg_x);
				p--;
			}
			p = top_column_index;
			//Back
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + pos_z, starting_index, true) &&
				(!chunk_plus_z || !chunk_plus_z->IsBlock(m_LocalBlocks[p].Position() + pos_z)))
			{
				if (conf_rlfb[3] && !chunk_plus_z)
					break;

				m_LocalBlocks[p].AddNormal(pos_z);
				p--;
			}
			p = top_column_index;
			//Front
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + neg_z, starting_index, false) &&
				(!chunk_minus_z || !chunk_minus_z->IsBlock(m_LocalBlocks[p].Position() + neg_z)))
			{
				if (conf_rlfb[2] && !chunk_minus_z)
					break;

				m_LocalBlocks[p].AddNormal(neg_z);
				p--;
			}
		}
		else
		{
			//A more slim implementation for the majority of the iterations
			uint32_t p = top_column_index;
			//Right
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + pos_x, starting_index, true))
			{
				m_LocalBlocks[p].AddNormal(pos_x);
				p--;
			}
			p = top_column_index;
			//Left
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + neg_x, starting_index, false))
			{
				m_LocalBlocks[p].AddNormal(neg_x);
				p--;
			}
			p = top_column_index;
			//Back
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + pos_z, starting_index, true))
			{
				m_LocalBlocks[p].AddNormal(pos_z);
				p--;
			}
			p = top_column_index;
			//Front
			while (p >= starting_index && !IsBlock(m_LocalBlocks[p].Position() + neg_z, starting_index, false))
			{
				m_LocalBlocks[p].AddNormal(neg_z);
				p--;
			}
		}

		//Adding additional normals to side chunks, which were not considered because there hasn't
		//been a spawned chunk yet
		if (conf_rlfb[0] && chunk_plus_x)
		{
			uint32_t block_index;
			//Emplacing back normals from the first block higher than the current one 
			glm::vec3 begin_check = GlCore::g_PosY;
			while (chunk_plus_x->IsBlock(m_LocalBlocks[top_column_index].Position() + pos_x + begin_check, 0, true, &block_index))
			{
				chunk_plus_x->GetBlock(block_index).AddNormal(-1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[1] && chunk_minus_x)
		{
			uint32_t block_index;
			glm::vec3 begin_check = GlCore::g_PosY;
			while (chunk_minus_x->IsBlock(m_LocalBlocks[top_column_index].Position() + neg_x + begin_check, 0, true, &block_index))
			{
				chunk_minus_x->GetBlock(block_index).AddNormal(1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[2] && chunk_minus_z)
		{
			uint32_t block_index;
			glm::vec3 local_pos{ 0.0f, 1.0f, 0.0f };
			while (chunk_minus_z->IsBlock(m_LocalBlocks[top_column_index].Position() + neg_z + local_pos, 0, true, &block_index))
			{
				chunk_minus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, 1.0f);
				local_pos.y += 1.0f;
			}
		}

		if (conf_rlfb[3] && chunk_plus_z)
		{
			uint32_t block_index;
			glm::vec3 local_pos{ 0.0f, 1.0f, 0.0f };
			while (chunk_plus_z->IsBlock(m_LocalBlocks[top_column_index].Position() + pos_z + local_pos, 0, true, &block_index))
			{
				chunk_plus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, -1.0f);
				local_pos.y += 1.0f;
			}
		}

		starting_index = i;
	}
}

void Chunk::SetBlockSelected(bool selected) const
{
	m_IsSelectionHere = selected;
}

float Chunk::BlockCollisionLogic(const Gd::ChunkLogicData& ld)
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
		bool bSelected = Gd::ViewBlockCollision(ld.camera_position, ld.camera_direction, block.Position(), dist);
		
		if (bSelected && dist < closest_selected_block_dist)
		{
			closest_selected_block_dist = dist;
			m_SelectedBlock = i;
		}
	}

	//Logic which removes a block
	if (m_SelectedBlock != static_cast<uint32_t>(-1) && ld.mouse_input.left_click)
	{
		AddNewExposedNormals(m_LocalBlocks[m_SelectedBlock].Position());
		m_LocalBlocks.erase(m_LocalBlocks.begin() + m_SelectedBlock);
		m_SelectedBlock = static_cast<uint32_t>(-1);
	}

	return closest_selected_block_dist;
}

void Chunk::UpdateBlocks(const Gd::ChunkLogicData& ld)
{
	//Update each single block
	for (auto& block : m_LocalBlocks)
		block.UpdateRenderableSides(ld.camera_position);
}

bool Chunk::IsChunkRenderable(const Gd::ChunkLogicData& rd) const
{
	//This algorithm does not take account for the player altitude in space
	glm::vec2 cam_pos(rd.camera_position.x, rd.camera_position.z);
	glm::vec2 chunk_center_pos(m_ChunkCenter.x, m_ChunkCenter.z);
	return (glm::length(cam_pos - chunk_center_pos) < Gd::g_ChunkRenderingDistance);
}

bool Chunk::IsChunkVisible(const Gd::ChunkLogicData& rd) const
{
	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - rd.camera_position);
	return (glm::dot(camera_to_midway, rd.camera_direction) > 0.0f ||
		glm::length(rd.camera_position - m_ChunkCenter) < s_DiagonalLenght + Gd::g_CameraCompensation);

	//The 5.0f is just an arbitrary value to fix drawing issues that would be
	//to unnecessarily complex to fix precisely
}

void Chunk::RemoveBorderNorm(const glm::vec3& norm)
{
	//Function that determines if a block normal towards the chunk can be removed,
	//depending on if there is a block next to him in the desired direction.
	//If there is no block, the normal isn't going to be removed
	auto erase_flanked_normal = [&](Block& block, const glm::vec3& vec, const Gd::ChunkLocation& loc)
	{
		uint32_t index = GetLoadedChunk(loc).value_or(0);
		glm::vec3 block_pos = block.Position() + vec;
		bool is_block = m_RelativeWorld->GetChunk(index).IsBlock(block_pos);

		if (is_block)
			block.RemoveNormal(norm);
	};

	//I remind you that the y component in the chunk origin is actually the world z component
	if (norm == glm::vec3(1.0f, 0.0f, 0.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1)
				erase_flanked_normal(block, norm, Gd::ChunkLocation::PLUS_X);
	}
	else if (norm == glm::vec3(-1.0f, 0.0f, 0.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().x == m_ChunkOrigin.x)
				erase_flanked_normal(block, norm, Gd::ChunkLocation::MINUS_X);
	}
	else if (norm == glm::vec3(0.0f, 0.0f, 1.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1)
				erase_flanked_normal(block, norm, Gd::ChunkLocation::PLUS_Z);
	}
	else if (norm == glm::vec3(0.0f, 0.0f, -1.0f))
	{
		for (auto& block : m_LocalBlocks)
			if (block.Position().z == m_ChunkOrigin.y)
				erase_flanked_normal(block, norm, Gd::ChunkLocation::MINUS_Z);
	}

}

const glm::vec2& Chunk::GetChunkOrigin() const
{
	return m_ChunkOrigin;
}

const std::optional<uint32_t>& Chunk::GetLoadedChunk(const Gd::ChunkLocation& cl) const
{
	switch (cl)
	{
	case Gd::ChunkLocation::PLUS_X:
		return m_PlusX;
	case Gd::ChunkLocation::MINUS_X:
		return m_MinusX;
	case Gd::ChunkLocation::PLUS_Z:
		return m_PlusZ;
	case Gd::ChunkLocation::MINUS_Z:
		return m_MinusZ;
	}

	return std::nullopt;
}

void Chunk::SetLoadedChunk(const Gd::ChunkLocation& cl, uint32_t value)
{
	switch (cl)
	{
	case Gd::ChunkLocation::PLUS_X:
		m_PlusX = value;
		break;
	case Gd::ChunkLocation::MINUS_X:
		m_MinusX = value;
		break;
	case Gd::ChunkLocation::PLUS_Z:
		m_PlusZ = value;
		break;
	case Gd::ChunkLocation::MINUS_Z:
		m_MinusZ = value;
		break;
	}
}

void Chunk::Draw(const Gd::RenderData& rd) const
{
	for (std::size_t i = 0; i < m_LocalBlocks.size(); ++i)
	{
		auto& block = m_LocalBlocks[i];
		//Discard automatically blocks which cant be drawn
		if (!block.IsDrawable())
			continue;

		block.Draw(m_IsSelectionHere && i == m_SelectedBlock);
	}
}

void Chunk::AddNewExposedNormals(const glm::vec3& block_pos, bool side_chunk_check)
{
	auto compute_new_normals = [&](const glm::vec3& pos, const glm::vec3& norm)
	{
		glm::vec3 neighbor_pos = pos + norm;
		auto local_iter = std::find_if(m_LocalBlocks.begin(), m_LocalBlocks.end(),
			[neighbor_pos](const Block& b) {return b.Position() == neighbor_pos; });

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
			local_iter->AddNormal(-norm);
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

bool Chunk::IsBlock(const glm::vec3& pos, int32_t starting_index, bool search_towards_end, uint32_t* block_index) const
{
	if (starting_index < 0 || starting_index >= m_LocalBlocks.size())
		throw std::runtime_error("Starting index out of bounds");

	if (search_towards_end)
	{
		//Searching before in the defined batch
		for (int32_t i = starting_index; i < m_LocalBlocks.size(); i++)
		{
			if (m_LocalBlocks[i].Position() == pos)
			{
				if (block_index)
					*block_index = i;

				return true;
			}
		}

	}
	else
	{
		for (int32_t i = starting_index; i >= 0; i--)
		{
			if (m_LocalBlocks[i].Position() == pos)
			{
				if (block_index)
					*block_index = i;

				return true;
			}
		}
	}

	return false;
}

Block& Chunk::GetBlock(uint32_t index)
{
	return m_LocalBlocks[index];
}

const Block& Chunk::GetBlock(uint32_t index) const
{
	return m_LocalBlocks[index];
}
