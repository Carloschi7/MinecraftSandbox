#include "Memory.h"
#include <algorithm>
#include <bit>

namespace mem
{
	Arena g_Arena;
	bool g_ArenaInitialized = false;
	std::mutex g_ArenaMutex;
	std::condition_variable g_ArenaConditionVariable;

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
		u64 required_bytes = size + padding;
		MC_ASSERT(g_Arena.memory_used + required_bytes < g_Arena.memory_size, "there is no more space in the memory arena");
		Region reg;
		auto& mapped_regions = g_Arena.mapped_regions;

		VAddr addr = 0;
		if (!mapped_regions.empty()) {
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
		}
		reg.begin = addr;
		reg.end = addr + required_bytes;
		mapped_regions.push_back(reg);
		std::sort(mapped_regions.begin(), mapped_regions.end(), [](const Region& a, const Region& b) {return a.begin < b.begin; });

		u32* ptr = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + addr);
		*ptr = signature;
		*(ptr + 1) = 0;

		g_Arena.memory_used += required_bytes;
		return addr;
	}

	void Free(VAddr ptr)
	{
		auto& mapped_regions = g_Arena.mapped_regions;
		for (auto iter = mapped_regions.begin(); iter != mapped_regions.end(); ++iter) {
			if (iter->begin == ptr) {
				//mark the memory as freed
				u32* ptr = reinterpret_cast<u32*>(static_cast<u8*>(g_Arena.memory) + iter->begin);

				MC_ASSERT(*(ptr + 1) == 0, "You cant free a locked region");

				*ptr = 0;
				*(ptr + 1) = 0;

				g_Arena.memory_used -= (iter->end - iter->begin);
				mapped_regions.erase(iter);
				return;
			}
		}
	}

	void Free(void* ptr)
	{
		u8* address = static_cast<u8*>(ptr) - padding;
		return Free(std::bit_cast<VAddr>(address - g_Arena.memory));
	}

	void* Get(VAddr addr)
	{
		//Memory is guaranteed to exist here
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + addr);
		u32* owner = address + 1;
#ifdef _DEBUG
		//We keep this to fix bugs
		MC_ASSERT(*address == signature, "trying to access invalid memory");
#else
		//In release mode we dont bother with this.
		//We can assume two things could happen if *address != signature:
		//either the vaddr is pointing to some random address which is not a "regb" value
		//or there actually was a valid region that was released by another thread in parallel

		//If something ever gets removed from the arena memory that means the region pointed to data
		//which is not needed now, so just tell the caller that the information that was here
		//is no longer relevant
		if (*address != signature)
			return nullptr;
#endif
		if (*owner != 0 && *owner != std::bit_cast<u32>(std::this_thread::get_id())) {
			std::unique_lock<std::mutex> lock{ g_ArenaMutex };
			g_ArenaConditionVariable.wait(lock, [owner]() {return *owner == 0; });
		}

		return address + padding / sizeof(u32);
	}



	void LockRegion(VAddr addr)
	{
		MC_ASSERT(addr < g_Arena.memory_size, "virtual address out of buonds from the virtual space");
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + addr);
		u32* owner = address + 1;
#ifdef _DEBUG
		MC_ASSERT(*address == signature, "memory can be locked only as a full region, please provide a valid virtual address");
#else
		if (*address != signature)
			return;
#endif
		if (*owner == std::bit_cast<u32>(std::this_thread::get_id()))
			return;
		if (*owner != 0) {
			std::unique_lock<std::mutex> lock{ g_ArenaMutex };
			g_ArenaConditionVariable.wait(lock, [owner]() {return *owner == 0; });
		}

		{
			std::unique_lock<std::mutex> lock{ g_ArenaMutex };
			*owner = std::bit_cast<u32>(std::this_thread::get_id());
		}

		//g_ArenaConditionVariable.notify_all();
	}

	void UnlockRegion(VAddr addr)
	{
		MC_ASSERT(addr < g_Arena.memory_size, "virtual address out of buonds from the virtual space");
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(g_Arena.memory) + addr);
		u32* owner = address + 1;
		//In this case the region is already locked by the current thread
		//so we leave this because this MUST be true everytime
		MC_ASSERT(*address == signature, "memory can be locked only as a full region, please provide a valid virtual address");
		if (*owner == std::bit_cast<u32>(std::this_thread::get_id())) {
			{
				std::unique_lock<std::mutex> lock{ g_ArenaMutex };
				*owner = 0;
			}
			g_ArenaConditionVariable.notify_all();
		}

	}
}

