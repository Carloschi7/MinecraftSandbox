#include "Block.h"

Block::Block(const glm::vec3 position, const GameDefs::BlockType &bt)
    :m_Position(position), m_BlockType(bt)
{
}

void Block::Draw() const
{
    m_BlockStructure.Draw(m_Position, m_BlockType);
}

std::shared_ptr<Shader> Block::GetBlockShader() const
{
    return m_BlockStructure.m_ShaderPtr;
}
