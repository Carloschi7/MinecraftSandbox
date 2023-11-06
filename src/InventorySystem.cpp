#include "InventorySystem.h"
#include "GlStructure.h"

Inventory::Inventory(TextRenderer& text_renderer) :
    m_State(*GlCore::pstate), m_CursorIndex(0), 
    m_TextRenderer{ text_renderer }
{
    //TextureWater at the moment is the first non-block texture in the game assets
    //so use this as a limit so that each block type occupies a slot in the 
    //inventory
    for (u32 i = 0; i < static_cast<u32>(Defs::TextureBinding::TextureWater); i++)
        m_Slots[i] = { static_cast<Defs::BlockType>(i), 1 };
    
    //Avoid writing glm a thousand times in some of these functions
    using namespace glm;

    m_InternAbsTransf = scale(translate(mat4(1.0f), vec3(960.0f, 460.0f, 0.0f)), vec3(970.0f, 789.0f, 0.0f));
    m_ScreenAbsTransf = scale(translate(mat4(1.0f), vec3(958.0f, 1030.0f, 0.0f)), vec3(1010.0f, 100.0f, 0.0f));

    //Item grids
    m_InternalGrid = Grid(ivec2(530, 510), ivec2(1502, 772), 9, 3);
    m_InternalScreenGrid = Grid(ivec2(530, 810), ivec2(1502, 898), 9, 1);
    m_ScreenGrid = Grid(ivec2(510, 1030), ivec2(1526, 1070), 9, 1);

    crafting_2x2 = Grid(ivec2(912, 214), ivec2(912 + 106 * 2, 214 + 88 * 2), 2, 2);
    crafting_3x3 = Grid(ivec2(692, 174), ivec2(692 + 106 * 3, 174 + 88 * 3), 3, 3);
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
                u32 x_start, y_start, x_size, y_size;
                if (i != 3) {
                    x_start = (m_InternalGrid.measures.entry_position.x - 48.0f) + m_InternalGrid.measures.entry_stride.x * j;
                    y_start = (m_InternalGrid.measures.entry_position.y - 40.0f) + m_InternalGrid.measures.entry_stride.y * i;
                    x_size = m_InternalGrid.measures.entry_stride.x;
                    y_size = m_InternalGrid.measures.entry_stride.y;
                }
                else {
                    x_start = (m_InternalScreenGrid.measures.entry_position.x - 48.0f) + m_InternalScreenGrid.measures.entry_stride.x * j;
                    y_start = (m_InternalScreenGrid.measures.entry_position.y - 40.0f);
                    x_size = m_InternalScreenGrid.measures.entry_stride.x;
                    y_size = m_InternalScreenGrid.measures.entry_stride.y;
                }

                if (dx >= x_start && dy >= y_start && dx < x_start + x_size && dy < y_start + y_size)
                {
                    u32 index = i * 9 + j;
                    if (m_PendingEntry.has_value()) {
                        if (!m_Slots[index].has_value()) {
                            auto& opt = m_PendingEntry->from_crafting_grid ? m_CraftingSlots[m_PendingEntry->index] : m_Slots[m_PendingEntry->index];
                            m_Slots[index] = opt;
                            opt = std::nullopt;
                            m_PendingEntry = std::nullopt;
                            return;
                        }

                        if (!m_PendingEntry->from_crafting_grid && index == m_PendingEntry->index)
                            m_PendingEntry = std::nullopt;
                    } else {
                        if (m_Slots[index].has_value()) {
                            m_PendingEntry = { index, false };
                            return;
                        }
                    }
                }
            }
        }

        //Crafting section

        GridMeasures& measures = view_crafting_table ? crafting_3x3.measures : crafting_2x2.measures;
        u32 lim = view_crafting_table ? 3 : 2;
        for (u32 i = 0; i < lim; i++) {
            for (u32 j = 0; j < lim; j++) {
                u32 x_start = (measures.entry_position.x - 48.0f) + measures.entry_stride.x * j;
                u32 y_start = (measures.entry_position.y - 40.0f) + measures.entry_stride.y * i;
                u32 x_size = measures.entry_stride.x;
                u32 y_size = measures.entry_stride.y;

                if (dx >= x_start && dy >= y_start && dx < x_start + x_size && dy < y_start + y_size)
                {
                    u32 index = i * 3 + j;
                    if (m_PendingEntry.has_value()) {
                        if (!m_CraftingSlots[index].has_value()) {
                            auto& opt = m_PendingEntry->from_crafting_grid ? m_CraftingSlots[m_PendingEntry->index] : m_Slots[m_PendingEntry->index];
                            m_CraftingSlots[index] = opt;
                            opt = std::nullopt;
                            m_PendingEntry = std::nullopt;
                            return;
                        }

                        if (m_PendingEntry->from_crafting_grid && index == m_PendingEntry->index)
                            m_PendingEntry = std::nullopt;
                    }
                    else {
                        if (m_CraftingSlots[index].has_value()) {
                            m_PendingEntry = { index, true };
                            return;
                        }
                    }
                }

            }
        }
    }
}

void Inventory::InternalRender()
{
    glDisable(GL_DEPTH_TEST);

    //Actual inventory rendering
    Defs::TextureBinding texture = view_crafting_table ? Defs::TextureBinding::TextureCraftingTableInventory : Defs::TextureBinding::TextureInventory;
    u32 inventory_binding = static_cast<u32>(texture);
    m_State.game_textures[inventory_binding].Bind(inventory_binding);
    m_State.inventory_shader->Uniform1i(inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, m_InternAbsTransf);

    //Draw entries
    for (u32 i = 0; i < m_Slots.size(); i++) {
        std::optional<InventoryEntry> entry = m_Slots[i];
        if (!entry.has_value())
            continue;

        if (m_PendingEntry.has_value() && !m_PendingEntry->from_crafting_grid && i == m_PendingEntry->index)
            continue;

        RenderEntry(EntryType::Default, entry.value(), i);
    }

    //Draw crafting entries if present
    for (u32 i = 0; i < m_CraftingSlots.size(); i++) {

        //This will need to be removed, because if the player exit the inventory screen the items 
        //left in the inventory greed need to be inserted back into the inventory.
        //TODO this function that can be integrated with the current UnsetPendingEntry, for how that it
        //is called in the main code, can be implemented in the future, for now we leave everithing in the
        //inventory slots without removing them
        if(texture == Defs::TextureBinding::TextureInventory)
            if (i != 0 && i != 1 && i != 3 && i != 4)
                continue;

        std::optional<InventoryEntry> entry = m_CraftingSlots[i];
        if (!entry.has_value())
            continue;

        if (m_PendingEntry.has_value() && m_PendingEntry->from_crafting_grid && i == m_PendingEntry->index)
            continue;

        RenderEntry(view_crafting_table ? EntryType::Crafting_3x3 : EntryType::Crafting_2x2, entry.value(), i);
    }

    //Draw selection
    if (m_PendingEntry.has_value()) {
        if (m_PendingEntry->from_crafting_grid)
            RenderEntry(EntryType::Pending, m_CraftingSlots[m_PendingEntry->index].value(), {}); //{} symbolyzed the value is not needed if Pending
        else
            RenderEntry(EntryType::Pending, m_Slots[m_PendingEntry->index].value(), {});
    }

    glEnable(GL_DEPTH_TEST);
}

//void Inventory::CraftingTableRender()
//{
//    glDisable(GL_DEPTH_TEST);
//
//    //Actual inventory rendering
//    u32 inventory_binding = static_cast<u32>(Defs::TextureBinding::TextureCraftingTableInventory);
//    m_State.game_textures[inventory_binding].Bind(inventory_binding);
//    m_State.inventory_shader->Uniform1i(inventory_binding, "texture_inventory");
//    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, m_InternAbsTransf);
//
//    //Draw entries
//    for (u32 i = 0; i < m_Slots.size(); i++) {
//        std::optional<InventoryEntry> entry = m_Slots[i];
//        if (!entry.has_value())
//            continue;
//
//        if (m_PendingEntry.has_value() && !m_PendingEntry->from_crafting_grid && i == m_PendingEntry->index)
//            continue;
//
//        RenderEntry(entry.value(), i);
//    }
//
//    //Draw crafting entries if present
//    for (u32 i = 0; i < m_CraftingSlots.size(); i++) {
//        std::optional<InventoryEntry> entry = m_CraftingSlots[i];
//        if (!entry.has_value())
//            continue;
//
//        if (m_PendingEntry.has_value() && m_PendingEntry->from_crafting_grid && i == m_PendingEntry->index)
//            continue;
//
//        RenderCraftingEntry(entry.value(), GridType::Grid3x3, i);
//    }
//
//    //Draw selection
//    if (m_PendingEntry.has_value()) {
//        if (m_PendingEntry->from_crafting_grid)
//            RenderPendingEntry(m_CraftingSlots[m_PendingEntry->index].value());
//        else
//            RenderPendingEntry(m_Slots[m_PendingEntry->index].value());
//    }
//
//    glEnable(GL_DEPTH_TEST);
//}

void Inventory::ScreenRender()
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
    Defs::g_InventorySelectedBlock = block_type.has_value() ? block_type->block_type : Defs::BlockType::Dirt;
    

    u32 scr_inventory_binding = static_cast<u32>(Defs::TextureBinding::TextureScreenInventory);
    m_State.game_textures[scr_inventory_binding].Bind(scr_inventory_binding);
    m_State.inventory_shader->Uniform1i(scr_inventory_binding, "texture_inventory");
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, m_ScreenAbsTransf);

    //Draw entities
    for (u32 i = Defs::g_InventoryInternalSlotsCount; i < m_Slots.size(); i++) {
        std::optional<InventoryEntry> entry = m_Slots[i];
        if (!entry.has_value())
            continue;

        RenderEntry(EntryType::Screen, entry.value(), i);
    }

    //Render selector
    u32 selector_binding = static_cast<u32>(Defs::TextureBinding::TextureScreenInventorySelector);
    m_State.game_textures[selector_binding].Bind(selector_binding);
    m_State.inventory_shader->Uniform1i(selector_binding, "texture_inventory");
    auto [screen_slot_transform, _] = SlotScreenTransform(m_CursorIndex, true);
    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_vm, nullptr, screen_slot_transform);

    glEnable(GL_DEPTH_TEST);
}

void Inventory::RenderEntry(EntryType entry_type, InventoryEntry entry, u32 binding_index)
{
    auto draw_string_basic = [this, entry](glm::vec2 num_transform)
    {
        //Drawing block count
        //Number testing, needs to be removed soon
        glEnable(GL_BLEND);
        m_TextRenderer.DrawString(std::to_string(entry.block_count), num_transform);
        glDisable(GL_BLEND);
    };

    //Drawing actual block
    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");
    switch (entry_type) {
    case EntryType::Default: {
            auto [icon_transform, num_transform] = SlotTransform(binding_index, entry.block_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, icon_transform);
            draw_string_basic(num_transform);
        } return;
    case EntryType::Screen: {
            auto [screen_slot_transform, num_transform] = SlotScreenTransform(binding_index - Defs::g_InventoryInternalSlotsCount, entry.block_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);
            draw_string_basic(num_transform);
        } return;
    case EntryType::Crafting_2x2:
    case EntryType::Crafting_3x3: {
            auto [screen_slot_transform, num_transform] = SlotCraftingTransform(entry_type == EntryType::Crafting_3x3, binding_index, entry.block_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);
            draw_string_basic(num_transform);
        } return;
    case EntryType::Pending: {
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
        } return;
    }
}

//void Inventory::RenderScreenEntry(InventoryEntry entry, u32 binding_index)
//{
//    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");
//
//
//    glEnable(GL_BLEND);
//    m_TextRenderer.DrawString(std::to_string(entry.block_count), num_transform);
//    glDisable(GL_BLEND);
//}
//
//void Inventory::RenderCraftingEntry(InventoryEntry entry, GridType grid_type, u32 binding_index)
//{
//    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");
//    auto [screen_slot_transform, num_transform] = SlotCraftingTransform(grid_type, binding_index, entry.block_count >= 10);
//    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);
//
//    glEnable(GL_BLEND);
//    m_TextRenderer.DrawString(std::to_string(entry.block_count), num_transform);
//    glDisable(GL_BLEND);
//}
//
//void Inventory::RenderPendingEntry(InventoryEntry entry)
//{
//    m_State.inventory_shader->Uniform1i(static_cast<u32>(entry.block_type), "texture_inventory");
//
//    Window& wnd = *m_State.game_window;
//    double dx, dy;
//    wnd.GetCursorCoord(dx, dy);
//
//    glm::mat4 pos_mat = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dy, 0.0f));
//    pos_mat = glm::scale(pos_mat, GridMeasures::tile_transform);
//    GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, pos_mat);
//
//    glEnable(GL_BLEND);
//    f32 factor = entry.block_count >= 10 ? pending_double_digit_offset : pending_single_digit_offset;
//    glm::ivec2 number_padding(2, 2);
//    m_TextRenderer.DrawString(std::to_string(entry.block_count), glm::ivec2(dx + factor, dy) + number_padding);
//    glDisable(GL_BLEND);
//}

std::optional<InventoryEntry>& Inventory::HoveredFromSelector()
{
    //Should always be defined when the method is called
    return m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
}

void Inventory::ClearUsedSlots()
{
    for (auto& entry : m_Slots) {
        if (entry.has_value() && entry->block_count == 0)
            entry = std::nullopt;
    }
}

//pardon for the hardcoded section, haven't come up with a more appropriate way of doing this
std::pair<glm::mat4, glm::vec2> Inventory::SlotTransform(u32 slot_index, bool two_digit_number)
{
    using namespace glm;
    if (slot_index >= Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount)
        return { mat4{}, vec2{} };

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
    return { icon_transform, num_ret };
}

std::pair<glm::mat4, glm::vec2> Inventory::SlotScreenTransform(u32 slot_index, bool two_digit_number)
{
    using namespace glm;
    if (slot_index >= Defs::g_InventoryScreenSlotsCount)
        return { mat4{}, vec2{} };

    mat4 ret(1.0f);
    vec2 num_ret(1.0f);
    GridMeasures& ms = m_ScreenGrid.measures;

    u32 two_digit_offset = two_digit_number ? double_digit_offset : single_digit_offset;

    ret = translate(ret, vec3(ms.entry_position, 0.0f) + vec3(ms.entry_stride.x * slot_index, 0.0f, 0.0f));
    num_ret = ms.number_position + ivec2(ms.number_stride.x * slot_index - two_digit_offset, 0.0f);

    mat4 icon_transform = scale(ret, GridMeasures::tile_transform);
    return { icon_transform, num_ret };
}

std::pair<glm::mat4, glm::vec2> Inventory::SlotCraftingTransform(bool grid_3x3, u32 slot_index, bool two_digit_number)
{
    using namespace glm;
    if (slot_index > Defs::g_CraftingSlotsMaxCount)
        return { mat4{}, vec2{} };

    mat4 ret(1.0f);
    vec2 num_ret(1.0f);
    GridMeasures& ms = grid_3x3 ? crafting_3x3.measures : crafting_2x2.measures;
    u32 two_digit_offset = two_digit_number ? double_digit_offset : single_digit_offset;

    vec3 tile_offset = {
        ms.entry_position + ms.entry_stride * ivec2(slot_index % 3, slot_index / 3),
        0.0f
    };

    ret = translate(ret, tile_offset);
    num_ret = ms.number_position + (ms.number_stride * ivec2(slot_index % 3, slot_index / 3)) - ivec2(two_digit_offset, 0);

    mat4 icon_transform = scale(ret, GridMeasures::tile_transform);
    return { icon_transform, num_ret };
}
