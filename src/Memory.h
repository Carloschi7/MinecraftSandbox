#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include "utils/types.h"

//Used to refer to the virtual space address created by the memory arena
//no direct access to the physycal memory space is granted because of 
//memory locking
typedef u64 VAddr;

//msg for now its just a warning displayed in the same line of the assert, no practical usage
#define MC_ASSERT(x, msg)\
	if(!(x)){*(int*)0 = 0;}
#define MC_LOG(msg, ...) std::printf(msg, __VA_ARGS__);

namespace mem 
{
	//Each region is composed by a signature u32, an u32 which tells if the memory is locked, and then the actual payload
	struct Region
	{
		VAddr begin;
		VAddr end;
	};

	struct Arena
	{
		void* memory = nullptr;
		u64 memory_size = 0;
		u64 memory_used = 0;
		std::vector<Region> mapped_regions;
	};

	//Random signature to ensure validity, translate to ascii "regb" -> Region Begin
	static constexpr u32 signature = 0x62676572;
	static constexpr u32 padding = 2 * sizeof(u32);

	extern Arena g_Arena;
	extern bool g_ArenaInitialized;
	extern std::mutex g_ArenaMutex;
	extern std::condition_variable g_ArenaConditionVariable;

	void InitializeArena(u64 size);
	void DestroyArena();
	VAddr Allocate(u64 size);
	void Free(VAddr ptr);
	void Free(void* ptr);
	void* Get(VAddr addr);

	template<class T>
	inline T* Get(VAddr addr) { return static_cast<T*>(Get(addr)); }

	void LockRegion(VAddr addr);
	void UnlockRegion(VAddr addr);

	template <class T>
	struct RemoveArray {
		using type = T;
	};

	template <class T>
	struct RemoveArray<T[]> {
		using type = T;
	};

	//Uses c++20 features
	template <class T, class... Args>
	VAddr New(Args&&... arguments) requires (!std::is_array_v<T>)
	{
		VAddr addr = Allocate(sizeof(T));
		void* paddr = static_cast<u8*>(g_Arena.memory) + addr + padding;
		//Placement new, to construct an object in a pre-allocated region of memory
		new (paddr) T{ std::forward<Args>(arguments)... };
		return addr;
	}

	//Support also integral arrays
	template <class T, class Underlying = typename RemoveArray<T>::type>
	VAddr New(u64 count) requires (std::is_array_v<T> && std::is_integral_v<Underlying>)
	{
		VAddr addr = Allocate(sizeof(Underlying) * count);
		return addr;
	}

	template<class T>
	void Delete(VAddr addr) requires (!std::is_array_v<T>)
	{
		u8* paddr = static_cast<u8*>(g_Arena.memory) + addr;
		u32* owner = std::bit_cast<u32*>(paddr) + 1;
      	MC_ASSERT(*owner == 0, "You cant free a locked region");

		//Retrieve the actual object offset and calling the distructor
		std::bit_cast<T*>(paddr + padding)->~T();
		Free(addr);
	}

	template<class T, class Underlying = typename RemoveArray<T>::type>
	void Delete(VAddr addr) requires (std::is_array_v<T> && std::is_integral_v<Underlying>)
	{
		//Destructor not needed in this case
		Free(addr);
	}

	//Custom allocator for dynamic arrays such as vectors so that they can use the memory arena
	template <typename T>
	class ArenaAllocator {
	public:
		using value_type = T;
		
		ArenaAllocator() = default;

		template <typename U>
		ArenaAllocator(const ArenaAllocator<U>&) {}

		//Allow more ownership to some vectors
		T* allocate(std::size_t n) {
			return mem::Get<T>(mem::Allocate(n * sizeof(T)));
		}

		void deallocate(T* p, std::size_t n) {
			return mem::Free(p);
		}

		std::size_t max_size() const {
			return static_cast<std::size_t>(-1) / sizeof(T);
		}
	};
}
