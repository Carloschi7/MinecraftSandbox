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

struct GridMeasures
{
	glm::ivec2 entry_position;
	glm::ivec2 entry_stride;
	glm::ivec2 number_position;
	glm::ivec2 number_stride;

	static constexpr glm::vec3 tile_transform = glm::vec3(96.0f, 75.0f, 0.0f);
};

struct Grid {
	Grid() {}
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
	Inventory();
	void AddToNewSlot(Defs::BlockType block);
	void HandleInventorySelection();
	void InternalSideRender();
	void ScreenSideRender();
	std::optional<InventoryEntry>& HoveredFromSelector();
	void ClearUsedSlots();
private:
	void RenderEntry(InventoryEntry entry, u32 binding_index);
	void RenderScreenEntry(InventoryEntry binding, u32 binding_index);
	void RenderPendingEntry(InventoryEntry entry);
	std::pair<glm::mat4, glm::vec2> SlotTransform(u32 slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotScreenTransform(u32 slot_index, bool two_digit_number);
private:
	GlCore::State& m_State;
	TextRenderer m_TextRenderer;

	static constexpr u32 s_InventorySize = Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount;
	static constexpr u8 s_MaxItemsPerSlot = 64;
	std::array<std::optional<InventoryEntry>, s_InventorySize> m_Slots;
	std::optional<u32> m_PendingEntry;

	//This values fit the inventory only for the current image scaling, very important to note
	glm::vec2 m_InternalStart;
	glm::vec2 m_ScreenStart;
	glm::vec2 m_IntervalDimension;

	glm::mat4 m_InternAbsTransf;
	glm::mat4 m_ScreenAbsTransf;
	i32 m_CursorIndex;

	static constexpr f32 single_digit_offset = 0.0f;
	static constexpr f32 double_digit_offset = 36.0f;
	static constexpr f32 pending_single_digit_offset = 10.0f;
	static constexpr f32 pending_double_digit_offset = -25.0f;

	//Tree main grids,
	//	internal
	//	internal for screen section
	//	screen section
	Grid m_InternalGrid, m_InternalScreenGrid, m_ScreenGrid;
};