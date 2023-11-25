#pragma once
#include "glm/glm.hpp"
#include "State.h"
#include "GameDefinitions.h"
#include <optional>
#include <array>
#include "TextRenderer.h"

struct InventoryEntry
{
	Defs::Item item_type;
	u8 item_count; //Max 64 like in the original, so small type used

	bool CanBeInserted(InventoryEntry other) 
	{
		return item_type == other.item_type &&
			item_count + other.item_count <= Defs::g_MaxItemsPerSlot;
	}
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
	CraftingProduct,
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

//Game related inventory stuff
struct Recipe2x2 
{
	template<uint32_t count>
	using EntryArray = std::array<std::optional<InventoryEntry>, count>;
	template<uint32_t count>
	using IngredientArray = std::array<std::optional<Defs::Item>, count>;

	//NOTE these probably should be riconverted to counted entries for much more
	//specific crafting. This feature will not be used for now
	IngredientArray<4> ingredients;
	//Result block type and quantity
	InventoryEntry product;
	//Accepts maximum entry array dimensions (crafting-table like)
	bool Matches(const EntryArray<Defs::g_CraftingSlotsMaxCount>& entry_array) const 
	{
		auto equals = [](const std::optional<InventoryEntry>& a, const std::optional<Defs::Item>& b) ->bool
		{
			return(!a.has_value() && !b.has_value()) || (a.has_value() && b.has_value() && a.value().item_type == b.value());
		};

		//Assert the number of items is the same, so there wont be extra items inserted
		//beyond the checked area
		u8 entry_count = 0, actual_count = 0;
		for (auto entry : entry_array) {
			entry_count += static_cast<u8>(entry.has_value());
		}
		for (auto entry : ingredients) {
			actual_count += static_cast<u8>(entry.has_value());
		}
		if (entry_count != actual_count)
			return false;

		for (u8 i = 0; i < 2; i++) {
			for (u8 j = 0; j < 2; j++) {
				if (equals(entry_array[i * 3 + j], ingredients[0]) &&
					equals(entry_array[i * 3 + (j + 1)], ingredients[1]) &&
					equals(entry_array[(i + 1) * 3 + j], ingredients[2]) &&
					equals(entry_array[(i + 1) * 3 + (j + 1)], ingredients[3])) 
				{
					return true;
				}
			}
		}

		return false;
	}
};

struct Recipe3x3 
{
	template<uint32_t count>
	using EntryArray = std::array<std::optional<InventoryEntry>, count>;
	template<uint32_t count>
	using IngredientArray = std::array<std::optional<Defs::Item>, count>;

	IngredientArray<Defs::g_CraftingSlotsMaxCount> ingredients;
	InventoryEntry product;

	bool Matches(const EntryArray<Defs::g_CraftingSlotsMaxCount>& entry_array) const
	{
		auto equals = [](const std::optional<InventoryEntry>& a, const std::optional<Defs::Item>& b) ->bool
		{
			return(!a.has_value() && !b.has_value()) || (a.has_value() && b.has_value() && a.value().item_type == b.value());
		};

		for (u8 i = 0; i < Defs::g_CraftingSlotsMaxCount; i++) {
			if (!equals(entry_array[i], ingredients[i]))
				return false;
		}

		return true;
	}

};

void LoadRecipes(Utils::AVector<Recipe2x2>& recipes_2x2, Utils::AVector<Recipe3x3>& recipes_3x3);

class Inventory
{
public:
	Inventory(TextRenderer& text_renderer);
	void AddToNewSlot(Defs::Item block);
	void HandleInventorySelection();
	void ProcessRecipes();
	void InternalRender();
	void ScreenRender();
	std::optional<InventoryEntry>& HoveredFromSelector();
	const std::optional<InventoryEntry>& HoveredFromSelector() const;
	void ClearUsedSlots();
	void ViewCleanup();
private:
	void RenderEntry(EntryType entry_type, InventoryEntry entry, u32 binding_index);
	std::pair<glm::mat4, glm::vec2> SlotTransform(u32 slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotScreenTransform(u32 slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotCraftingTransform(bool grid_3x3, u32 slot_index, bool two_digit_number);
	std::pair<glm::mat4, glm::vec2> SlotCraftingProductTransform(bool two_digit_number);
	bool PerformCleanup(std::optional<InventoryEntry>& first, std::optional<InventoryEntry>& second);
public:
	//Tells if this is a crafting table view or a normal inventory view
	bool view_crafting_table = false;
private:
	GlCore::State& m_State;
	TextRenderer& m_TextRenderer;

	static constexpr u8 s_InventorySize = Defs::g_InventoryInternalSlotsCount + Defs::g_InventoryScreenSlotsCount;
	static constexpr u8 s_MaxItemsPerSlot = Defs::g_MaxItemsPerSlot;
	static constexpr u8 s_CraftingMaxSize = Defs::g_CraftingSlotsMaxCount;
	std::array<std::optional<InventoryEntry>, s_InventorySize> m_Slots;
	std::array<std::optional<InventoryEntry>, s_CraftingMaxSize> m_CraftingSlots;
	std::optional<InventoryEntry> m_ProductEntry;
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
	Grid crafting_2x2, crafting_3x3, product_grid;

	//Recipes for crafting ingredients
	Utils::AVector<Recipe2x2> recipes_2x2;
	Utils::AVector<Recipe3x3> recipes_3x3;
};