#include "Memory.h"
#include "Utils.h"
#include <algorithm>
#include <bit>

namespace mem
{
	extern Arena g_Arena;
	extern bool g_ArenaInitialized = false;
	extern std::mutex g_ArenaMutex;
	extern std::condition_variable g_ArenaConditionVariable;

	//Random signature to ensure validity, translate to ascii "regb" -> Region Begin
	static constexpr u32 signature = 0x72656762;

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
		u64 required_bytes = size + sizeof(u32) * 2;
		MC_ASSERT(g_Arena.memory_used + required_bytes < g_Arena.memory_size, "there is no more space in the memory arena");
		Region reg;
		auto& mapped_regions = g_Arena.mapped_regions;

		if (mapped_regions.empty()) {
			reg.begin = 0;
			reg.end = required_bytes;
			mapped_regions.push_back(reg);
			return 0;
		}

		VAddr addr = 0;
		bool found = false;
		for (u32 i = 0; i < mapped_regions.size() - 1; i++) {
			if (mapped_regions[i + 1].begin - mapped_regions[i].end >= required_bytes) {
				addr = mapped_regions[i].end;
				found = true;
				break;
			}
		}

		if (!found) {
			MC_ASSERT(g_Arena.memory_size - mapped_regions.back().end >= required_bytes, "there is no more space in the arena");
			addr = mapped_regions.back().end;
		}
		reg.begin = addr;
		reg.end = addr + required_bytes;
		mapped_regions.push_back(reg);
		std::sort(mapped_regions.begin(), mapped_regions.end(), [](const Region& a, const Region& b) {return a.begin < b.begin; });

		u32* ptr = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + addr);
		*ptr = signature;
		*(ptr + 1) = 0;

		return addr;
	}

	void Free(VAddr ptr)
	{
		auto& mapped_regions = g_Arena.mapped_regions;
		for (auto iter = mapped_regions.begin(); iter != mapped_regions.end(); ++iter) {
			if (iter->begin == ptr) {
				//mark the memory as freed
				u32* ptr = reinterpret_cast<u32*>(static_cast<u8*>(g_Arena.memory) + iter->begin);
				*ptr = 0;
				*(ptr + 1) = 0;

				mapped_regions.erase(iter);
				return;
			}
		}
	}

	void* Get(VAddr addr)
	{
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + addr);
		u32 owner = *(address + 1);
		MC_ASSERT(*address == signature, "trying to access invalid memory");
		if (owner == 0 || owner == std::bit_cast<u32>(std::this_thread::get_id()))
			return address + 2;

		std::unique_lock<std::mutex> lock{ g_ArenaMutex };
		g_ArenaConditionVariable.wait(lock, [owner]() {return owner == 0; });
		return address + 2;
	}



	void LockRegion(VAddr ptr)
	{
		for (auto& region : g_Arena.mapped_regions) {
			if (region.begin == ptr) {
				u32* address = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + region.begin);
				u32 owner = *(address + 1);
				if (owner == std::bit_cast<u32>(std::this_thread::get_id()))
					return;
				if (owner != 0) {
					std::unique_lock<std::mutex> lock{ g_ArenaMutex };
					g_ArenaConditionVariable.wait(lock, [owner]() {return owner == 0; });
				}
				*(address + 1) = std::bit_cast<u32>(std::this_thread::get_id());
				g_ArenaConditionVariable.notify_all();
				return;
			}
		}
	}

	void UnlockRegion(VAddr ptr)
	{
		for (auto& region : g_Arena.mapped_regions) {
			if (region.begin == ptr) {
				u32* address = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + region.begin);
				u32 owner = *(address + 1);
				if (owner == std::bit_cast<u32>(std::this_thread::get_id()))
					*(address + 1) = 0;

				return;
			}
		}
	}
}

