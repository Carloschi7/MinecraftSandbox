#pragma once
#include <iostream>
#include "GlStructure.h"

class Block
{
public:
    Block(const glm::vec3 position, const GameDefs::BlockType& bt);
    void Draw(bool bIsBlockSelected = false) const;
    void UpdateRenderableSides(const glm::vec3& camera_pos);

    const glm::vec3& GetPosition() const;
    std::vector<glm::vec3>& ExposedNormals();
    const GameDefs::DrawableData& DrawableSides() const;
    const std::vector<glm::vec3>& ExposedNormals() const;
    bool HasNormals() const;
    bool IsDrawable() const;
private:
    glm::vec3 m_Position;
    //Used to determine which sides are exposed, thus
    //determining if the cube can be drawn
    std::vector<glm::vec3> m_ExposedNormals;
    //Drawable sides of a cube(max 3 in 3d space obv)
    GameDefs::DrawableData m_DrawableSides;
    GameDefs::BlockType m_BlockType;
    GlCore::BlockStructure m_BlockStructure;
};

