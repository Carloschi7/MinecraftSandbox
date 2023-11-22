#include "Memory.h"
#include <algorithm>
#include <bit>

namespace Memory
{
	Arena* InitializeArena(u64 bytes)
	{
		Arena* arena = new Arena;
		MC_ASSERT(arena != nullptr);

		arena->mapped_space.memory = ::operator new(bytes);
		std::memset(arena->mapped_space.memory, 0, bytes);
		MC_ASSERT(arena->mapped_space.memory != nullptr, "not enough memory can be requested");
		arena->mapped_space.memory_size = bytes;
		arena->initialized = true;

		return arena;
	}

	void DestroyArena(Arena* arena)
	{
		MC_ASSERT(arena != nullptr);
		if (arena->mapped_space.memory) {
			::operator delete(arena->mapped_space.memory);
			arena->mapped_space.memory_size = 0;
			arena->mapped_space.mapped_regions.clear();

			arena->initialized = false;
		}
		delete arena;
	}


	VAddr Allocate(Arena* arena, u64 size)
	{
		u64 required_bytes = size + padding;
		MC_ASSERT(arena->mapped_space.memory_used + required_bytes < arena->mapped_space.memory_size, "there is no more space in the memory arena");
		auto& mapped_regions = arena->mapped_space.mapped_regions;

		VAddr addr = 0;
		if (!mapped_regions.empty()) {
			bool found = false;
			for (auto iter = mapped_regions.begin(), next_iter = std::next(mapped_regions.begin());
				next_iter != mapped_regions.end(); ++iter, ++next_iter) {
				if (next_iter->first - iter->second >= required_bytes) {
					addr = iter->second;
					found = true;
					break;
				}
			}

			if (!found) {
				addr = std::prev(mapped_regions.end())->second;
				MC_ASSERT(arena->mapped_space.memory_size - addr >= required_bytes, "there is no more space in the arena");
			}
		}
		mapped_regions[addr] = addr + required_bytes;
		u32* ptr = std::bit_cast<u32*>(static_cast<u8*>(arena->mapped_space.memory) + addr);
		*ptr = signature;
		*(ptr + 1) = 0;

		arena->mapped_space.memory_used += required_bytes;
		return addr;
	}

	void Free(Arena* arena, VAddr addr)
	{
		auto& mapped_regions = arena->mapped_space.mapped_regions;
		auto iter = mapped_regions.find(addr);
#ifdef _DEBUG
		MC_ASSERT(iter != mapped_regions.end());
#else
		if (iter == mapped_regions.end())
			return;
#endif

		u32* ptr = reinterpret_cast<u32*>(static_cast<u8*>(arena->mapped_space.memory) + iter->first);
		*ptr = 0;
		*(ptr + 1) = 0;
		arena->mapped_space.memory_used -= (iter->second - iter->first);
		mapped_regions.erase(iter);
	}

	void* AllocateUnchecked(Arena* arena, u64 size)
	{
		MC_ASSERT(arena->mapped_space.memory_used + size < arena->mapped_space.memory_size, "there is no more space in the memory arena");
		auto& mapped_regions = arena->mapped_space.mapped_regions;

		VAddr addr = 0;
		if (!mapped_regions.empty()) {
			bool found = false;
			for (auto iter = mapped_regions.begin(), next_iter = std::next(mapped_regions.begin());
				next_iter != mapped_regions.end(); ++iter, ++next_iter) {
				if (next_iter->first - iter->second >= size) {
					addr = iter->second;
					found = true;
					break;
				}
			}

			if (!found) {
				addr = std::prev(mapped_regions.end())->second;
				MC_ASSERT(arena->mapped_space.memory_size - addr >= size, "there is no more space in the arena");
			}
		}
		mapped_regions[addr] = addr + size;
		arena->mapped_space.memory_used += size;
		return static_cast<u8*>(arena->mapped_space.memory) + addr;
	}

	void FreeUnchecked(Arena* arena, void* ptr)
	{
		auto& mapped_regions = arena->mapped_space.mapped_regions;
		VAddr addr = static_cast<VAddr>(static_cast<u8*>(ptr) - static_cast<u8*>(arena->mapped_space.memory));

		auto iter = mapped_regions.find(addr);
#ifdef _DEBUG
		MC_ASSERT(iter != mapped_regions.end());
#else
		if (iter == mapped_regions.end())
			return;
#endif

		arena->mapped_space.memory_used -= (iter->second - iter->first);
		mapped_regions.erase(iter);
	}

	void* Get(Arena* arena, VAddr addr)
	{
		//Memory is guaranteed to exist here
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(arena->mapped_space.memory) + addr);
		u32* owner = address + 1;
#ifdef _DEBUG
		//We keep this to fix bugs
		MC_ASSERT(*address == signature, "trying to access invalid memory");
#else
		//In release mode we dont bother with this.
		//We can assume two things could happen if *address != signature:
		//either the vaddr is pointing to some random address which is not a "regb" value
		//or there actually was a valid region that was released by another thread in parallel

		//If something ever gets removed from the mapped_space memory that means the region pointed to data
		//which is not needed now, so just tell the caller that the information that was here
		//is no longer relevant
		if (*address != signature)
			return nullptr;
#endif
		if (*owner != 0 && *owner != std::bit_cast<u32>(std::this_thread::get_id())) {
			std::unique_lock<std::mutex> lock{ arena->arena_mutex };
			arena->arena_condition_variable.wait(lock, [owner]() {return *owner == 0; });
		}

		return address + padding / sizeof(u32);
	}



	void LockRegion(Arena* arena, VAddr addr)
	{
		MC_ASSERT(addr < arena->mapped_space.memory_size, "virtual address out of buonds from the virtual space");
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(arena->mapped_space.memory) + addr);
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
			std::unique_lock<std::mutex> lock{ arena->arena_mutex };
			arena->arena_condition_variable.wait(lock, [owner]() {return *owner == 0; });
		}

		{
			std::unique_lock<std::mutex> lock{ arena->arena_mutex };
			*owner = std::bit_cast<u32>(std::this_thread::get_id());
		}
	}

	void UnlockRegion(Arena* arena, VAddr addr)
	{
		MC_ASSERT(addr < arena->mapped_space.memory_size, "virtual address out of buonds from the virtual space");
		u32* address = std::bit_cast<u32*>(static_cast<u8*>(arena->mapped_space.memory) + addr);
		u32* owner = address + 1;
		//In this case the region is already locked by the current thread
		//so we leave this because this MUST be true everytime
		MC_ASSERT(*address == signature, "memory can be locked only as a full region, please provide a valid virtual address");
		if (*owner == std::bit_cast<u32>(std::this_thread::get_id())) {
			{
				std::unique_lock<std::mutex> lock{ arena->arena_mutex };
				*owner = 0;
			}
			arena->arena_condition_variable.notify_all();
		}

	}
}

