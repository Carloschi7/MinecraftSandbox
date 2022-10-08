#pragma once
#include <iostream>
#include "GlStructure.h"

class Block
{
public:
    Block(const glm::vec3 position, const GameDefs::BlockType& bt);
    void Draw(bool bIsBlockSelected = false) const;

    const glm::vec3& GetPosition() const;
    std::shared_ptr<Shader> GetBlockShader() const;
    std::vector<glm::vec3>& ExposedNormals();
    const std::vector<glm::vec3>& ExposedNormals() const;
    bool HasNormals() const;
private:
    glm::vec3 m_Position;
    //Used to determine which sides are exposed, thus
    //determining if the cube can be drawn
    std::vector<glm::vec3> m_ExposedNormals;
    GameDefs::BlockType m_BlockType;
    GlCore::BlockStructure m_BlockStructure;
};

