#include "InventorySystem.h"
#include "GlStructure.h"

Inventory::Inventory() :
    m_State(GlCore::State::GetState()), m_CursorIndex(0), 
    m_TextRenderer(static_cast<uint32_t>(Defs::TextureBinding::TextureText))
{
    for (uint32_t i = 0; i < 5; i++)
    {
        m_Slots[i] = { static_cast<Defs::BlockType>(i), 1 };
    }
    m_Slots[35] = { Defs::BlockType::Grass, 1 };

    //Raw values from inventory dimension, a very simple way that is used only beacause the
    //inventory is not gonna change dimensions for the time being
    m_InternalStart = { 523, 542 };
    m_ScreenStart = { 523, 780 };
    m_IntervalDimension = { 97, 74 };

    m_InternAbsTransf = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.25f, 0.0f));
    m_ScreenAbsTransf = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.91f, 0.0f)), glm::vec3(1.0f, 0.15f, 0.0f));
}

void Inventory::AddToNewSlot(Defs::BlockType type)
{
    //check for a stacking slot first
    for (std::optional<InventoryEntry>& slot : m_Slots) {
        if (slot.has_value() && slot->block_type == type && slot->block_count < 9) {
            slot->block_count++;
            return;
        }
    }

    //Try to push it to the hovered selection first
    if (auto& opt = m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex]; !opt.has_value()) {
        opt = { type, 1 };
        return;
    }

    //Else use one free slot starting from the inventory beginning
    for (std::optional<InventoryEntry>& slot : m_Slots) {
        if (!slot.has_value()) {
            slot = {type, 1};
            break;
        }
    }
}

void Inventory::HandleInventorySelection()
{
    Window& wnd = GlCore::State::GetState().GameWindow();
    double dx, dy;
    wnd.GetCursorCoord(dx, dy);
    bool mouse_left_state = wnd.IsKeyPressed(GLFW_MOUSE_BUTTON_1);

    //Scan inventory
    if (mouse_left_state)
    {
        for (uint32_t i = 0; i < 4; i++)
        {
            for (uint32_t j = 0; j < 9; j++)
            {
                uint32_t x_start, y_start;

                if (i != 4) {
                    x_start = m_InternalStart.x + m_IntervalDimension.x * j;
                    y_start = m_InternalStart.y + m_IntervalDimension.y * i;
                }
                else {
                    x_start = m_ScreenStart.x + m_IntervalDimension.x * j;
                    y_start = m_ScreenStart.y;
                }

                
                if (dx >= x_start && dy >= y_start && dx < x_start + m_IntervalDimension.x && dy < y_start + m_IntervalDimension.y)
                {
                    uint32_t index = i * 9 + j;
                    if (m_PendingEntry.has_value()) {
                        if (!m_Slots[index].has_value()) {
                            m_Slots[index] = m_Slots[m_PendingEntry.value()];
                            m_Slots[m_PendingEntry.value()] = std::nullopt;
                            m_PendingEntry = std::nullopt;
                            return;
                        }

                        if (index == m_PendingEntry.value())
                            m_PendingEntry = std::nullopt;
                    } else {
                        if (m_Slots[index].has_value()) {
                            m_PendingEntry = index;
                            return;
                        }
                    }
                }
            }
        }
    }
}

void Inventory::InternalSideRender()
{
    glDisable(GL_DEPTH_TEST);

    //Actual inventory rendering
    uint32_t inventory_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureInventory);
    GlCore::GameTextures()[inventory_binding].Bind(inventory_binding);
    m_State.InventoryShader()->Uniform1i(inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryVM(), nullptr, m_InternAbsTransf);

    //Draw entries
    for (uint32_t i = 0; i < m_Slots.size(); i++) {
        std::optional<InventoryEntry> entry = m_Slots[i];
        if (!entry.has_value())
            continue;

        if (m_PendingEntry.has_value() && i == m_PendingEntry.value())
            continue;

        RenderEntry(entry.value(), i);
    }

    //Draw selection
    if (m_PendingEntry.has_value()) {
        InventoryEntry entry = m_Slots[m_PendingEntry.value()].value();
        RenderPendingEntry(static_cast<Defs::TextureBinding>(entry.block_type));
    }

    glEnable(GL_DEPTH_TEST);
}

void Inventory::ScreenSideRender()
{
    glDisable(GL_DEPTH_TEST);

    //Move cursor if necessary
    Window& wnd = GlCore::State::GetState().GameWindow();
    if (wnd.IsMouseWheelUp() && m_CursorIndex < Defs::g_InventoryScreenSlotsCount - 1)
        m_CursorIndex = (m_CursorIndex + 1) % 9;
    if (wnd.IsMouseWheelDown() && m_CursorIndex > 0)
        m_CursorIndex = (m_CursorIndex - 1) % 9;

    //Assign the selection to the right variable
    std::optional<InventoryEntry> block_type = m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
    Defs::g_InventorySelectedBlock = block_type.has_value() ? block_type.value().block_type : Defs::BlockType::Dirt;
    

    uint32_t scr_inventory_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureScreenInventory);
    GlCore::GameTextures()[scr_inventory_binding].Bind(scr_inventory_binding);
    m_State.InventoryShader()->Uniform1i(scr_inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryVM(), nullptr, m_ScreenAbsTransf);

    //Draw entities
    for (uint32_t i = Defs::g_InventoryInternalSlotsCount; i < m_Slots.size(); i++) {
        std::optional<InventoryEntry> entry = m_Slots[i];
        if (!entry.has_value())
            continue;

        if (m_PendingEntry.has_value() && i == m_PendingEntry.value())
            continue;

        RenderScreenEntry(entry.value(), i);
    }

    //Render selector
    uint32_t selector_binding = static_cast<uint32_t>(Defs::TextureBinding::TextureScreenInventorySelector);
    GlCore::GameTextures()[selector_binding].Bind(selector_binding);
    m_State.InventoryShader()->Uniform1i(selector_binding, "texture_inventory");
    auto [screen_slot_transform, _] = SlotScreenTransform(m_CursorIndex);
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryVM(), nullptr, screen_slot_transform);

    glEnable(GL_DEPTH_TEST);
}

void Inventory::RenderEntry(InventoryEntry entry, uint32_t binding_index)
{
    //Drawing actual block
    m_State.InventoryShader()->Uniform1i(static_cast<uint32_t>(entry.block_type), "texture_inventory");
    auto[icon_transform, num_transform] = SlotTransform(binding_index);
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryEntryVM(), nullptr, icon_transform);

    //Drawing block count
    //Number testing, needs to be removed soon
    glEnable(GL_BLEND);
    m_TextRenderer.PrintString(std::to_string(entry.block_count), num_transform);
    glDisable(GL_BLEND);

}

void Inventory::RenderScreenEntry(InventoryEntry entry, uint32_t binding_index)
{
    m_State.InventoryShader()->Uniform1i(static_cast<uint32_t>(entry.block_type), "texture_inventory");
    auto [screen_slot_transform, num_transform] = SlotScreenTransform(binding_index - Defs::g_InventoryInternalSlotsCount);
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryEntryVM(), nullptr, screen_slot_transform);

    glEnable(GL_BLEND);
    m_TextRenderer.PrintString(std::to_string(entry.block_count), num_transform);
    glDisable(GL_BLEND);
}

void Inventory::RenderPendingEntry(Defs::TextureBinding binding)
{
    m_State.InventoryShader()->Uniform1i(static_cast<uint32_t>(binding), "texture_inventory");

    Window& wnd = GlCore::State::GetState().GameWindow();
    double dx, dy, cx, cy;
    wnd.GetCursorCoord(dx, dy);

    //Convert coordinates to screen space
    cx = (dx / 1920.0) * 2.0 - 1.0;
    cy = (dy / 1080.0) * 2.0 - 1.0;

    glm::mat4 pos_mat = glm::translate(glm::mat4(1.0f), glm::vec3(cx, -cy, 0.0f));
    pos_mat = glm::scale(pos_mat, glm::vec3(0.09f, 0.122f, 0.0f));
    GlCore::Renderer::Render(m_State.InventoryShader(), *m_State.InventoryEntryVM(), nullptr, pos_mat);
}

std::optional<InventoryEntry>& Inventory::HoveredFromSelector()
{
    //Should always be defined when the method is called
    return m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
}

void Inventory::ClearUsedSlots()
{
    for (auto& entry : m_Slots) {
        if (entry.has_value() && entry.value().block_count == 0)
            entry = std::nullopt;
    }
}

//pardon for the hardcoded section, haven't come up with a more appropriate way of doing this
std::pair<glm::mat4, glm::mat4> Inventory::SlotTransform(uint32_t slot_index)
{
    if (slot_index >= Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount)
        return std::make_pair(glm::mat4(), glm::mat4());

    glm::mat4 ret(1.0f);
    glm::mat4 num_ret(1.0f);
    if (slot_index < Defs::g_InventoryInternalSlotsCount) {
        uint32_t div = slot_index / 9;
        ret = glm::translate(ret, glm::vec3(-0.41f + 0.1019f * (slot_index % 9), -0.066f - 0.1352f * div, 0.0f));
        num_ret = glm::translate(num_ret, glm::vec3(-0.385f + 0.1019f * (slot_index % 9), -0.105f - 0.1352f * div, 0.0f));
    }
    else {
        ret = glm::translate(ret, glm::vec3(-0.41f + 0.1019f * (slot_index % 9), -0.505f, 0.0f));
        num_ret = glm::translate(num_ret, glm::vec3(-0.385f + 0.1019f * (slot_index % 9), -0.545f, 0.0f));
    }
    
    glm::mat4 icon_transform = glm::scale(ret, glm::vec3(0.09f, 0.122f, 0.0f));
    glm::mat4 num_transform = glm::scale(num_ret, glm::vec3(0.04f));
    return std::make_pair(icon_transform, num_transform);
}

std::pair<glm::mat4, glm::mat4> Inventory::SlotScreenTransform(uint32_t slot_index)
{
    if (slot_index >= Defs::g_InventoryScreenSlotsCount)
        return std::make_pair(glm::mat4(), glm::mat4());

    glm::mat4 ret(1.0f);
    glm::mat4 num_ret(1.0f);
    ret = glm::translate(ret, glm::vec3(-0.444f + 0.111f * slot_index, -0.91f, 0.0f));
    num_ret = glm::translate(num_ret, glm::vec3(-0.42f + 0.111f * slot_index, -0.945f, 0.0f));
    
    glm::mat4 icon_transform = glm::scale(ret, glm::vec3(0.09f, 0.122f, 0.0f));
    glm::mat4 num_transform = glm::scale(num_ret, glm::vec3(0.04f));
    return std::make_pair(icon_transform, num_transform);
}
