#pragma once
#include <iostream>
#include <mutex>
#include "GlStructure.h"
#include "Renderer.h"

class Chunk;

class Block
{
public:
    Block(const glm::vec3& position, const Gd::BlockType& bt);
    void UpdateRenderableSides(const glm::vec3& camera_pos);

    const glm::vec3& Position() const;
    const Gd::BlockType& Type() const;

    const GlCore::DrawableData& DrawableSides() const;
    void AddNormal(const glm::vec3& norm);
    void AddNormal(float x, float y, float z);
    void RemoveNormal(const glm::vec3& norm);
    void RemoveNormal(float x, float y, float z);
    bool HasNormals() const;
    bool IsDrawable() const;

    //Serialization
    void Serialize(const Utils::Serializer& sz, const glm::vec3& base_pos);
public:
    static uint32_t IndexForNormal(const glm::vec3& vec);
    static glm::vec3 NormalForIndex(uint32_t index);
private:
    glm::vec3 m_Position;
    //Used to determine which sides are exposed, thus
    //determining if the cube can be drawn
    //The normal is present if the matching index in
    //the array is one
    std::array<bool, 6> m_ExposedNormals;
    //Drawable sides of a cube(max 3 in 3d space obv)
    //Determines which sides can be drawn
    GlCore::DrawableData m_DrawableSides;
    Gd::BlockType m_BlockType;
};
