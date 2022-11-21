#include "Block.h"
#include "Vertices.h"

Block::Block(const glm::vec3& position, const GameDefs::BlockType& bt)
    :m_Position(position), m_BlockType(bt), m_BlockStructure(position, bt),
    m_ExposedNormals{}
{
}

void Block::Draw(bool bIsBlockSelected) const
{
    m_BlockStructure.Draw(m_Position, m_BlockType, m_DrawableSides, bIsBlockSelected);
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
            glm::vec3 norm = _NormalForIndex(i);
            if (counter < 3 && glm::dot(norm, -dir) > 0.0f)
                m_DrawableSides.first[counter++] = GlCore::GetNormVertexBegin(norm);
        }
    }
}

const glm::vec3& Block::Position() const
{
    return m_Position;
}

const GlCore::DrawableData& Block::DrawableSides() const
{
    return m_DrawableSides;
}

void Block::AddNormal(const glm::vec3& norm)
{
    m_ExposedNormals[_IndexForNormal(norm)] = true;
}

void Block::AddNormal(float x, float y, float z)
{
    AddNormal(glm::vec3(x, y, z));
}

void Block::RemoveNormal(const glm::vec3& norm)
{
    m_ExposedNormals[_IndexForNormal(norm)] = false;
}

void Block::RemoveNormal(float x, float y, float z)
{
    RemoveNormal(glm::vec3(x, y, z));
}

bool Block::HasNormals() const
{
    return !m_ExposedNormals.empty();
}

bool Block::IsDrawable() const
{
    return m_DrawableSides.second;
}

uint32_t Block::_IndexForNormal(const glm::vec3& vec)
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

glm::vec3 Block::_NormalForIndex(uint32_t index)
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

