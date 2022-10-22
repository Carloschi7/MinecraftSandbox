#include "Block.h"
#include "Vertices.h"

Block::Block(const glm::vec3 position, const GameDefs::BlockType &bt)
    :m_Position(position), m_BlockType(bt), m_BlockStructure(position, bt)
{
}

void Block::Draw(bool bIsBlockSelected) const
{
    m_BlockStructure.Draw(m_DrawableSides, bIsBlockSelected);
}

void Block::UpdateRenderableSides(const glm::vec3& camera_pos)
{
    uint32_t& counter = m_DrawableSides.second;
    counter = 0;

    //Can never go above 3 elements
    glm::vec3 dir = m_Position - camera_pos;
    for (auto& norm : m_ExposedNormals)
        if (glm::dot(norm, -dir) > 0.0f)
            m_DrawableSides.first[counter++] = Utils::GetNormVertexBegin(norm);
}

const glm::vec3& Block::GetPosition() const
{
    return m_Position;
}


std::vector<glm::vec3>& Block::ExposedNormals()
{
    return m_ExposedNormals;
}

const GameDefs::DrawableData& Block::DrawableSides() const
{
    return m_DrawableSides;
}

const std::vector<glm::vec3>& Block::ExposedNormals() const
{
    return m_ExposedNormals;
}

bool Block::HasNormals() const
{
    return !m_ExposedNormals.empty();
}

bool Block::IsDrawable() const
{
    return m_DrawableSides.second != 0;
}

