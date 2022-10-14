#include "Block.h"

Block::Block(const glm::vec3 position, const GameDefs::BlockType &bt)
    :m_Position(position), m_BlockType(bt), m_BlockStructure(position, bt)
{
    m_DrawableNormals.reserve(3);
}

void Block::Draw(bool bIsBlockSelected) const
{
    m_BlockStructure.Draw(m_DrawableNormals, bIsBlockSelected);
}

void Block::UpdateRenderableSides(const glm::vec3& camera_pos)
{
    m_DrawableNormals.clear();

    glm::vec3 dir = m_Position - camera_pos;
    for (auto& norm : m_ExposedNormals)
        if (glm::dot(norm, -dir) > 0.0f)
            m_DrawableNormals.push_back(norm);
}

const glm::vec3& Block::GetPosition() const
{
    return m_Position;
}

std::shared_ptr<Shader> Block::GetBlockShader() const
{
    return m_BlockStructure.GetShader();
}

std::vector<glm::vec3>& Block::ExposedNormals()
{
    return m_ExposedNormals;
}

const std::vector<glm::vec3>& Block::DrawableNormals() const
{
    return m_DrawableNormals;
}

const std::vector<glm::vec3>& Block::ExposedNormals() const
{
    return m_ExposedNormals;
}

bool Block::HasNormals() const
{
    return !m_ExposedNormals.empty();
}

