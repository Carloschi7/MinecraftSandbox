#include "Block.h"
#include "Vertices.h"

Block::Block(const glm::vec3& position, const Defs::BlockType& bt)
    :m_Position(position), m_BlockType(bt), m_ExposedNormals{}
{
}

void Block::UpdateRenderableSides(const glm::vec3& camera_pos)
{
    uint8_t& counter = m_DrawableSides.second;
    counter = 0;

    //Should never go above 3 elements
    glm::vec3 dir = Position() - camera_pos;
    for (uint32_t i = 0; i < m_ExposedNormals.size(); i++)
    {
        const bool& is_norm = m_ExposedNormals[i];
        if (is_norm)
        {
            glm::vec3 norm = NormalForIndex(i);
            if (counter < 3 && glm::dot(norm, -dir) > 0.0f)
                m_DrawableSides.first[counter++] = GlCore::GetNormVertexBegin(norm);
        }
    }
}

const glm::vec3& Block::Position() const
{
    return m_Position;
}

const Defs::BlockType& Block::Type() const
{
    return m_BlockType;
}

const GlCore::DrawableData& Block::DrawableSides() const
{
    return m_DrawableSides;
}

void Block::AddNormal(const glm::vec3& norm)
{
    m_ExposedNormals[IndexForNormal(norm)] = true;
}

void Block::AddNormal(float x, float y, float z)
{
    AddNormal(glm::vec3(x, y, z));
}

void Block::RemoveNormal(const glm::vec3& norm)
{
    m_ExposedNormals[IndexForNormal(norm)] = false;
}

void Block::RemoveNormal(float x, float y, float z)
{
    RemoveNormal(glm::vec3(x, y, z));
}

bool Block::HasNormals() const
{
    for (bool b : m_ExposedNormals)
        if (b)
            return true;

    return false;
}

bool Block::IsDrawable() const
{
    return m_DrawableSides.second;
}

void Block::Serialize(const Utils::Serializer& sz, const glm::vec3& base_pos)
{
    auto offset_vec = static_cast<glm::u8vec3>(m_Position - base_pos);
    sz& offset_vec.x& offset_vec.y& offset_vec.z;

    Utils::Bitfield<6> bitfield;
    for (uint32_t i = 0; i < m_ExposedNormals.size(); i++)
        bitfield.Set(i, m_ExposedNormals[i]);

    sz& bitfield.Getu8Payload(0);
    //m_BlockStructure and m_DrawableData do not need to be serialized

    sz& static_cast<uint8_t>(m_BlockType);
}

uint32_t Block::IndexForNormal(const glm::vec3& vec)
{
    if (vec == glm::vec3(1.0f, 0.0f, 0.0f))
        return 0;
    if (vec == glm::vec3(-1.0f, 0.0f, 0.0f))
        return 1;
    if (vec == glm::vec3(0.0f, 1.0f, 0.0f))
        return 2;
    if (vec == glm::vec3(0.0f, -1.0f, 0.0f))
        return 3;
    if (vec == glm::vec3(0.0f, 0.0f, 1.0f))
        return 4;
    if (vec == glm::vec3(0.0f, 0.0f, -1.0f))
        return 5;

    return static_cast<uint32_t>(-1);
}

glm::vec3 Block::NormalForIndex(uint32_t index)
{
    switch (index)
    {
    case 0:
        return glm::vec3(1.0f, 0.0f, 0.0f);
    case 1:
        return glm::vec3(-1.0f, 0.0f, 0.0f);
    case 2:
        return glm::vec3(0.0f, 1.0f, 0.0f);
    case 3:
        return glm::vec3(0.0f, -1.0f, 0.0f);
    case 4:
        return glm::vec3(0.0f, 0.0f, 1.0f);
    case 5:
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }

    return glm::vec3(0.0f);
}

