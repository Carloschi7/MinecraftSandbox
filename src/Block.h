#pragma once
#include <iostream>
#include <mutex>
#include "GlStructure.h"
#include "Renderer.h"

class Block
{
public:
    Block(const glm::vec3& position, const GameDefs::BlockType& bt);

    void Draw(bool bIsBlockSelected = false) const;
    void UpdateRenderableSides(const glm::vec3& camera_pos);

    const glm::vec3& GetPosition() const;

    const GlCore::DrawableData& DrawableSides() const;
    VecType<glm::vec3>& ExposedNormals();
    const VecType<glm::vec3>& ExposedNormals() const;
    bool HasNormals() const;
    bool IsDrawable() const;
private:
    glm::vec3 m_Position;
    //Used to determine which sides are exposed, thus
    //determining if the cube can be drawn
    VecType<glm::vec3> m_ExposedNormals;
    //Drawable sides of a cube(max 3 in 3d space obv)
    GlCore::BlockStructure m_BlockStructure;
    //Determines which sides can be drawn
    GlCore::DrawableData m_DrawableSides;
    GameDefs::BlockType m_BlockType;
};
