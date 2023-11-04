#pragma once
#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <fstream>
#include <utility>
#include "Macros.h"
#include "Memory.h"
#include "State.h"

#ifndef BYTE_INDEX_FOR_BITS
	#define BYTE_INDEX_FOR_BITS(Bits) ((Bits - 1) / 8) + 1
#endif

#ifdef MC_SOURCE_PATH
	//Returns absolute path of the repo directory concatenated with the input string, useful for asset loading
	#define PATH(str) std::string(MC_SOURCE_PATH) + std::string("/") + std::string(str)
	#define CPATH(str) (PATH(str)).c_str()
#endif

//Utilities
namespace Utils
{
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
	using AVector = std::vector<T, ArenaAllocator<T>>;

	//Thread safe vector
	//This class is pseudo-safe, locking a mutex each of the many accesses in the TSvector
	//can be really expensive, expecially with more limited hardware.
	//So the locking process is explicit and can be performed via the functions lock and unlock
	//This is done because in the scenario we find ourselves needing to lock the vector only if
	//we are adding/removing elements
	//Data gets stored in a shared ptr so the content of the vector can get borrowed for a thread
	//and keeps on living if the original ptr stored by this ts vector gets erased
	template<class T>
	class TSVector
	{
		using iterator = typename std::vector<std::shared_ptr<T>>::iterator;
		using const_iterator = typename std::vector<std::shared_ptr<T>>::const_iterator;
	public:
		TSVector() = default;
		~TSVector() {}
		TSVector(const TSVector&) = default;
		TSVector(TSVector&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) :
			m_Container(std::move(rhs.m_Container))
		{}

		TSVector& operator=(const TSVector&) = default;
		TSVector& operator=(TSVector&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>)
		{
			m_Container = std::move(rhs.m_Container);
			return *this;
		}

		//Specific features
		void lock()
		{
			WaitUnlocked();
			m_OwningThreadID = std::this_thread::get_id();
		}
		void unlock()
		{
			WaitUnlocked();
			m_OwningThreadID = std::thread::id{};
			//wake up a waiting thread if there is one
			std::unique_lock<std::mutex> MyLock{m_Mutex};
			m_ConditionVariable.notify_all();
		}

		void push_back(std::shared_ptr<T> ref)
		{
			WaitUnlocked();
			m_Container.push_back(ref);
		}
		template<class... Args>
		decltype(auto) emplace_back(Args&&... args)
		{
			WaitUnlocked();
			return m_Container.emplace_back(std::make_shared<T>(std::forward<Args>(args)...));
		}
		std::shared_ptr<T> operator[](u64 index)
		{
			WaitUnlocked();
			//If this thread waits on a resource that has index 1000
			//but its waiting for a thread to finish a shrinking task that
			//would reduce the size of the vector to 900, we need to make sure to
			//tell that the desired data is no longer existing in memory
			return index < m_Container.size() ? m_Container[index] : nullptr;
		}
		std::shared_ptr<T> operator[](u64 index) const
		{
			WaitUnlocked();
			return index < m_Container.size() ? m_Container[index] : nullptr;
		}
		iterator erase(const_iterator iter)
		{
			WaitUnlocked();
			return m_Container.erase(iter);
		}
		iterator erase(const_iterator first, const_iterator last)
		{
			WaitUnlocked();
			return m_Container.erase(first, last);
		}
		void clear()
		{
			WaitUnlocked();
			m_Container.clear();
		}

		iterator begin()
		{
			WaitUnlocked();
			return m_Container.begin();
		}
		iterator end()
		{
			WaitUnlocked();
			return m_Container.end();
		}
		const_iterator begin() const
		{
			WaitUnlocked();
			return m_Container.begin();
		}
		const_iterator end() const
		{
			WaitUnlocked();
			return m_Container.end();
		}
		const_iterator cbegin() const { WaitUnlocked(); return begin(); }
		const_iterator cend() const { WaitUnlocked(); return end(); }

		std::shared_ptr<T> front() {
			WaitUnlocked();	
			return !m_Container.empty() ? m_Container[0] : nullptr;
		}
		std::shared_ptr<T> back() { 
			WaitUnlocked();
			return !m_Container.empty() ? m_Container[m_Container.size() - 1] : nullptr;
		}
		std::shared_ptr<T> front() const {
			WaitUnlocked();
			return !m_Container.empty() ? m_Container[0] : nullptr;
		}
		std::shared_ptr<T> back() const {
			WaitUnlocked();
			return !m_Container.empty() ? m_Container[m_Container.size() - 1] : nullptr;
		}

		u64 size() const
		{
			WaitUnlocked();
			return m_Container.size();
		}
		u64 capacity() const
		{
			WaitUnlocked();
			return m_Container.capacity();
		}
		bool empty() const
		{
			WaitUnlocked();
			return m_Container.empty();
		}

		void resize(u64 size)
		{
			WaitUnlocked();
			m_Container.resize(size);
		}
		void reserve(u64 size)
		{
			WaitUnlocked();
			m_Container.reserve(size);
		}

	private:
		inline void WaitUnlocked() const {
			if (m_OwningThreadID != std::thread::id{} && m_OwningThreadID != std::this_thread::get_id()) {
				std::unique_lock<std::mutex> MyLock{ m_Mutex };
				m_ConditionVariable.wait(MyLock, [this]() {return m_OwningThreadID == std::thread::id{}; });
			}
		}
		
	private:
		std::vector<std::shared_ptr<T>> m_Container;
		mutable std::atomic<std::thread::id> m_OwningThreadID;
		mutable std::mutex m_Mutex;
		mutable std::condition_variable m_ConditionVariable;
	};

	//Thread safe list (currently unused)
	template<class T>
	class TSList
	{
	public:
		TSList() {}
		~TSList() = default;

		TSList(const TSList&) = default;
		TSList(TSList&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) :
			m_Container(std::move(rhs.m_Container))
		{}

		TSList& operator=(const TSList&) = default;
		TSList& operator=(TSList&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>)
		{
			m_Container = std::move(rhs.m_Container);
			return *this;
		}

		void lock()
		{
			WaitUnlocked();
			m_OwningThreadID = std::this_thread::get_id();
		}
		void unlock()
		{
			WaitUnlocked();
			m_OwningThreadID = std::thread::id{};
		}

		void push_front(const T& p)
		{
			WaitUnlocked();
			m_Container.push_front(p);
		}
		void push_back(const T& p)
		{
			WaitUnlocked();
			m_Container.push_back(p);
		}
		template<class... Args>
		decltype(auto) emplace_back(Args&&... args)
		{
			WaitUnlocked();
			return m_Container.emplace_back(std::forward<Args>(args)...);
		}
		template<class... Args>
		decltype(auto) emplace_front(Args&&... args)
		{
			WaitUnlocked();
			return m_Container.emplace_front(std::forward<Args>(args)...);
		}

		const T& front()
		{
			WaitUnlocked();
			return m_Container.front();
		}
		const T& back()
		{
			WaitUnlocked();
			return m_Container.back();
		}
		T pop_front()
		{
			WaitUnlocked();
			T ret = m_Container.front();
			m_Container.pop_front();
			return ret;
		}
		T pop_back()
		{
			WaitUnlocked();
			T ret = m_Container.back();
			m_Container.pop_back();
			return ret;
		}
		void clear()
		{
			WaitUnlocked();
			m_Container.clear();
		}
		std::size_t size() const
		{
			WaitUnlocked();
			return m_Container.size();
		}
		bool empty() const
		{
			WaitUnlocked();
			return m_Container.empty();
		}

		void resize(std::size_t size)
		{
			WaitUnlocked();
			m_Container.resize(size);
		}


	private:
		void WaitUnlocked() 
		{
			MC_ASSERT(false, "This function is not yet implemented and should not be called");
		}
	private:
		std::list<T> m_Container;
		mutable std::atomic<std::thread::id> m_OwningThreadID;
	};

	static constexpr u32 g_PointerBytes = POINTER_BYTES;

	//Byte aligned pointer, allows class to adapt to the 8bit alignment
	//DOES NOT HANDLE NULL EXCEPTIONS
	template<class T>
	class AlignedPtr
	{
	public:
		AlignedPtr() { std::memset(m_Data, 0, g_PointerBytes); }
		AlignedPtr(const AlignedPtr&) = default;
		AlignedPtr(T* ptr) { operator=(ptr); }

		operator T* () { return Get(); }
		operator bool() { return Get() != nullptr; }

		T* operator->() { return Get(); }
		const T* operator->() const { return Get(); }

		T* Get()
		{
#ifdef ENV64
			u64 res;
#else
			u32 res;
#endif
			std::memcpy(&res, m_Data, g_PointerBytes);
			return reinterpret_cast<T*>(res);
		}
		const T* Get() const
		{
#ifdef ENV64
			u64 res;
#else
			u32 res;
#endif
			std::memcpy(&res, m_Data, g_PointerBytes);
			return reinterpret_cast<T*>(res);
		}
		AlignedPtr& operator=(const AlignedPtr& rhs) { std::memcpy(m_Data, rhs.m_Data, g_PointerBytes); return *this; }
		AlignedPtr& operator=(T* ptr)
		{
#ifdef ENV64
			u64 pointer_val = reinterpret_cast<u64>(ptr);
#else
			u32 pointer_val = reinterpret_cast<u32>(ptr);
#endif
			std::memcpy(m_Data, &pointer_val, g_PointerBytes);
			return *this;
		}
	private:
		u8 m_Data[g_PointerBytes];
	};

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
		Serializer(const std::string& filename, const char* mode)
		{
#ifdef _MSC_VER
			s32 er = fopen_s(&m_File, filename.c_str(), mode);
			MC_ASSERT(er == 0, "file buffer could not be opened");
#else
			m_File = fopen(filename.c_str(), mode);
			MC_ASSERT(m_File != nullptr, "file buffer could not be opened");
#endif
		}
		~Serializer()
		{
			if (!m_File)
				return;

			std::fclose(m_File);
		}

		Serializer(const Serializer&) = default;
		Serializer(Serializer&& rhs) noexcept
		{
			std::swap(m_File, rhs.m_File);
			rhs.m_File = nullptr;
		}

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

		void Seek(std::size_t pos) const
		{
			std::fseek(m_File, pos, SEEK_SET);
		}

		bool Eof() const
		{
			u32 cur_pos = std::ftell(m_File);
			std::fseek(m_File, 0, SEEK_END);
			bool is_eof = cur_pos == std::ftell(m_File);
			std::fseek(m_File, cur_pos, SEEK_SET);
			return is_eof;
		}

		u64 Tell() const
		{
			return std::ftell(m_File);
		}
		u64 FileSize() const
		{
			u64 pos = Tell();
			std::fseek(m_File, 0, SEEK_END);
			u64 ret_val = Tell();
			Seek(pos);
			return ret_val;
		}
		void Rewind()
		{
			Seek(0);
		}

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

	//Avoid using std::bitset because it occupies 4 bytes even for a few bits
	//
	//	00100010 01001000 ... 
	//  |				|
	//  |				|
	//  |				|
	//	bit 0			bit 15
	//
	//	Implicit big endian conversion performed to preserv continuity

	//Calculates a u8 array dimension for the given number of bits
	//In this project, helps with block normal serialization
	template<u32 Bits>
	class Bitfield
	{
		static_assert(Bits != 0);
	public:
		Bitfield() = default;
		Bitfield(const Bitfield&) = default;
		Bitfield(const std::array<u8, BYTE_INDEX_FOR_BITS(Bits)>& arr)
			:m_Data(arr)
		{
		}

		void Set(u32 pos, bool state)
		{
			if (pos >= Bits)
			{
				LOG_DEBUG("Invalid pos, nothing will change");
				return;
			}

			u8& cur_byte = m_Data[pos / 8];
			u32 wrapped_pos = 7 - (pos % 8);

			cur_byte = (cur_byte & ~(1 << wrapped_pos)) | (state << wrapped_pos);
		}
		bool Get(u32 pos) const
		{
			//If index over bounds, just return false
			if (pos >= Bits)
			{
				LOG_DEBUG("Invalid pos, false will be returned");
				return false;
			}

			const u8 cur_byte = m_Data[pos / 8];
			u32 wrapped_pos = 7 - (pos % 8);

			auto val = static_cast<u8>((cur_byte & (1 << wrapped_pos)) >> wrapped_pos);
			return val != 0;
		}
		u32 Size() const { return Bits; }
		bool operator[](u32 bit) const { return Get(bit); }

		u8 Getu8Payload(u32 index)
		{
			if (index >= m_Data.size())
				return 0;

			return m_Data[index];
		}

		bool AllOf(bool state) const
		{
			for (const u8 byte : m_Data)
				if (byte != static_cast<u8>(-state))
					return false;

			return true;
		}

	private:
		std::array<u8, BYTE_INDEX_FOR_BITS(Bits)> m_Data{};
	};

}

