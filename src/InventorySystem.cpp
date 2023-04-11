#include "InventorySystem.h"
#include "GlStructure.h"

Inventory::Inventory() :
    m_State(GlCore::State::GetState())
{
    for (uint32_t i = 0; i < 5; i++)
    {
        m_InternalSlots[i] = static_cast<Defs::BlockType>(i);
    }
    m_InternalSlots[12+9] = Defs::BlockType::Grass;

    //Raw values from inventory dimension, a very simple way that is used only beacause the
    //inventory is not gonna change dimensions for the time being
    m_InventoryStart = { 523, 542 };
    m_IntervalDimension = { 97, 74 };

    m_InternAbsTransf = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.25f, 0.0f));
    m_ScreenAbsTransf = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.91f, 0.0f)), glm::vec3(1.0f, 0.15f, 0.0f));
}

void Inventory::HandleInventorySelection()
{


    Window& wnd = GlCore::State::GetState().GameWindow();
    double dx, dy;
    wnd.GetCursorCoord(dx, dy);

    bool mouse_left_state = wnd.IsMouseEvent({ GLFW_MOUSE_BUTTON_1, GLFW_PRESS });

    //Scan inventory
    if (mouse_left_state)
    {
        for (uint32_t i = 0; i < 3; i++)
        {
            for (uint32_t j = 0; j < 9; j++)
            {
                uint32_t x_start = m_InventoryStart.x + m_IntervalDimension.x * j;
                uint32_t y_start = m_InventoryStart.y + m_IntervalDimension.y * i;
                if (dx >= x_start && dy >= y_start && dx < x_start + m_IntervalDimension.x && dy < y_start + m_IntervalDimension.y)
                {
                    uint32_t index = i * 9 + j;
                    if (m_InternalSlots[index].has_value()) {
                        Defs::g_InventorySelectedBlock = m_InternalSlots[index].value();
                        return;
                    }
                }
            }
        }
    }
}

void Inventory::InternalSideRender()
{
    auto entry_rendering = [this](Defs::TextureBinding binding, uint32_t binding_index) {
        //Draw elements
        uint32_t grass_binding = static_cast<uint32_t>(binding);
        m_State.InventoryShader()->Uniform1i(grass_binding, "texture_inventory");
        glm::mat4 pos_mat = InternalSlotAbsoluteTransform(binding_index);
        GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryEntryVM(), nullptr, pos_mat);
    };

    for (uint32_t i = 0; i < m_InternalSlots.size(); i++)
    {
        std::optional<Defs::BlockType> entry = m_InternalSlots[i];
        if (!entry.has_value())
            continue;

        entry_rendering(static_cast<Defs::TextureBinding>(entry.value()), i);
    }

    //Draw currently selected
    m_State.InventoryShader()->Uniform1i(static_cast<uint32_t>(Defs::g_InventorySelectedBlock), "texture_inventory");
    glm::mat4 pos_mat = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.21f, 0.3f, 0.0f)), glm::vec3(0.2f, 0.2f, 0.0f));
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryEntryVM(), nullptr, pos_mat);

    //Actual inventory rendering
    uint32_t inventory_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureInventory);
    GlCore::GameTextures()[inventory_binding].Bind(inventory_binding);
    m_State.InventoryShader()->Uniform1i(inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryVM(), nullptr, m_InternAbsTransf);
}

void Inventory::ScreenSideRender()
{
    uint32_t scr_inventory_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureScreenInventory);
    GlCore::GameTextures()[scr_inventory_binding].Bind(scr_inventory_binding);
    m_State.InventoryShader()->Uniform1i(scr_inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryVM(), nullptr, m_ScreenAbsTransf);
}

//Returns the mapped matrix for a specific inventory slot in the inventory view
glm::mat4 Inventory::InternalSlotAbsoluteTransform(uint32_t slot_index)
{
    glm::mat4 ret(1.0f);
    uint32_t div = slot_index / 9;
    ret = glm::translate(ret, glm::vec3(-0.41f + 0.1019f * (slot_index % 9), -0.066f - 0.1352f * div, 0.0f));
    return glm::scale(ret, glm::vec3(0.09f, 0.122f, 0.0f));
}

glm::mat4 Inventory::ScreenSlotAbsoluteTransform(uint32_t slot_index)
{
    return glm::mat4(1.0f);
}
