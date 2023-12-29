#include "InventorySystem.h"
#include "GlStructure.h"
#include "Renderer.h"

Inventory::Inventory(TextRenderer& text_renderer) :
    m_State(*GlCore::pstate), m_CursorIndex(0), 
    m_TextRenderer{ text_renderer }
{
#ifndef MC_STANDALONE
    //This is left to debug 2d decals items, but this is not left in the standalone version
    m_Slots[27] = { static_cast<Defs::Item>(Defs::Item::WoodPickaxe), 1 };
#endif
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
    product_grid = Grid(ivec2(1304, 263), ivec2(1304 + 106, 263 + 88), 1, 1);

    //Game related stuff
    LoadRecipes(recipes_2x2, recipes_3x3);
}

void Inventory::AddToNewSlot(Defs::Item type)
{
    //check for a stacking slot first
    for (std::optional<InventoryEntry>& slot : m_Slots) {
        if (slot.has_value() && slot->item_type == type && slot->item_count < s_MaxItemsPerSlot) {
            slot->item_count++;
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
    bool mouse_right_state = wnd.IsKeyPressed(GLFW_MOUSE_BUTTON_2);

    //Scan inventory
    if (mouse_left_state || mouse_right_state)
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
                    auto& slot = m_Slots[index];
                    if (m_PendingEntry.has_value()) {
                        if (!slot.has_value()) {
                            slot = m_PendingEntry;
                            m_PendingEntry = std::nullopt;
                            return;
                        }
                        if (slot.has_value() && slot->CanBeInserted(m_PendingEntry.value())) {
                            slot->item_count += m_PendingEntry->item_count;
                            m_PendingEntry = std::nullopt;
                            return;
                        }

                    } else {
                        //Grab a single block unit from that entry or the whole stack if right clicking
                        if (slot.has_value()) {
                            MC_ASSERT(slot->item_count != 0, "this entry should have been deleted previously");
                            if (mouse_left_state) {
                                slot->item_count--;
                                m_PendingEntry = { slot->item_type, 1 };
                                ClearUsedSlots();
                            }
                            else if(mouse_right_state) {
                                m_PendingEntry = slot;
                                slot = std::nullopt;
                            }
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
                    auto& slot = m_CraftingSlots[index];
                    if (m_PendingEntry.has_value()) {
                        if (!slot.has_value()) {
                            slot = m_PendingEntry;
                            //Find if the new configuration matches a recipe
                            ProcessRecipes();
                            m_PendingEntry = std::nullopt;
                            return;
                        }
                        if (slot.has_value() && slot->CanBeInserted(m_PendingEntry.value())) {
                            slot->item_count += m_PendingEntry->item_count;
                            ProcessRecipes();
                            m_PendingEntry = std::nullopt;
                            return;
                        }
                    }
                    else {
                        if (slot.has_value()) {
                            MC_ASSERT(slot->item_count != 0, "this entry should have been deleted previously");
                            if (mouse_left_state) {
                                slot->item_count--;
                                m_PendingEntry = { slot->item_type, 1 };
                                ClearUsedSlots();
                            }
                            else if (mouse_right_state) {
                                m_PendingEntry = slot;
                                slot = std::nullopt;
                            }

                            ProcessRecipes();
                            return;
                        }
                    }
                }

            }
        }

        //Handle crafting product
        if (m_ProductEntry.has_value() && !m_PendingEntry.has_value()) {
            GridMeasures& measures = product_grid.measures;
            u32 x_start = (measures.entry_position.x - 48.0f);
            u32 y_start = (measures.entry_position.y - 40.0f);
            u32 x_size = measures.entry_stride.x;
            u32 y_size = measures.entry_stride.y;
            if (dx >= x_start && dy >= y_start && dx < x_start + x_size && dy < y_start + y_size) {
                for (auto& slot : m_CraftingSlots)
                    if (slot.has_value())
                        slot->item_count--;

                ClearUsedSlots();
                m_PendingEntry = std::exchange(m_ProductEntry, std::nullopt);
                ProcessRecipes();
                return;
            }
        }
    }
}

void Inventory::ProcessRecipes()
{
    m_ProductEntry = std::nullopt;
    //Check for recipes only when we insert a new item in the crafting table section
    bool set_occurrency = false;
    if (m_PendingEntry.has_value()) {
        if (view_crafting_table) {
            for (auto& recipe : recipes_3x3) {
                if (recipe.Matches(m_CraftingSlots)) {
                    m_ProductEntry = recipe.product;
                }
            }
        }

        for (auto& recipe : recipes_2x2) {
            if (recipe.Matches(m_CraftingSlots)) {
                m_ProductEntry = recipe.product;
            }
        }
    }
}

void Inventory::InternalRender()
{
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

        RenderEntry(EntryType::Default, entry.value(), i);
    }

    //Draw crafting entries if present
    for (u32 i = 0; i < m_CraftingSlots.size(); i++) {
        if(texture == Defs::TextureBinding::TextureInventory)
            if (i != 0 && i != 1 && i != 3 && i != 4)
                continue;

        std::optional<InventoryEntry> entry = m_CraftingSlots[i];
        if (!entry.has_value())
            continue;

        RenderEntry(view_crafting_table ? EntryType::Crafting_3x3 : EntryType::Crafting_2x2, entry.value(), i);
    }


    if (m_ProductEntry.has_value())
        RenderEntry(EntryType::CraftingProduct, m_ProductEntry.value(), {});

    //Draw selection
    if (m_PendingEntry.has_value()) 
        RenderEntry(EntryType::Pending, m_PendingEntry.value(), {});
}

void Inventory::ScreenRender()
{
    //Move cursor if necessary
    Window& wnd = *m_State.game_window;
    if (wnd.IsMouseWheelUp() && m_CursorIndex < Defs::g_InventoryScreenSlotsCount - 1)
        m_CursorIndex = (m_CursorIndex + 1) % 9;
    if (wnd.IsMouseWheelDown() && m_CursorIndex > 0)
        m_CursorIndex = (m_CursorIndex - 1) % 9;

    //Assign the selection to the right variable
    std::optional<InventoryEntry> item_type = m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
    Defs::g_InventorySelectedBlock = item_type.has_value() ? item_type->item_type : Defs::Item::Dirt;
    

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
}

void Inventory::RenderEntry(EntryType entry_type, InventoryEntry entry, u32 binding_index)
{
    auto draw_string_basic = [this, entry](glm::vec2 num_transform)
    {
        //Drawing block count
        //Number testing, needs to be removed soon
        glEnable(GL_BLEND);
        m_TextRenderer.DrawString(std::to_string(entry.item_count), num_transform);
        glDisable(GL_BLEND);

        //Reset the optional index uniform
        m_State.inventory_shader->Uniform1i(-1, "optional_texture_index");
    };

    //Drawing actual block
    m_State.inventory_shader->Uniform1i(static_cast<s32>(Defs::TextureBinding::GlobalTexture), "texture_inventory");
    m_State.inventory_shader->Uniform1i(static_cast<s32>(entry.item_type), "optional_texture_index");
    switch (entry_type) {
    case EntryType::Default: {
            auto [icon_transform, num_transform] = SlotTransform(binding_index, entry.item_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, icon_transform);
            draw_string_basic(num_transform);
        } return;
    case EntryType::Screen: {
            auto [screen_slot_transform, num_transform] = SlotScreenTransform(binding_index - Defs::g_InventoryInternalSlotsCount, entry.item_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);
            draw_string_basic(num_transform);
        } return;
    case EntryType::Crafting_2x2:
    case EntryType::Crafting_3x3: {
            auto [screen_slot_transform, num_transform] = SlotCraftingTransform(entry_type == EntryType::Crafting_3x3, binding_index, entry.item_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);
            draw_string_basic(num_transform);
        } return;

    case EntryType::CraftingProduct: {
            auto [screen_slot_transform, num_transform] = SlotCraftingProductTransform(entry.item_count >= 10);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, screen_slot_transform);
            draw_string_basic(num_transform);
        } return;
    case EntryType::Pending: {
            Window& wnd = *m_State.game_window;
            double dx, dy;
            wnd.GetCursorCoord(dx, dy);

            glm::mat4 pos_mat = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dy, 0.0f));
            pos_mat = glm::scale(pos_mat, GridMeasures::tile_transform);
            GlCore::Renderer::Render(m_State.inventory_shader, *m_State.inventory_entry_vm, nullptr, pos_mat);

            glEnable(GL_BLEND);
            f32 factor = entry.item_count >= 10 ? pending_double_digit_offset : pending_single_digit_offset;
            glm::ivec2 number_padding(2, 2);
            m_TextRenderer.DrawString(std::to_string(entry.item_count), glm::ivec2(dx + factor, dy) + number_padding);
            glDisable(GL_BLEND);

            m_State.inventory_shader->Uniform1i(-1, "optional_texture_index");
        } return;
    }
}

std::optional<InventoryEntry>& Inventory::HoveredFromSelector()
{
    //Should always be defined when the method is called
    return m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
}

const std::optional<InventoryEntry>& Inventory::HoveredFromSelector() const
{
    return m_Slots[Defs::g_InventoryInternalSlotsCount + m_CursorIndex];
}

void Inventory::ClearUsedSlots()
{
    for (auto& entry : m_Slots) {
        if (entry.has_value() && entry->item_count == 0)
            entry = std::nullopt;
    }

    for (auto& entry : m_CraftingSlots) {
        if (entry.has_value() && entry->item_count == 0)
            entry = std::nullopt;
    }
}

void Inventory::ViewCleanup()
{
    //Empty pending entry
    if (m_PendingEntry.has_value()) {
        for (u8 k = 0; k < s_InventorySize; k++) {
            if (PerformCleanup(m_Slots[k], m_PendingEntry))
                break;
        }
    }

#ifdef _DEBUG
    MC_ASSERT(m_PendingEntry == std::nullopt, "this should have been cleared, wasn't the original slot found?");
#else
    //If for some reason the current pending entry was not backed in the inventory(should never happen tecnically), just simply erase it
    m_PendingEntry = std::nullopt;
#endif

    //Empty inventory slots
    u8 lim = view_crafting_table ? 3 : 2;
    for (u8 i = 0; i < lim; i++) {
        for (u8 j = 0; j < lim; j++) {
            u8 index = i * 3 + j;

            if (m_CraftingSlots[index].has_value()) {
                for (u8 k = 0; k < s_InventorySize; k++) {
                    if (PerformCleanup(m_Slots[k], m_CraftingSlots[index]))
                        break;
                }
            }
        }
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

std::pair<glm::mat4, glm::vec2> Inventory::SlotCraftingProductTransform(bool two_digit_number)
{
    using namespace glm;
    mat4 ret(1.0f);
    vec2 num_ret(1.0f);
    GridMeasures& ms = product_grid.measures;
    u32 two_digit_offset = two_digit_number ? double_digit_offset : single_digit_offset;

    ret = translate(ret, vec3(ms.entry_position, 0.0f));
    num_ret = ms.number_position - ivec2(two_digit_offset, 0);

    mat4 icon_transform = scale(ret, GridMeasures::tile_transform);
    return { icon_transform, num_ret };
}

bool Inventory::PerformCleanup(std::optional<InventoryEntry>& first, std::optional<InventoryEntry>& second)
{
    if (first.has_value() && first->item_type == second->item_type && first->item_count + second->item_count <= s_MaxItemsPerSlot) {
        first->item_count += second->item_count;
        second = std::nullopt;
        return true;
    }
    else if (!first.has_value()) {
        first = second;
        second = std::nullopt;
        return true;
    }

    return false;
}

void LoadRecipes(Utils::Vector<Recipe2x2>& recipes_2x2, Utils::Vector<Recipe3x3>& recipes_3x3)
{
    recipes_2x2.clear();
    recipes_3x3.clear();
    //Lets get rusty poggers
    auto some = [](Defs::Item type) {return std::make_optional(type); };
    auto none = []() {return std::nullopt; };
    
    using enum Defs::Item;
    recipes_2x2.push_back({some(Wood),          none(),
                           none(),              none(),
                           {WoodPlanks, 4} });

    recipes_2x2.push_back({ some(WoodPlanks),   none(),
                            some(WoodPlanks),   none(),
                            {WoodStick, 4} });

    recipes_2x2.push_back({ some(WoodPlanks),   some(WoodPlanks),
                            some(WoodPlanks),   some(WoodPlanks),
                            {CraftingTable, 1} });

    recipes_3x3.push_back({ {   some(WoodPlanks),   some(WoodPlanks),   some(WoodPlanks),
                                none(),             some(WoodStick),    none()          ,
                                none(),             some(WoodStick),    none()          },
                                {WoodPickaxe, 1} });
}
