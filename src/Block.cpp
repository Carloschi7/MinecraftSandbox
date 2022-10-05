#include "Block.h"

Block::Block(const glm::vec3 position, const GlCore::BlockType &bt)
    :m_Position(position), m_BlockStructure(bt)
{

}