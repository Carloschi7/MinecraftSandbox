#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <format>
#include <functional>
#include "utils/types.h"

//Used to refer to the virtual space address created by the memory mapped_space
//no direct access to the physycal memory space is granted because of 
//memory locking
typedef u64 VAddr;

//Virtual pointer mapped in a Memory::Arena, VAddr alias that leaves the pointed type explicit
template<class T>
using Ptr = VAddr;

//msg for now its just a warning displayed in the same line of the assert, no practical usage
#define MC_ASSERT(x, msg)\
	if(!(x)){*(int*)0 = 0;}
#define MC_CLOG(msg, ...) std::printf(msg, __VA_ARGS__)
#define MC_LOG(msg, ...) std::printf(std::format(msg, __VA_ARGS__).c_str());

//TODO fix shadows and add crafting table with some very basic crafting patterns, then release

namespace Memory 
{
	struct MappedSpace
	{
		void* memory = nullptr;
		u64 memory_size = 0;
		u64 memory_used = 0;
		//Describes all the mapped regions, the first u64 indicates the region begin and the second one the region end
		std::map<u64, u64> mapped_regions;
	};

	//Each (non unchecked) region is composed by a signature u32, an u32 which tells if the memory is locked, and then the actual payload
	//Random signature to ensure validity, translate to ascii "regb" -> Region Begin
	static constexpr u32 signature = 0x62676572;
	static constexpr u32 padding = 2 * sizeof(u32);

	struct Arena {
		MappedSpace mapped_space;
		bool initialized;
		std::mutex arena_mutex;
		std::condition_variable arena_condition_variable;

		//Variable which tracks how much allocated memory won't be explicitly freed by
		//destructors or some other equivalent methods. This is used to track small buffers
		//of data that is trivially copiable, meaning that no explicit destructor must be called
		//in order to free the memory we are concerned about.
		//E.G this may track numbers, arrays of numbers, arrays of vectors and other stuff that
		//can be deleted just with the mapped_space destruction without ever having the posibility to
		//produce any leak. Also objects with empty distructors or with distructors that free
		//memory which is mapped in the mapped_space are allowed to be referred here
		//(In this current app implementation this is going to be used only for the first case)
		u32 unfreed_mem = 0;
	};

	//NOTE: is it better to implement these as Arena methods?
	Arena* InitializeArena(u64 bytes);
	void DestroyArena(Arena* arena);

	VAddr Allocate(Arena* arena, u64 size);
	void Free(Arena* arena, VAddr ptr);
	void* AllocateUnchecked(Arena* arena, u64 size);
	void FreeUnchecked(Arena* arena, void* ptr);
	void* Get(Arena* arena, VAddr addr);

	template<class T>
	inline T* Get(Arena* arena, VAddr addr) { return static_cast<T*>(Get(arena, addr)); }

	void LockRegion(Arena* arena, VAddr addr);
	void UnlockRegion(Arena* arena, VAddr addr);

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
	VAddr New(Arena* arena, Args&&... arguments) requires (!std::is_array_v<T>)
	{
		VAddr addr = Allocate(arena, sizeof(T));
		void* paddr = static_cast<u8*>(arena->mapped_space.memory) + addr + padding;
		//Placement new, to construct an object in a pre-allocated region of memory
		new (paddr) T{ std::forward<Args>(arguments)... };
		return addr;
	}

	//Support also integral arrays
	template <class T, class Underlying = typename RemoveArray<T>::type>
	VAddr New(Arena* arena, u64 count) requires (std::is_array_v<T> && std::is_integral_v<Underlying>)
	{
		VAddr addr = Allocate(sizeof(Underlying) * count);
		return addr;
	}

	template <class T, class... Args>
	T* NewUnchecked(Arena* arena, Args&&... arguments) requires (!std::is_array_v<T>)
	{
		void* addr = AllocateUnchecked(arena, sizeof(T));
		//Placement new, to construct an object in a pre-allocated region of memory
		new (addr) T{ std::forward<Args>(arguments)... };
		return static_cast<T*>(addr);
	}

	template<class T>
	void Delete(Arena* arena, VAddr addr) requires (!std::is_array_v<T>)
	{
		u8* paddr = static_cast<u8*>(arena->mapped_space.memory) + addr;
		u32* owner = std::bit_cast<u32*>(paddr) + 1;
      	MC_ASSERT(*owner == 0, "You cant free a locked region");

		//Retrieve the actual object offset and calling the distructor
		std::bit_cast<T*>(paddr + padding)->~T();
		Free(arena, addr);
	}

	template<class T, class Underlying = typename RemoveArray<T>::type>
	void Delete(Arena* arena, VAddr addr) requires (std::is_array_v<T> && std::is_integral_v<Underlying>)
	{
		//Destructor not needed in this case
		Free(arena, addr);
	}

	template<class T>
	void DeleteUnchecked(Arena* arena, T* addr) requires (!std::is_array_v<T>)
	{
		addr->~T();
		FreeUnchecked(arena, addr);
	}
}


//Experimental
#define defer(body) \
struct DeferImpl##__FUNCTION__##__LINE__ \
{\
	DeferImpl##__FUNCTION__##__LINE__(const std::function<void()>& _func) : func(_func) {}\
	~DeferImpl##__FUNCTION__##__LINE__() { func(); }\
	std::function<void()> func;\
}; \
DeferImpl##__FUNCTION__##__LINE__ defer_impl##__FUNCTION__##__LINE__([&]() body);