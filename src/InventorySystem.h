#pragma once
#include "glm/glm.hpp"
#include "State.h"
#include "GameDefinitions.h"
#include <optional>
#include <array>
#include "TextRenderer.h"

struct InventoryEntry
{
	Defs::BlockType block_type;
	u8 block_count; //Max 64 like in the original, so small type used
};

enum class GridType : u8
{
	Grid2x2 = 0,
	Grid3x3
};

enum class EntryType : u8
{
	Default = 0,
	Screen,
	Crafting_2x2,
	Crafting_3x3,
	Pending
};

struct GridMeasures
{
	glm::ivec2 entry_position;
	glm::ivec2 entry_stride;
	glm::ivec2 number_position;
	glm::ivec2 number_stride;

	static constexpr glm::vec3 tile_transform = glm::vec3(96.0f, 75.0f, 0.0f);
};

struct Grid {
	Grid() = default;
	Grid(const Grid&) = default;
	Grid& operator=(const Grid&) = default;
	Grid(glm::ivec2 start, glm::ivec2 end, u32 x_slots, u32 y_slots)
	{
		measures.entry_position = start;
		measures.entry_stride = measures.number_stride = {
			(end.x - start.x) / x_slots,
			(end.y - start.y) / y_slots
		};
		measures.number_position = start + glm::ivec2(12, 3);
	}

	GridMeasures measures;
};

class Inventory
{
public:
	Inventory(TextRenderer& text_renderer);
	void AddToNewSlot(Defs::BlockType block);
	void HandleInventorySelection();
	void InternalRender();
	void ScreenRender();
	std::optional<InventoryEntry>& HoveredFromSelector();
	void ClearUsedSlots();
	void ViewCleanup();
private:
	void RenderEntry(EntryType entry_type, InventoryEntry entry, u32 binding_index);
	std::pair<glm::mat4, glm::vec2> SlotTransform(u32 slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotScreenTransform(u32 slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotCraftingTransform(bool grid_3x3, u32 slot_index, bool two_digit_number);
	bool PerformCleanup(std::optional<InventoryEntry>& first, std::optional<InventoryEntry>& second);
public:
	//Tells if this is a crafting table view or a normal inventory view
	bool view_crafting_table = false;
private:
	GlCore::State& m_State;
	TextRenderer& m_TextRenderer;

	static constexpr u8 s_InventorySize = Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount;
	static constexpr u8 s_MaxItemsPerSlot = 64;
	static constexpr u8 s_CraftingMaxSize = Defs::g_CraftingSlotsMaxCount;
	std::array<std::optional<InventoryEntry>, s_InventorySize> m_Slots;
	std::array<std::optional<InventoryEntry>, s_CraftingMaxSize> m_CraftingSlots;
	std::optional<InventoryEntry> m_PendingEntry;

	glm::mat4 m_InternAbsTransf;
	glm::mat4 m_ScreenAbsTransf;
	s32 m_CursorIndex;

	static constexpr s32 single_digit_offset = 0;
	static constexpr s32 double_digit_offset = 36;
	static constexpr s32 pending_single_digit_offset = 10;
	static constexpr s32 pending_double_digit_offset = -25;

	//Tree main grids,
	//	internal
	//	internal for screen section
	//	screen section
	//MAIN INVENTORY GRID
	Grid m_InternalGrid, m_ScreenGrid, m_InternalScreenGrid;
	Grid crafting_2x2, crafting_3x3;
};