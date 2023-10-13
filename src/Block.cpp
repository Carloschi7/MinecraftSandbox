#include "Block.h"
#include "Vertices.h"
#include "Chunk.h"

Block::Block(const glm::vec3& position, const Defs::BlockType& bt)
    :m_Position(position), m_BlockType(bt), m_ExposedNormals{}
{
}

void Block::UpdateRenderableSides(const glm::vec3& camera_pos)
{
    u8& counter = m_DrawableSides.second;
    counter = 0;

    //Should never go above 3 elements
    glm::vec3 dir = Position() - camera_pos;
    for (u32 i = 0; i < m_ExposedNormals.size(); i++)
    {
        const bool& is_norm = m_ExposedNormals[i];
        if (is_norm)
        {
            glm::vec3 norm = NormalForIndex(i);
            if (counter < 3 && glm::dot(norm, -dir) > 0.0f)
                m_DrawableSides.first[counter++] = GlCore::GetNormVertexBegin(norm);
        }
    }
}

const glm::vec3& Block::Position() const
{
    return m_Position;
}

const Defs::BlockType& Block::Type() const
{
    return m_BlockType;
}

const GlCore::DrawableData& Block::DrawableSides() const
{
    return m_DrawableSides;
}

void Block::AddNormal(const glm::vec3& norm)
{
    m_ExposedNormals[IndexForNormal(norm)] = true;
}

void Block::AddNormal(f32 x, f32 y, f32 z)
{
    AddNormal(glm::vec3(x, y, z));
}

void Block::RemoveNormal(const glm::vec3& norm)
{
    m_ExposedNormals[IndexForNormal(norm)] = false;
}

void Block::RemoveNormal(f32 x, f32 y, f32 z)
{
    RemoveNormal(glm::vec3(x, y, z));
}

bool Block::HasNormals() const
{
    for (bool b : m_ExposedNormals)
        if (b)
            return true;

    return false;
}

bool Block::IsDrawable() const
{
    return m_DrawableSides.second;
}

void Block::Serialize(const Utils::Serializer& sz, const glm::vec3& base_pos)
{
    auto offset_vec = static_cast<glm::u8vec3>(m_Position - base_pos);
    sz& offset_vec.x& offset_vec.y& offset_vec.z;

    Utils::Bitfield<6> bitfield;
    for (u32 i = 0; i < m_ExposedNormals.size(); i++)
        bitfield.Set(i, m_ExposedNormals[i]);

    sz& bitfield.Getu8Payload(0);
    //m_BlockStructure and m_DrawableData do not need to be serialized

    sz& static_cast<u8>(m_BlockType);
}

u32 Block::IndexForNormal(const glm::vec3& vec)
{
    if (vec == glm::vec3(1.0f, 0.0f, 0.0f))
        return 0;
    if (vec == glm::vec3(-1.0f, 0.0f, 0.0f))
        return 1;
    if (vec == glm::vec3(0.0f, 1.0f, 0.0f))
        return 2;
    if (vec == glm::vec3(0.0f, -1.0f, 0.0f))
        return 3;
    if (vec == glm::vec3(0.0f, 0.0f, 1.0f))
        return 4;
    if (vec == glm::vec3(0.0f, 0.0f, -1.0f))
        return 5;

    return static_cast<u32>(-1);
}

glm::vec3 Block::NormalForIndex(u32 index)
{
    switch (index)
    {
    case 0:
        return glm::vec3(1.0f, 0.0f, 0.0f);
    case 1:
        return glm::vec3(-1.0f, 0.0f, 0.0f);
    case 2:
        return glm::vec3(0.0f, 1.0f, 0.0f);
    case 3:
        return glm::vec3(0.0f, -1.0f, 0.0f);
    case 4:
        return glm::vec3(0.0f, 0.0f, 1.0f);
    case 5:
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }

    return glm::vec3(0.0f);
}

Drop::Drop(const glm::vec3& position, Defs::BlockType type) :
    m_State(GlCore::State::GlobalInstance()),
    m_Type(type), m_Position(position), m_RotationAngle(0.0f), m_Model(1.0f)
{
    GlCore::VertexData vd = GlCore::Cube();
    
    m_Acceleration = { 0.0f, -1.0f, 0.0f };
    m_Velocity = { 0.0f, 0.0f, 0.0f };
}

Drop::Drop(Drop&& right) noexcept :
    m_State(right.m_State)
{
    operator=(std::move(right));
}

Drop& Drop::operator=(Drop&& right) noexcept
{
    m_State = right.m_State;
    m_Position = right.m_Position;
    m_Velocity = right.m_Velocity;
    m_Acceleration = right.m_Acceleration;
    m_Type = right.m_Type;
    m_RotationAngle = right.m_RotationAngle;
    m_Model = right.m_Model;
    return *this;
}

void Drop::Render()
{
    auto shader = m_State.drop_shader;
    shader->Use();
    m_State.drop_vm->BindVertexArray();

    
    shader->UniformMat4f(m_Model, "model");
    //shader->Uniform1i(static_cast<u32>(m_Type), "texture_diffuse");
    shader->Uniform1i(static_cast<u32>(m_Type), "drop_texture_index");
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Drop::Update(Chunk* chunk, f32 elapsed_time)
{
    m_Velocity += m_Acceleration * elapsed_time * 0.5f;
    m_Position += m_Velocity;

    glm::vec3 to_find = glm::vec3(std::roundf(m_Position.x), static_cast<s32>(m_Position.y), std::roundf(m_Position.z));

    const auto& vec = chunk->Blocks();
    auto iter = std::find_if(vec.begin(), vec.end(), [to_find](const Block& b) {return b.Position() == to_find; });
    if (iter != vec.end()) {
        m_Position = { m_Position.x, iter->Position().y + 0.8f, m_Position.z};
        m_Velocity = glm::vec3(0.0f);
    }
}

void Drop::UpdateModel(f32 elapsed_time)
{
    m_Model = glm::translate(glm::mat4(1.0f), m_Position);
    m_Model = glm::rotate(m_Model, m_RotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    m_Model = glm::scale(m_Model, glm::vec3(0.4f));

    m_RotationAngle += elapsed_time * 2.0f;
}
