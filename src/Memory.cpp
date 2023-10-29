#include "Memory.h"
#include <algorithm>
#include <bit>

namespace mem
{
	MemoryArena* InitializeArena(u64 size)
	{
		MemoryArena* ma = new MemoryArena;
		MC_ASSERT(ma != nullptr);

		ma->arena.memory = ::operator new(size);
		std::memset(ma->arena.memory, 0, size);
		MC_ASSERT(ma->arena.memory != nullptr, "not enough memory can be requested");
		ma->arena.memory_size = size;
		ma->initialized = true;

		return ma;
	}

	void DestroyArena(MemoryArena* ma)
	{
		MC_ASSERT(ma != nullptr);
		if (ma->arena.memory) {
			std::free(ma->arena.memory);
			ma->arena.memory_size = 0;
			ma->arena.mapped_regions.clear();

			ma->initialized = false;
		}
		delete ma;
	}


	VAddr Allocate(MemoryArena* ma, u64 size)
	{
		u64 required_bytes = size + padding;
		MC_ASSERT(ma->arena.memory_used + required_bytes < ma->arena.memory_size, "there is no more space in the memory arena");
		Region reg;
		auto& mapped_regions = ma->arena.mapped_regions;

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
				MC_ASSERT(ma->arena.memory_size - mapped_regions.back().end >= required_bytes, "there is no more space in the arena");
				addr = mapped_regions.back().end;
			}
		}
		reg.begin = addr;
		reg.end = addr + required_bytes;
		mapped_regions.push_back(reg);
		std::sort(mapped_regions.begin(), mapped_regions.end(), [](const Region& a, const Region& b) {return a.begin < b.begin; });

		u32* ptr = std::bit_cast<u32*>(static_cast<u8*>(ma->arena.memory) + addr);
		*ptr = signature;
		*(ptr + 1) = 0;

		ma->arena.memory_used += required_bytes;
		return addr;
	}

	void Free(MemoryArena* ma, VAddr ptr)
	{
		auto& mapped_regions = ma->arena.mapped_regions;
		for (auto iter = mapped_regions.begin(); iter != mapped_regions.end(); ++iter) {
			if (iter->begin == ptr) {
				//mark the memory as freed
				u32* ptr = reinterpret_cast<u32*>(static_cast<u8*>(ma->arena.memory) + iter->begin);

				MC_ASSERT(*(ptr + 1) == 0, "You cant free a locked region");

				*ptr = 0;
				*(ptr + 1) = 0;

				ma->arena.memory_used -= (iter->end - iter->begin);
				mapped_regions.erase(iter);
				return;
			}
		}
	}

	void Free(MemoryArena* ma, void* ptr)
	{
		u8* address = static_cast<u8*>(ptr) - padding;
		return Free(ma, std::bit_cast<VAddr>(address - ma->arena.memory));
	}

	void* Get(MemoryArena* ma, VAddr addr)
	{
		//Memory is guaranteed to exist here
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(ma->arena.memory) + addr);
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
			std::unique_lock<std::mutex> lock{ ma->arena_mutex };
			ma->arena_condition_variable.wait(lock, [owner]() {return *owner == 0; });
		}

		return address + padding / sizeof(u32);
	}



	void LockRegion(MemoryArena* ma, VAddr addr)
	{
		MC_ASSERT(addr < ma->arena.memory_size, "virtual address out of buonds from the virtual space");
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(ma->arena.memory) + addr);
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
			std::unique_lock<std::mutex> lock{ ma->arena_mutex };
			ma->arena_condition_variable.wait(lock, [owner]() {return *owner == 0; });
		}

		{
			std::unique_lock<std::mutex> lock{ ma->arena_mutex };
			*owner = std::bit_cast<u32>(std::this_thread::get_id());
		}
	}

	void UnlockRegion(MemoryArena* ma, VAddr addr)
	{
		MC_ASSERT(addr < ma->arena.memory_size, "virtual address out of buonds from the virtual space");
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(ma->arena.memory) + addr);
		u32* owner = address + 1;
		//In this case the region is already locked by the current thread
		//so we leave this because this MUST be true everytime
		MC_ASSERT(*address == signature, "memory can be locked only as a full region, please provide a valid virtual address");
		if (*owner == std::bit_cast<u32>(std::this_thread::get_id())) {
			{
				std::unique_lock<std::mutex> lock{ ma->arena_mutex };
				*owner = 0;
			}
			ma->arena_condition_variable.notify_all();
		}

	}
}

