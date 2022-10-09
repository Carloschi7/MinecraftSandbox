#include "Block.h"

Block::Block(const glm::vec3 position, const GameDefs::BlockType &bt)
    :m_Position(position), m_BlockType(bt)
{
}

void Block::Draw(bool bIsBlockSelected) const
{
    m_BlockStructure.Draw(m_Position, m_BlockType, bIsBlockSelected);
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

const std::vector<glm::vec3>& Block::ExposedNormals() const
{
    return m_ExposedNormals;
}

bool Block::HasNormals() const
{
    return !m_ExposedNormals.empty();
}

