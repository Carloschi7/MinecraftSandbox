#include "Block.h"
#include "Vertices.h"
#include "Chunk.h"

//For block normal handling
static const std::array<u8, 6> bitshifts = {
    (1 << 0), //pos x
    (1 << 1), //neg x
    (1 << 2), //pos y
    (1 << 3), //neg y
    (1 << 4), //pos z 
    (1 << 5), //neg z
};

Block::Block(glm::u8vec3 position, const Defs::BlockType& bt)
    :position(position), m_BlockType(bt), exposed_normals{}
{
}

void Block::UpdateRenderableSides(const Chunk* parent, const glm::vec3& camera_pos)
{
    u8& counter = m_DrawableSides.second;
    counter = 0;

    //Should never go above 3 elements
    glm::vec3 dir = parent->ToWorld(position) - camera_pos;
    for (u32 i = 0; i < bitshifts.size(); i++)
    {
        const bool is_norm = exposed_normals & bitshifts[i];
        if (is_norm)
        {
            glm::vec3 norm = NormalForIndex(i);
            if (counter < 3 && glm::dot(norm, -dir) > 0.0f)
                m_DrawableSides.first[counter++] = GlCore::GetNormVertexBegin(norm);
        }
    }
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
    exposed_normals |= (1 << IndexForNormal(norm));
}

void Block::AddNormal(f32 x, f32 y, f32 z)
{
    AddNormal(glm::vec3(x, y, z));
}

void Block::RemoveNormal(const glm::vec3& norm)
{
    exposed_normals &= ~(1 << IndexForNormal(norm));
}

void Block::RemoveNormal(f32 x, f32 y, f32 z)
{
    RemoveNormal(glm::vec3(x, y, z));
}

bool Block::HasNormals() const
{
    for (u8 i = 0; i < bitshifts.size(); i++)
        if (exposed_normals & bitshifts[i])
            return true;

    return false;
}

bool Block::IsDrawable() const
{
    return m_DrawableSides.second;
}

void Block::Serialize(const Utils::Serializer& sz)
{
    sz& position.x& position.y& position.z;
    sz& exposed_normals;
    //m_BlockStructure and m_DrawableData do not need to be serialized

    sz& static_cast<u8>(m_BlockType);
}

u8 Block::IndexForNormal(const glm::vec3& vec)
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
    m_Type(type), position(position), m_RotationAngle(0.0f), m_Model(1.0f)
{
    GlCore::VertexData vd = GlCore::Cube();
    
    m_Acceleration = { 0.0f, -1.0f, 0.0f };
    m_Velocity = { 0.0f, 0.0f, 0.0f };
}

Drop::Drop(Drop&& right) noexcept
{
    operator=(std::move(right));
}

Drop& Drop::operator=(Drop&& right) noexcept
{
    position = right.position;
    m_Velocity = right.m_Velocity;
    m_Acceleration = right.m_Acceleration;
    m_Type = right.m_Type;
    m_RotationAngle = right.m_RotationAngle;
    m_Model = right.m_Model;
    return *this;
}

void Drop::Render()
{
    auto drop_shader = GlCore::State::GlobalInstance().drop_shader;
    auto drop_vm = GlCore::State::GlobalInstance().drop_vm;
    drop_shader->Use();
    drop_vm->BindVertexArray();

    drop_shader->UniformMat4f(m_Model, "model");
    //shader->Uniform1i(static_cast<u32>(m_Type), "texture_diffuse");
    drop_shader->Uniform1i(static_cast<u32>(m_Type), "drop_texture_index");
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Drop::Update(Chunk* chunk, f32 elapsed_time)
{
    m_Velocity += m_Acceleration * elapsed_time * 0.5f;
    position += m_Velocity;

    glm::vec3 to_find = glm::vec3(std::roundf(position.x), static_cast<s32>(position.y), std::roundf(position.z));

    const auto& vec = chunk->Blocks();
    auto iter = std::find_if(vec.begin(), vec.end(), [chunk, to_find](const Block& b) {return chunk->ToWorld(b.position) == to_find; });
    if (iter != vec.end()) {
        position = { position.x, iter->position.y + 0.8f, position.z};
        m_Velocity = glm::vec3(0.0f);
    }
}

void Drop::UpdateModel(f32 elapsed_time)
{
    m_Model = glm::translate(glm::mat4(1.0f), position);
    m_Model = glm::rotate(m_Model, m_RotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    m_Model = glm::scale(m_Model, glm::vec3(0.4f));

    m_RotationAngle += elapsed_time * 2.0f;
}
