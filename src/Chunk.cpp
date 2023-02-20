#include "Chunk.h"
#include "World.h"
#include <algorithm>
#include <chrono>

//Half a chunk's diagonal
float Chunk::s_DiagonalLenght = 0.0f;
uint32_t Chunk::s_InternalSelectedBlock = static_cast<uint32_t>(-1);

Chunk::Chunk(World* father, glm::vec2 origin)
	:m_RelativeWorld(father), m_ChunkOrigin(origin),
	m_SelectedBlock(static_cast<uint32_t>(-1)), m_ChunkCenter(0.0f),
	m_SectorIndex(0)
{
	//Assigning chunk index
	m_ChunkIndex = Gd::g_ChunkProgIndex++;

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
	{
		for (int32_t k = origin.y; k < origin.y + s_ChunkWidthAndHeight; k++)
		{
			auto perlin_data = Gd::PerlNoise::GetBlockAltitude(static_cast<float>(i), static_cast<float>(k), m_RelativeWorld->Seed());
			uint32_t final_height = (s_ChunkDepth - 10) + std::roundf(perlin_data.altitude * 8.0f);

			for (int32_t j = 0; j < final_height; j++)
			{
				switch (perlin_data.biome)
				{
				case Gd::Biome::PLAINS:
					m_LocalBlocks.emplace_back(glm::vec3(i, j, k), (j == final_height - 1) ?
						Gd::BlockType::GRASS : Gd::BlockType::DIRT);
					break;
				case Gd::Biome::DESERT:
					m_LocalBlocks.emplace_back(glm::vec3(i, j, k), Gd::BlockType::SAND);
					break;
				}
			}

			//Spawn a tree in the center
			if (perlin_data.biome == Gd::Biome::PLAINS &&
				i == origin.x + s_ChunkWidthAndHeight / 2 &&
				k == origin.y + s_ChunkWidthAndHeight / 2)
			{
				for(uint32_t p = 0; p < 4; p++)
					m_LocalBlocks.emplace_back(glm::vec3(i, final_height + p, k), Gd::BlockType::WOOD);

				//With its leaves
				for (int32_t x = -1; x <= 1; x++)
					for (int32_t z = -1; z <= 1; z++)
						for (int32_t y = 1; y <= 3; y++)
							m_LocalBlocks.emplace_back(glm::vec3(i + x, final_height + 3 + y, k + z), Gd::BlockType::LEAVES);
			}
		}
	}

	//Set chunk sector
	m_SectorIndex = Gd::ChunkSectorIndex(m_ChunkOrigin);
	Gd::g_PushedSections.insert(m_SectorIndex);
}

Chunk::Chunk(World* father, const Utils::Serializer& sz, uint32_t index) :
	m_RelativeWorld(father), m_SectorIndex(index), m_SelectedBlock(static_cast<uint32_t>(-1))
{
	//Simply forward everithing to the deserializing operator
	*this % sz;
}

Chunk::Chunk(Chunk&& rhs) noexcept
{
	*this = std::move(rhs);
}

Chunk::~Chunk()
{
}

Chunk& Chunk::operator=(Chunk&& rhs) noexcept
{
	m_RelativeWorld = rhs.m_RelativeWorld;
	m_ChunkIndex = rhs.m_ChunkIndex;

	m_LocalBlocks = std::move(rhs.m_LocalBlocks);
	m_ChunkOrigin = rhs.m_ChunkOrigin;
	m_ChunkCenter = rhs.m_ChunkCenter;
	m_SectorIndex = rhs.m_SectorIndex;

	m_PlusX = rhs.m_PlusX;
	m_MinusX = rhs.m_MinusX;
	m_PlusZ = rhs.m_PlusZ;
	m_MinusZ = rhs.m_MinusZ;
	return *this;
}

void Chunk::InitGlobalNorms()
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
	float local_x =  m_LocalBlocks[0].Position().x, local_z = m_LocalBlocks[0].Position().z;
	int32_t starting_index = 0;
	//+1 because we need to parse the last column
	for (int32_t i = 0; i < m_LocalBlocks.size() + 1; i++)
	{
		if (i != m_LocalBlocks.size() &&
			m_LocalBlocks[i].Position().x == local_x &&
			m_LocalBlocks[i].Position().z == local_z)
		{
			continue;
		}

		if (i != m_LocalBlocks.size())
		{
			local_x = m_LocalBlocks[i].Position().x;
			local_z = m_LocalBlocks[i].Position().z;
		}

		uint32_t top_column_index = i - 1;
		//The block at the top of the pile and the one at the bottom
		auto& top_block = m_LocalBlocks[top_column_index];
		auto& bot_block = m_LocalBlocks[starting_index];

		const glm::vec3& block_pos = top_block.Position();

		//if the blocks confines with another chunk right left front or back
		bool conf_rlfb[4];
		conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
		conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
		conf_rlfb[2] = block_pos.z == m_ChunkOrigin.y;
		conf_rlfb[3] = block_pos.z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1;

		//Upper block of a column is always visible from the top
		top_block.AddNormal(GlCore::g_PosY);

		int32_t p = top_column_index;
		if (conf_rlfb[0] || conf_rlfb[1] || conf_rlfb[2] || conf_rlfb[3])
		{
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
		//Push a normal at the bottom
		if(bot_block.Position().y != 0.0f)
			bot_block.AddNormal(GlCore::g_NegY);


		//Adding additional normals to side chunks, which were not considered because there hasn't
		//been a spawned chunk yet
		if (conf_rlfb[0] && chunk_plus_x)
		{
			uint32_t block_index;
			//Emplacing back normals from the first block higher than the current one 
			glm::vec3 begin_check = GlCore::g_PosY;
			while (chunk_plus_x->IsBlock(top_block.Position() + pos_x + begin_check, 0, true, &block_index))
			{
				chunk_plus_x->GetBlock(block_index).AddNormal(-1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[1] && chunk_minus_x)
		{
			uint32_t block_index;
			glm::vec3 begin_check = GlCore::g_PosY;
			while (chunk_minus_x->IsBlock(top_block.Position() + neg_x + begin_check, 0, true, &block_index))
			{
				chunk_minus_x->GetBlock(block_index).AddNormal(1.0f, 0.0f, 0.0f);
				begin_check.y += 1.0f;
			}
		}

		if (conf_rlfb[2] && chunk_minus_z)
		{
			uint32_t block_index;
			glm::vec3 local_pos = GlCore::g_PosY;
			while (chunk_minus_z->IsBlock(top_block.Position() + neg_z + local_pos, 0, true, &block_index))
			{
				chunk_minus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, 1.0f);
				local_pos.y += 1.0f;
			}
		}

		if (conf_rlfb[3] && chunk_plus_z)
		{
			uint32_t block_index;
			glm::vec3 local_pos = GlCore::g_PosY;
			while (chunk_plus_z->IsBlock(top_block.Position() + pos_z + local_pos, 0, true, &block_index))
			{
				chunk_plus_z->GetBlock(block_index).AddNormal(0.0f, 0.0f, -1.0f);
				local_pos.y += 1.0f;
			}
		}

		starting_index = i;
	}
}

void Chunk::AddFreshNormals(Block& b)
{
	const auto& block_pos = b.Position();

	bool conf_rlfb[4];
	conf_rlfb[0] = block_pos.x == m_ChunkOrigin.x + s_ChunkWidthAndHeight - 1;
	conf_rlfb[1] = block_pos.x == m_ChunkOrigin.x;
	conf_rlfb[2] = block_pos.z == m_ChunkOrigin.y;
	conf_rlfb[3] = block_pos.z == m_ChunkOrigin.y + s_ChunkWidthAndHeight - 1;

	auto local_lambda = [&](bool side_check, Gd::ChunkLocation cl, const glm::vec3& dir)
	{
		bool add_norm = true;
		if (side_check)
		{
			if (auto opt = m_RelativeWorld->IsChunk(*this, cl); opt.has_value())
			{
				if (m_RelativeWorld->GetChunk(opt.value()).IsBlock(block_pos + dir))
					add_norm = false;
			}
		}
		else
		{
			if (IsBlock(block_pos + dir))
				add_norm = false;
		}

		if (add_norm)
			b.AddNormal(dir);
	};

	local_lambda(conf_rlfb[0], Gd::ChunkLocation::PLUS_X, GlCore::g_PosX);
	local_lambda(conf_rlfb[1], Gd::ChunkLocation::MINUS_X, GlCore::g_NegX);
	local_lambda(false, Gd::ChunkLocation::NONE, GlCore::g_PosY);
	local_lambda(false, Gd::ChunkLocation::NONE, GlCore::g_NegY);
	local_lambda(conf_rlfb[2], Gd::ChunkLocation::PLUS_Z, GlCore::g_PosZ);
	local_lambda(conf_rlfb[3], Gd::ChunkLocation::MINUS_Z, GlCore::g_NegZ);
}

float Chunk::BlockCollisionLogic(bool left_click, bool right_click)
{
	float closest_selected_block_dist = INFINITY;
	//Reset the selection each time
	std::size_t vec_size = m_LocalBlocks.size();
	m_SelectedBlock = static_cast<uint32_t>(-1);

	auto& camera_position = GlCore::Root::GameCamera().GetPosition();
	auto& camera_direction = GlCore::Root::GameCamera().GetFront();

	Gd::HitDirection selection = Gd::HitDirection::NONE;

	//Checking selection
	for (std::size_t i = 0; i < vec_size; ++i)
	{
		auto& block = m_LocalBlocks[i];
		//Discard automatically blocks which cant be selected or seen
		if (!block.IsDrawable())
			continue;

		float dist = 0.0f;
		Gd::HitDirection local_selection = Gd::ViewBlockCollision(camera_position, camera_direction, block.Position(), dist);
		
		if (local_selection != Gd::HitDirection::NONE && dist < closest_selected_block_dist)
		{
			closest_selected_block_dist = dist;
			selection = local_selection;
			m_SelectedBlock = i;
		}
	}

	//Logic which removes a block
	if (left_click && m_SelectedBlock != static_cast<uint32_t>(-1))
	{
		AddNewExposedNormals(m_LocalBlocks[m_SelectedBlock].Position());
		m_LocalBlocks.erase(m_LocalBlocks.begin() + m_SelectedBlock);
		m_SelectedBlock = static_cast<uint32_t>(-1);

		//Signal block has been destroyed
		Gd::g_BlockDestroyed = true;
	}

	//Push a new block
	if (right_click && m_SelectedBlock != static_cast<uint32_t>(-1))
	{
		//if the selected block isn't -1 that means selection is not NONE

		auto bt = Gd::BlockType::DIRT;
		auto& block = m_LocalBlocks[m_SelectedBlock];

		switch (selection)
		{
		case Gd::HitDirection::POS_X:
			m_LocalBlocks.emplace_back(block.Position() + GlCore::g_PosX, bt);
			break;
		case Gd::HitDirection::NEG_X:
			m_LocalBlocks.emplace_back(block.Position() + GlCore::g_NegX, bt);
			break;
		case Gd::HitDirection::POS_Y:
			m_LocalBlocks.emplace_back(block.Position() + GlCore::g_PosY, bt);
			break;
		case Gd::HitDirection::NEG_Y:
			m_LocalBlocks.emplace_back(block.Position() + GlCore::g_NegY, bt);
			break;
		case Gd::HitDirection::POS_Z:
			m_LocalBlocks.emplace_back(block.Position() + GlCore::g_PosZ, bt);
			break;
		case Gd::HitDirection::NEG_Z:
			m_LocalBlocks.emplace_back(block.Position() + GlCore::g_NegZ, bt);
			break;

		case Gd::HitDirection::NONE:
			//UNREACHABLE
			break;
		}

		AddFreshNormals(m_LocalBlocks.back());
	}

	return closest_selected_block_dist;
}

void Chunk::UpdateBlocks()
{
	auto& camera_position = GlCore::Root::GameCamera().GetPosition();

	//Update each single block
	for (auto& block : m_LocalBlocks)
		block.UpdateRenderableSides(camera_position);
}

bool Chunk::IsChunkRenderable() const
{
	auto& camera_position = GlCore::Root::GameCamera().GetPosition();

	//This algorithm does not take account for the player altitude in space
	glm::vec2 cam_pos(camera_position.x, camera_position.z);
	glm::vec2 chunk_center_pos(m_ChunkCenter.x, m_ChunkCenter.z);
	return (glm::length(cam_pos - chunk_center_pos) < Gd::g_ChunkRenderingDistance);
}

bool Chunk::IsChunkVisible() const
{
	auto& camera_position = GlCore::Root::GameCamera().GetPosition();
	auto& camera_direction = GlCore::Root::GameCamera().GetFront();

	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - camera_position);
	return (glm::dot(camera_to_midway, camera_direction) > 0.4f ||
		glm::length(camera_position - m_ChunkCenter) < s_DiagonalLenght + Gd::g_CameraCompensation);
}

bool Chunk::IsChunkVisibleByShadow() const
{
	glm::vec3 camera_position = GlCore::Root::GameCamera().GetPosition() + GlCore::g_FramebufferPlayerOffset;
	const glm::vec3& camera_direction = GlCore::g_NegY;
	
	glm::vec3 camera_to_midway = glm::normalize(m_ChunkCenter - camera_position);
	return (glm::dot(camera_to_midway, camera_direction) > 0.5f);
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

void Chunk::Draw(bool depth_buf_draw, bool selected) const
{
	auto* dyn_pos = GlCore::g_DynamicPositionBuffer;
	auto* tex = GlCore::g_DynamicTextureIndicesBuffer;
	uint32_t count = 0;

	for (std::size_t i = 0; i < m_LocalBlocks.size(); ++i)
	{
		auto& block = m_LocalBlocks[i];
		//Discard automatically blocks which cant be drawn

		if (!depth_buf_draw)
		{
			//For standard frawcalls, we care about visible blocks
			if (!block.IsDrawable())
				continue;

			tex[count] = static_cast<uint32_t>(block.Type());
			dyn_pos[count++] = block.Position();

			//Handle selection by pushing another matrix and a null index
			if (selected && i == s_InternalSelectedBlock)
			{
				tex[count] = 256 + tex[count - 1];
				dyn_pos[count] = dyn_pos[count - 1];
				count++;
			}
		}
		else
		{
			//For depth drawcalls, we include every surface block
			if (!block.HasNormals())
				continue;

			dyn_pos[count++] = block.Position();
		}
	}

	if (count == 0)
		return;

	auto block_vm = GlCore::Root::BlockVM();
	auto depth_vm = GlCore::Root::DepthVM();

	//Decide which shader to use depending on the context
	if (!depth_buf_draw)
	{
		GlCore::Root::BlockShader()->Use();
		//Send all the matrices to the VertexManager, then draw everything in one instanced drawcall
		block_vm->BindVertexArray();
		block_vm->EditInstance(0, dyn_pos, sizeof(glm::vec3) * count, 0);
		block_vm->EditInstance(1, tex, sizeof(uint32_t) * count, 0);
	}
	else
	{
		GlCore::Root::DepthShader()->Use();
		depth_vm->BindVertexArray();
		depth_vm->EditInstance(0, dyn_pos, sizeof(glm::vec3) * count, 0);
	}
	

	GlCore::Renderer::RenderInstanced(count);
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
				if (m_PlusX.has_value() && norm == GlCore::g_PosX)
					m_RelativeWorld->GetChunk(m_PlusX.value()).AddNewExposedNormals(pos, true);
				if (m_MinusX.has_value() && norm == GlCore::g_NegX)
					m_RelativeWorld->GetChunk(m_MinusX.value()).AddNewExposedNormals(pos, true);
				if (m_PlusZ.has_value() && norm == GlCore::g_PosZ)
					m_RelativeWorld->GetChunk(m_PlusZ.value()).AddNewExposedNormals(pos, true);
				if (m_MinusZ.has_value() && norm == GlCore::g_NegZ)
					m_RelativeWorld->GetChunk(m_MinusZ.value()).AddNewExposedNormals(pos, true);
			}
		}
		else
		{
			local_iter->AddNormal(-norm);
		}
	};

	compute_new_normals(block_pos, GlCore::g_PosX);
	compute_new_normals(block_pos, GlCore::g_NegX);
	compute_new_normals(block_pos, GlCore::g_PosY);
	compute_new_normals(block_pos, GlCore::g_NegY);
	compute_new_normals(block_pos, GlCore::g_PosZ);
	compute_new_normals(block_pos, GlCore::g_NegZ);
}

uint32_t Chunk::LastSelectedBlock() const
{
	return m_SelectedBlock;
}

uint32_t Chunk::SectorIndex() const
{
	return m_SectorIndex;
}

uint32_t Chunk::Index() const
{
	return m_ChunkIndex;
}

glm::vec3 Chunk::GetHalfWayVector()
{
	return glm::vec3(s_ChunkWidthAndHeight / 2.0f, s_ChunkDepth / 2.0f, s_ChunkWidthAndHeight / 2.0f);
}

const Utils::Serializer& Chunk::operator&(const Utils::Serializer& sz)
{
	//Serializing components
	//At first we tell how many blocks the chunk has
	sz& m_LocalBlocks.size();
	sz& m_ChunkIndex;
	//World address does not need to be serialized

	if (!m_LocalBlocks.empty())
	{
		auto& base_vec = m_LocalBlocks[0].Position();
		//We serialize only a single 12bytes position vector,
		//so the other can be stored as 3bytes vector offsets
		sz& base_vec.x& base_vec.y& base_vec.z;

		for (auto& block : m_LocalBlocks)
			block.Serialize(sz, base_vec);
	}

	sz& m_ChunkOrigin.x & m_ChunkOrigin.y;
	//m_ChunkCenter can be calculated from m_ChunkOrigin
	//m_SelectedBlock does not need to be serialized

	sz& m_PlusX.value_or(-1)
		& m_MinusX.value_or(-1)
		& m_PlusZ.value_or(-1)
		& m_MinusZ.value_or(-1);

	return sz;
}

const Utils::Serializer& Chunk::operator%(const Utils::Serializer& sz)
{
	std::size_t blk_vec_size;
	glm::vec3 base_vec;
	uint32_t adj_chunks[4];

	sz% blk_vec_size;
	sz% m_ChunkIndex;

	if (blk_vec_size != 0)
	{
		m_LocalBlocks.clear();

		sz% base_vec.x% base_vec.y% base_vec.z;

		glm::u8vec3 v;
		uint8_t norm_payload, block_type;

		for (uint32_t i = 0; i < blk_vec_size; i++)
		{
			sz% v.x% v.y% v.z;
			sz% norm_payload;
			sz% block_type;

			glm::vec3 original_vec = base_vec + static_cast<glm::vec3>(v);
			m_LocalBlocks.emplace_back(original_vec, static_cast<Gd::BlockType>(block_type));
			auto& bb = m_LocalBlocks.back();
			
			Utils::Bitfield<6> bf( {norm_payload} );
			for (uint32_t i = 0; i < bf.Size(); i++)
				if (bf[i])
					bb.AddNormal(Block::NormalForIndex(i));
		}
	}

	sz% m_ChunkOrigin.x% m_ChunkOrigin.y;

	//Calculate chunk center
	glm::vec3 chunk_origin_3d(m_ChunkOrigin.x, 0.0f, m_ChunkOrigin.y);
	m_ChunkCenter = chunk_origin_3d + GetHalfWayVector();

	sz% adj_chunks[0] % adj_chunks[1] % adj_chunks[2] % adj_chunks[3];

	m_PlusX = adj_chunks[0] != -1 ? std::make_optional(adj_chunks[0]) : std::nullopt;
	m_MinusX = adj_chunks[1] != -1 ? std::make_optional(adj_chunks[1]) : std::nullopt;
	m_PlusZ = adj_chunks[2] != -1 ? std::make_optional(adj_chunks[2]) : std::nullopt;
	m_MinusZ = adj_chunks[3] != -1 ? std::make_optional(adj_chunks[3]) : std::nullopt;

	return sz;
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


