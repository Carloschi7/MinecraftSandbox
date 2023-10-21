#pragma once
#include <iostream>
#include <mutex>
#include "GlStructure.h"
#include "Renderer.h"

class Chunk;

class Block
{
public:
    Block(glm::u8vec3 position, const Defs::BlockType& bt);
    void UpdateRenderableSides(const Chunk* parent, const glm::vec3& camera_pos);

    const Defs::BlockType& Type() const;

    const GlCore::DrawableData& DrawableSides() const;
    void AddNormal(const glm::vec3& norm);
    void AddNormal(f32 x, f32 y, f32 z);
    void RemoveNormal(const glm::vec3& norm);
    void RemoveNormal(f32 x, f32 y, f32 z);
    bool HasNormals() const;
    bool IsDrawable() const;

    //Serialization
    void Serialize(const Utils::Serializer& sz);
public:
    static u8 IndexForNormal(const glm::vec3& vec);
    static glm::vec3 NormalForIndex(u32 index);
    //Position relative to the chunk
    glm::u8vec3 position;
    //Used to determine which sides are exposed, thus
    //determining if the cube can be drawn
    //The normal is present if the matching index in
    //the array is one
    u8 exposed_normals;
private:
    //Drawable sides of a cube(max 3 in 3d space obv)
    //Determines which sides can be drawn
    GlCore::DrawableData m_DrawableSides;
    Defs::BlockType m_BlockType;
};

constexpr int i = sizeof(Block);

//Version of a block that can be picked up from a player, like in the original Minecraft game
class Drop 
{
public:
    Drop(const glm::vec3& position, Defs::BlockType type);
    Drop(const Drop&) = delete;
    Drop(Drop&& right) noexcept;

    Drop& operator=(Drop&& right) noexcept;

    inline Defs::BlockType Type() const { return m_Type; }

    void Render();
    void Update(Chunk* chunk, f32 elapsed_time);
    void UpdateModel(f32 elapsed_time);
    glm::vec3 position;
private:
    glm::vec3 m_Velocity;
    glm::vec3 m_Acceleration;

    Defs::BlockType m_Type;
    f32 m_RotationAngle;
    glm::mat4 m_Model;
};
