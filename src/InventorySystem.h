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

struct InventoryMeasures
{
	glm::vec3 irn;
	glm::vec3 irn_offset;
	glm::vec2 irn_num_internal;
	glm::vec2 irn_num_offset;
	//Spart means screen part, the screen inventory slot in the internal side
	glm::vec3 irn_spart;
	glm::vec2 irn_num_spart;

	glm::vec3 scr;
	float scr_offset;
	glm::vec2 scr_num;
	float scr_num_offset;

	glm::vec3 tile_transform;
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
	std::pair<glm::mat4, glm::vec2> SlotTransform(uint32_t slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotScreenTransform(uint32_t slot_index, bool two_digit_number);
private:
	GlCore::State& m_State;
	TextRenderer m_TextRenderer;

	static constexpr uint32_t s_InventorySize = Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount;
	static constexpr uint8_t s_MaxItemsPerSlot = 64;
	std::array<std::optional<InventoryEntry>, s_InventorySize> m_Slots;
	std::optional<uint32_t> m_PendingEntry;

	//This values fit the inventory only for the current image scaling, very important to note
	glm::vec2 m_InternalStart;
	glm::vec2 m_ScreenStart;
	glm::vec2 m_IntervalDimension;

	glm::mat4 m_InternAbsTransf;
	glm::mat4 m_ScreenAbsTransf;
	int32_t m_CursorIndex;

	//Measures that fit the textures
	InventoryMeasures m_Measures;
};