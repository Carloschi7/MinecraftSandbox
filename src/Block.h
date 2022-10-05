#pragma once
#include <iostream>
#include "GlStructure.h"

class Block
{
public:
    Block(const glm::vec3 position, const GlCore::BlockType& bt);
private:
    glm::vec3 m_Position;
    GlCore::BlockStructure m_BlockStructure;
};

