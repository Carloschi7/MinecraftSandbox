#pragma once
#include <iostream>
#include "GlStructure.h"
#include "GameDefinitions.h"

class Block
{
public:
    Block(const glm::vec3 position, const GameDefs::BlockType& bt);
    void Draw() const;
    std::shared_ptr<Shader> GetBlockShader() const;
private:
    glm::vec3 m_Position;
    GameDefs::BlockType m_BlockType;
    GlCore::BlockStructure m_BlockStructure;
};

