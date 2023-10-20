#include "Memory.h"
#include "Utils.h"
#include <algorithm>

namespace mem
{
	extern Arena g_Arena;
	extern bool g_ArenaInitialized = false;
	extern std::mutex g_ArenaMutex;
	extern std::condition_variable g_ArenaConditionVariable;

	void InitializeArena(u64 size)
	{
		if (g_Arena.memory != nullptr)
			return;

		g_Arena.memory = ::operator new(size);
		std::memset(g_Arena.memory, 0, size);
		MC_ASSERT(g_Arena.memory != nullptr, "not enough memory can be requested");
		g_Arena.memory_size = size;
		g_ArenaInitialized = true;
	}

	void DestroyArena()
	{
		if (g_Arena.memory) {
			std::free(g_Arena.memory);
			g_Arena.memory_size = 0;
			g_Arena.mapped_regions.clear();

			g_ArenaInitialized = false;
		}
	}


	VAddr Allocate(u64 size)
	{
		MC_ASSERT(g_Arena.memory_used + size < g_Arena.memory_size, "there is no more space in the memory arena");
		Region reg;
		auto& mapped_regions = g_Arena.mapped_regions;

		if (mapped_regions.empty()) {
			reg.begin = 0;
			reg.end = size;
			mapped_regions.push_back(reg);
			return 0;
		}

		VAddr addr = 0;
		for (u32 i = 0; i < mapped_regions.size() - 1; i++) {
			if (mapped_regions[i + 1].begin - mapped_regions[i].end >= size) {
				addr = mapped_regions[i].end;

				reg.begin = addr;
				reg.end = addr + size;
				mapped_regions.push_back(reg);
				std::sort(mapped_regions.begin(), mapped_regions.end(), [](const Region& a, const Region& b) {return a.begin < b.begin; });
				return addr;
			}
		}

		MC_ASSERT(g_Arena.memory_size - mapped_regions.back().end >= size, "there is no more space in the arena");
		addr = mapped_regions.back().end;
		reg.begin = addr;
		reg.end = addr + size;
		mapped_regions.push_back(reg);
		std::sort(mapped_regions.begin(), mapped_regions.end(), [](const Region& a, const Region& b) {return a.begin < b.begin; });

		return addr;
	}

	void Free(VAddr ptr)
	{
		auto& mapped_regions = g_Arena.mapped_regions;
		for (auto iter = mapped_regions.begin(); iter != mapped_regions.end(); ++iter) {
			if (iter->begin == ptr) {
				mapped_regions.erase(iter);
				return;
			}
		}
	}

	void* Get(VAddr addr)
	{
		Region* match = nullptr;
		for (auto& region : g_Arena.mapped_regions) {
			if (region.begin == addr) {
				match = &region;
				break;
			}
		}

		void* address = static_cast<u8*>(g_Arena.memory) + addr;
		if (match && (match->owner == std::thread::id{} || match->owner == std::this_thread::get_id()))
			return address;

		std::unique_lock<std::mutex> lock{ g_ArenaMutex };
		g_ArenaConditionVariable.wait(lock, [match]() {return match->owner == std::thread::id{}; });
		return address;
	}



	void LockRegion(VAddr ptr)
	{
		for (auto& region : g_Arena.mapped_regions) {
			if (region.begin == ptr) {
				if (region.owner == std::thread::id{})
				{
					region.owner = std::this_thread::get_id();
					g_ArenaConditionVariable.notify_all();
					return;
				}
				else {
					std::unique_lock<std::mutex> lock{ g_ArenaMutex };
					g_ArenaConditionVariable.wait(lock, [region]() {return region.owner == std::thread::id(); });
				}
			}
		}
	}

	void UnlockRegion(VAddr ptr)
	{
		for (auto& region : g_Arena.mapped_regions)
			if (region.begin == ptr && region.owner == std::this_thread::get_id()) {
				region.owner = std::thread::id{};
				return;
			}
	}
}

