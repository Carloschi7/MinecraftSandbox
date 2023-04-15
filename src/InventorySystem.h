#pragma once
#include "glm/glm.hpp"
#include "State.h"
#include "GameDefinitions.h"
#include <optional>
#include <array>

class Inventory
{
public:
	Inventory();
	void HandleInventorySelection();
	void InternalSideRender();
	void ScreenSideRender();
private:
	void RenderEntry(Defs::TextureBinding binding, uint32_t binding_index);
	void RenderScreenEntry(Defs::TextureBinding binding, uint32_t binding_index);
	void RenderPendingEntry(Defs::TextureBinding binding);
	glm::mat4 SlotTransform(uint32_t slot_index);
	glm::mat4 SlotScreenTransform(uint32_t slot_index);
private:
	GlCore::State& m_State;

	std::array<std::optional<Defs::BlockType>, Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount> m_Slots;
	std::optional<uint32_t> m_PendingEntry;

	//This values fit the inventory only for the current image scaling, very important to note
	glm::vec2 m_InternalStart;
	glm::vec2 m_ScreenStart;
	glm::vec2 m_IntervalDimension;

	glm::mat4 m_InternAbsTransf;
	glm::mat4 m_ScreenAbsTransf;
	int32_t m_CursorIndex;
};