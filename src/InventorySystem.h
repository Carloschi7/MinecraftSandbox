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
	uint8_t block_count; //Max 64 like in the original, so small type used
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
	void RenderEntry(InventoryEntry entry, uint32_t binding_index);
	void RenderScreenEntry(InventoryEntry binding, uint32_t binding_index);
	void RenderPendingEntry(Defs::TextureBinding binding);
	std::pair<glm::mat4, glm::mat4> SlotTransform(uint32_t slot_index);
	std::pair<glm::mat4, glm::mat4> SlotScreenTransform(uint32_t slot_index);
private:
	GlCore::State& m_State;
	TextRenderer m_TextRenderer;

	static constexpr uint32_t INVENTORY_SIZE = Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount;
	std::array<std::optional<InventoryEntry>, INVENTORY_SIZE> m_Slots;
	std::optional<uint32_t> m_PendingEntry;

	//This values fit the inventory only for the current image scaling, very important to note
	glm::vec2 m_InternalStart;
	glm::vec2 m_ScreenStart;
	glm::vec2 m_IntervalDimension;

	glm::mat4 m_InternAbsTransf;
	glm::mat4 m_ScreenAbsTransf;
	int32_t m_CursorIndex;
};