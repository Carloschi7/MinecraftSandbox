#include "InventorySystem.h"
#include "GlStructure.h"

Inventory::Inventory() :
    m_State(*GlCore::pstate), m_CursorIndex(0), 
    m_TextRenderer({m_State.game_window->Width(), m_State.game_window->Height()},
        static_cast<u32>(Defs::TextureBinding::TextureText))
{
    for (u32 i = 0; i < 5; i++)
        m_Slots[i] = { static_cast<Defs::BlockType>(i), 1 };
    
    //Avoid writing glm a thousand times in some of these functions
    using namespace glm;
    //TODO compute these from the grid calculations in some way
    m_InternalStart = { 482, 470 };
    m_ScreenStart = { 523, 780 };
    m_IntervalDimension = { 588 - 482, 558 - 470 };

    m_InternAbsTransf = scale(translate(mat4(1.0f), vec3(960.0f, 468.0f, 0.0f)), vec3(1049.0f, 800.0f, 0.0f));
    m_ScreenAbsTransf = scale(translate(mat4(1.0f), vec3(958.0f, 1030.0f, 0.0f)), vec3(1010.0f, 100.0f, 0.0f));

    //Item grids
    m_InternalGrid = Grid(ivec2(530, 510), ivec2(1500, 772), 9, 3);
    m_InternalScreenGrid = Grid(ivec2(530, 790), ivec2(1500, 890), 9, 1);
    m_ScreenGrid = Grid(ivec2(510, 1030), ivec2(1526, 1070), 9, 1);
}

void Inventory::AddToNewSlot(Defs::BlockType type)
{
    //check for a stacking slot first
    for (std::optional<InventoryEntry>& slot : m_Slots) {
        if (slot.has_value() && slot->block_type == type && slot->block_count < s_MaxItemsPerSlot) {
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
    Window& wnd = *m_State.game_window;
    double dx, dy;
    wnd.GetCursorCoord(dx, dy);
    bool mouse_left_state = wnd.IsKeyPressed(GLFW_MOUSE_BUTTON_1);

    //Scan inventory
    if (mouse_left_state)
    {
        for (u32 i = 0; i < 4; i++)
        {
            for (u32 j = 0; j < 9; j++)
            {
                u32 x_start, y_start;

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
                    u32 index = i * 9 + j;
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
    u32 inventory_binding = static_cast<u32>(Defs::TextureBinding::TextureInventory);
    m_State.game_textures[inventory_binding].Bind(inventory_binding);
    m_State.inventory_shader->Uniform1i(inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, m_InternAbsTransf);

    //Draw entries
    for (u32 i = 0; i < m_Slots.size(); i++) {
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
        RenderPendingEntry(entry);
    }

    glEnable(GL_DEPTH_TEST);
}

void Inventory::ScreenSideRender()
{
    glDisable(GL_DEPTH_TEST);

    //Move cursor if necessary
    Window& wnd = *m_State.game_window;
    if (wnd.IsMouseWheelUp() && m_CursorIndex < Defs::g_InventoryScreenSlotsCount - 1)
        m_CursorIndex = (m_CursorIndex + 1) % 9;
    if (wnd.IsMouseWheelDown() && m_CursorIndex > 0)
        m_CursorIndex = (m_CursorIndex - 1) % 9;

    //Assign the selection to the right variable
    std::optional<InventoryEntry> block_type = m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
    Defs::g_InventorySelectedBlock = block_type.has_value() ? block_type.value().block_type : Defs::BlockType::Dirt;
    

    u32 scr_inventory_binding = static_cast<u32>(Defs::TextureBinding::TextureScreenInventory);
    m_State.game_textures[scr_inventory_binding].Bind(scr_inventory_binding);
    m_State.inventory_shader->Uniform1i(scr_inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, m_ScreenAbsTransf);

    //Draw entities
    for (u32 i = Defs::g_InventoryInternalSlotsCount; i < m_Slots.size(); i++) {
        std::optional<InventoryEntry> entry = m_Slots[i];
        if (!entry.has_value())
            continue;

        if (m_PendingEntry.has_value() && i == m_PendingEntry.value())
            continue;

        RenderScreenEntry(entry.value(), i);
    }

    //Render selector
    u32 selector_binding = static_cast<u32>(Defs::TextureBinding::TextureScreenInventorySelector);
    m_State.game_textures[selector_binding].Bind(selector_binding);
    m_State.inventory_shader->Uniform1i(selector_binding, "texture_inventory");
    auto [screen_slot_transform, _] = SlotScreenTransform(m_CursorIndex, true);
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, screen_slot_transform);

    glEnable(GL_DEPTH_TEST);
}

void Inventory::RenderEntry(InventoryEntry entry, u32 binding_index)
{
    //Drawing actual block
    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");
    auto[icon_transform, num_transform] = SlotTransform(binding_index, entry.block_count >= 10);
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, icon_transform);

    //Drawing block count
    //Number testing, needs to be removed soon
    glEnable(GL_BLEND);
    m_TextRenderer.DrawString(std::to_string(entry.block_count), num_transform);
    glDisable(GL_BLEND);

}

void Inventory::RenderScreenEntry(InventoryEntry entry, u32 binding_index)
{
    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");
    auto [screen_slot_transform, num_transform] = SlotScreenTransform(binding_index - Defs::g_InventoryInternalSlotsCount, entry.block_count >= 10);
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);

    glEnable(GL_BLEND);
    m_TextRenderer.DrawString(std::to_string(entry.block_count), num_transform);
    glDisable(GL_BLEND);
}

void Inventory::RenderPendingEntry(InventoryEntry entry)
{
    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");

    Window& wnd = *m_State.game_window;
    double dx, dy;
    wnd.GetCursorCoord(dx, dy);

    glm::mat4 pos_mat = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dy, 0.0f));
    pos_mat = glm::scale(pos_mat, GridMeasures::tile_transform);
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, pos_mat);

    glEnable(GL_BLEND);
    f32 factor = entry.block_count >= 10 ? pending_double_digit_offset : pending_single_digit_offset;
    glm::ivec2 number_padding(2, 2);
    m_TextRenderer.DrawString(std::to_string(entry.block_count), glm::ivec2(dx + factor, dy) + number_padding);
    glDisable(GL_BLEND);
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
std::pair<glm::mat4, glm::vec2> Inventory::SlotTransform(u32 slot_index, bool two_digit_number)
{
    using namespace glm;

    if (slot_index >= Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount)
        return std::make_pair(mat4(), vec2());

    mat4 ret(1.0f);
    vec2 num_ret(1.0f);
    
    //Eventual offset for two digit numbers
    s32 two_digit_offset = two_digit_number ? double_digit_offset : single_digit_offset;

    //Handle separately the inventory slots and the hand slots
    if (slot_index < Defs::g_InventoryInternalSlotsCount) {
        GridMeasures& ms = m_InternalGrid.measures;
        vec3 tile_offset = {
            ms.entry_position + ms.entry_stride * ivec2(slot_index % 9, slot_index / 9), 
            0.0f 
        };
        
        ret = translate(ret, tile_offset);
        num_ret = ms.number_position + (ms.number_stride * ivec2(slot_index % 9, slot_index / 9)) - ivec2(two_digit_offset, 0);
    }
    else {
        GridMeasures& ms = m_InternalScreenGrid.measures;
        ret = translate(ret, vec3(ms.entry_position, 0.0f) + vec3(ms.entry_stride.x * (slot_index % 9), 0.0f, 0.0f));
        num_ret = ms.number_position + ivec2(ms.number_stride.x * (slot_index % 9) - two_digit_offset, 0);
    }
    
    mat4 icon_transform = scale(ret, GridMeasures::tile_transform);
    return std::make_pair(icon_transform, num_ret);
}

std::pair<glm::mat4, glm::vec2> Inventory::SlotScreenTransform(u32 slot_index, bool two_digit_number)
{
    if (slot_index >= Defs::g_InventoryScreenSlotsCount)
        return std::make_pair(glm::mat4(), glm::vec2());

    glm::mat4 ret(1.0f);
    glm::vec2 num_ret(1.0f);
    GridMeasures& ms = m_ScreenGrid.measures;

    u32 two_digit_offset = two_digit_number ? double_digit_offset : single_digit_offset;

    ret = glm::translate(ret, glm::vec3(ms.entry_position, 0.0f) + glm::vec3(ms.entry_stride.x * slot_index, 0.0f, 0.0f));
    num_ret = ms.number_position + glm::ivec2(ms.number_stride.x * slot_index - two_digit_offset, 0.0f);

    glm::mat4 icon_transform = glm::scale(ret, GridMeasures::tile_transform);
    return std::make_pair(icon_transform, num_ret);
}
