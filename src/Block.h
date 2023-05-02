#pragma once
#include <iostream>
#include <mutex>
#include "GlStructure.h"
#include "Renderer.h"

class Chunk;

class Block
{
public:
    Block(const glm::vec3& position, const Defs::BlockType& bt);
    void UpdateRenderableSides(const glm::vec3& camera_pos);

    const glm::vec3& Position() const;
    const Defs::BlockType& Type() const;

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
    Defs::BlockType m_BlockType;
};

//Version of a block that can be picked up from a player, like in the original Minecraft game
class Drop 
{
public:
    Drop(const glm::vec3& position, Defs::BlockType type);
    Drop(const Drop&) = delete;
    Drop(Drop&& right) noexcept;

    Drop& operator=(Drop&& right) noexcept;

    inline const glm::vec3& Position() const { return m_Position; }
    inline Defs::BlockType Type() const { return m_Type; }

    void Render();
    void Update(Chunk* chunk, float elapsed_time);
    void UpdateModel(float elapsed_time);
private:
    GlCore::State& m_State;

    glm::vec3 m_Position;
    glm::vec3 m_Velocity;
    glm::vec3 m_Acceleration;

    Defs::BlockType m_Type;
    float m_RotationAngle;
    glm::mat4 m_Model;
};
