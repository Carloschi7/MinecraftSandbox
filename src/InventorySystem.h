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
	GlCore::State& m_State;

	std::array<std::optional<Defs::BlockType>, Defs::g_InventoryInternalSlotsCount> m_InternalSlots;
	std::array<std::optional<Defs::BlockType>, Defs::g_InventoryScreenSlotsCount> m_ScreenSlots;

	//This values fit the inventory only for the current image scaling, very important to note
	glm::vec2 m_InventoryStart;
	glm::vec2 m_IntervalDimension;

	glm::mat4 m_InternAbsTransf;
	glm::mat4 m_ScreenAbsTransf;
	glm::mat4 InternalSlotAbsoluteTransform(uint32_t slot_index);
	glm::mat4 ScreenSlotAbsoluteTransform(uint32_t slot_index);
};