#pragma once
#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <fstream>
#include <utility>
#ifdef __linux__
#include <cstring>
#endif
#include "Macros.h"
#include "Memory.h"

namespace GlCore {
	//Loads the state from the State.obj file without including the whole header file
	extern State* pstate;
}

//Utilities
namespace Utils
{
	std::string CompletePath(const char* str);
	u64 ChunkHasher(const glm::vec2& val);
	f32 PerspectiveItemRotation(f32 fov_degrees, bool is_block);
	//Custom allocator for dynamic arrays such as vectors so that they can use the memory mapped_space
	template <typename T>
	class ArenaAllocator {
	public:
		using value_type = T;

		ArenaAllocator() = default;

		template <typename U>
		ArenaAllocator(const ArenaAllocator<U>&) {}

		//Allow more ownership to some vectors
		T* allocate(std::size_t n) {
			Memory::Arena* inst = GlCore::pstate->memory_arena;
			return static_cast<T*>(Memory::AllocateUnchecked(inst, n * sizeof(T)));
		}
		void deallocate(T* p, std::size_t n) {
			Memory::Arena* inst = GlCore::pstate->memory_arena;
			return Memory::FreeUnchecked(inst, p);
		}
		std::size_t max_size() const {
			return static_cast<std::size_t>(-1) / sizeof(T);
		}
	};

	//MappedSpace vector
	template<class T>
	using Vector = std::vector<T, ArenaAllocator<T>>;
	template<class Key, class Item>
	using UnorderedMap = std::unordered_map<Key, Item, std::hash<Key>, std::equal_to<Key>, ArenaAllocator<std::pair<const Key, Item>>>;

	class Timer
	{
	public:
		Timer() = default;
		void StartTimer() { m_TimePoint = std::chrono::steady_clock::now(); }
		f32 GetElapsedMilliseconds() const
		{
			return GetElapsedSeconds() * 1000.0f;
		}
		f32 GetElapsedSeconds() const
		{
			auto now = std::chrono::steady_clock::now();
			return std::chrono::duration<f32>(now - m_TimePoint).count();
		}
	private:
		std::chrono::steady_clock::time_point m_TimePoint;
	};

	//Serializes object on the disk
	//As a convention, the & operator means serialization
	//and the % operator means deserialization
	//NOT THREAD SAFE
	class Serializer
	{
	public:
		Serializer(const std::string& filename, const char* mode);
		Serializer(const Serializer&) = default;
		Serializer(Serializer&& rhs) noexcept;
		~Serializer();

		//T object needs to be serializable
		template<typename T,
			std::enable_if_t<std::is_integral_v<T> | std::is_floating_point_v<T>,
			int> = 0>
		void Serialize(const T& obj) const
		{
			std::fwrite(&obj, sizeof(T), 1, m_File);
		}

		template<typename T,
			std::enable_if_t<std::is_integral_v<T> | std::is_floating_point_v<T>,
			int> = 0>
		T Deserialize() const
		{
			T obj;
			std::fread(&obj, sizeof(T), 1, m_File);
			return obj;
		}

		void Seek(std::size_t pos) const;
		bool Eof() const;
		u64 Tell() const;
		u64 FileSize() const;
		void Rewind();

		template<typename T>
		const Serializer& operator&(const T& obj) const
		{
			Serialize<T>(obj);
			return *this;
		}
		template<typename T>
		const Serializer& operator%(T& obj) const
		{
			obj = Deserialize<T>();
			return *this;
		}

	private:
		FILE* m_File;
	};
}

