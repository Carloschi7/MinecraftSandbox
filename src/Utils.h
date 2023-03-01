#pragma once
#include <vector>
#include <list>
#include <mutex>
#include <chrono>
#include <fstream>
#include "Macros.h"

#ifndef BYTE_INDEX_FOR_BITS
	#define BYTE_INDEX_FOR_BITS(Bits) ((Bits - 1) / 8) + 1
#endif


#define _CHECK_UNLOCKED() while (m_OwningThreadID != std::this_thread::get_id() && m_OwningThreadID != std::thread::id{}) {}


//Utilities
namespace Utils
{
	//Thread safe vector
	//This class is pseudo-safe, locking a mutex each of the many accesses in the TSvector
	//can be really expensive, expecially with more limited hardware.
	//So the locking process is explicit and can be performed via the functions lock and unlock
	//This is done because in the scenario we find ourselves needing to lock the vector only if
	//we are adding/removing elements
	template<class T>
	class TSVector
	{
		using iterator = typename std::vector<T>::iterator;
		using const_iterator = typename std::vector<T>::const_iterator;
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
			_CHECK_UNLOCKED();
			m_OwningThreadID = std::this_thread::get_id();
		}
		void unlock()
		{
			_CHECK_UNLOCKED();
			m_OwningThreadID = std::thread::id{};
		}

		void push_back(const T& ref)
		{
			_CHECK_UNLOCKED();
			m_Container.push_back(ref);
		}
		template<class... Args>
		decltype(auto) emplace_back(Args&&... args)
		{
			_CHECK_UNLOCKED();
			return m_Container.emplace_back(std::forward<Args>(args)...);
		}
		T& operator[](std::size_t index)
		{
			_CHECK_UNLOCKED();
			return m_Container[index];
		}
		const T& operator[](std::size_t index) const
		{
			_CHECK_UNLOCKED();
			return m_Container[index];
		}
		iterator erase(const_iterator iter)
		{
			_CHECK_UNLOCKED();
			return m_Container.erase(iter);
		}
		iterator erase(const_iterator first, const_iterator last)
		{
			_CHECK_UNLOCKED();
			return m_Container.erase(first, last);
		}
		void clear()
		{
			_CHECK_UNLOCKED();
			m_Container.clear();
		}

		iterator begin()
		{
			_CHECK_UNLOCKED();
			return m_Container.begin();
		}
		iterator end()
		{
			_CHECK_UNLOCKED();
			return m_Container.end();
		}
		const_iterator begin() const
		{
			_CHECK_UNLOCKED();
			return m_Container.begin();
		}
		const_iterator end() const
		{
			_CHECK_UNLOCKED();
			return m_Container.end();
		}
		const_iterator cbegin() const { _CHECK_UNLOCKED(); return begin(); }
		const_iterator cend() const { _CHECK_UNLOCKED(); return end(); }

		T& front() { _CHECK_UNLOCKED(); return m_Container[0]; }
		T& back() { _CHECK_UNLOCKED(); return m_Container[m_Container.size() - 1]; }
		const T& front() const { _CHECK_UNLOCKED(); return m_Container[0]; }
		const T& back() const { _CHECK_UNLOCKED(); return m_Container[m_Container.size() - 1]; }

		std::size_t size() const
		{
			_CHECK_UNLOCKED();
			return m_Container.size();
		}
		std::size_t capacity() const
		{
			_CHECK_UNLOCKED();
			return m_Container.capacity();
		}
		bool empty() const
		{
			_CHECK_UNLOCKED();
			return m_Container.empty();
		}

		void resize(std::size_t size)
		{
			_CHECK_UNLOCKED();
			m_Container.resize(size);
		}
		void reserve(std::size_t size)
		{
			_CHECK_UNLOCKED();
			m_Container.reserve(size);
		}

	private:
		std::vector<T> m_Container;
		mutable std::atomic<std::thread::id> m_OwningThreadID;
	};

	//Thread safe list
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

		void push_front(const T& p)
		{
			std::scoped_lock lk(m_Lock);
			m_Container.push_front(p);
		}
		void push_back(const T& p)
		{
			std::scoped_lock lk(m_Lock);
			m_Container.push_back(p);
		}
		const T& front()
		{
			std::scoped_lock lk(m_Lock);
			return m_Container.front();
		}
		const T& back()
		{
			std::scoped_lock lk(m_Lock);
			return m_Container.back();
		}
		T pop_front()
		{
			std::scoped_lock lk(m_Lock);
			T ret = m_Container.front();
			m_Container.pop_front();
			return ret;
		}
		T pop_back()
		{
			std::scoped_lock lk(m_Lock);
			T ret = m_Container.back();
			m_Container.pop_back();
			return ret;
		}
		void clear()
		{
			std::scoped_lock lk(m_Lock);
			m_Container.clear();
		}
		std::size_t size() const
		{
			return m_Container.size();
		}
		bool empty() const
		{
			return m_Container.empty();
		}

	private:
		std::list<T> m_Container;
		mutable std::mutex m_Lock;
	};

	static constexpr uint32_t g_PointerBytes = POINTER_BYTES;

	//Byte aligned pointer, allows class to adapt to the 8bit alignment
	//DOES NOT HANDLE NULL EXCEPTIONS
	template<class T>
	class AlignedPtr
	{
	public:
		AlignedPtr() { std::memset(m_Data, 0, g_PointerBytes); }
		AlignedPtr(const AlignedPtr&) = default;
		AlignedPtr(T* ptr) { operator=(ptr); }
		
		operator T*() { return Get(); }
		operator bool() { return Get() != nullptr; }

		T* operator->() { return Get(); }
		const T* operator->() const { return Get(); }

		T* Get() 
		{
#ifdef ENV64
			uint64_t res;
#else
			uint32_t res;
#endif
			std::memcpy(&res, m_Data, g_PointerBytes);
			return reinterpret_cast<T*>(res);
		}
		const T* Get() const 
		{
#ifdef ENV64
			uint64_t res;
#else
			uint32_t res;
#endif
			std::memcpy(&res, m_Data, g_PointerBytes);
			return reinterpret_cast<T*>(res);
		}
		AlignedPtr& operator=(const AlignedPtr& rhs) { std::memcpy(m_Data, rhs.m_Data, g_PointerBytes); return *this; }
		AlignedPtr& operator=(T* ptr) 
		{
#ifdef ENV64
			uint64_t pointer_val = reinterpret_cast<uint64_t>(ptr);
#else
			uint32_t pointer_val = reinterpret_cast<uint32_t>(ptr);
#endif
			std::memcpy(m_Data, &pointer_val, g_PointerBytes); 
			return *this;
		}
	private:
		uint8_t m_Data[g_PointerBytes];
	};

	class Timer
	{
	public:
		Timer() = default;
		void StartTimer() { m_TimePoint = std::chrono::steady_clock::now(); }
		float GetElapsedMilliseconds() const
		{
			return GetElapsedSeconds() * 1000.0f;
		}
		float GetElapsedSeconds() const
		{
			auto now = std::chrono::steady_clock::now();
			return std::chrono::duration<float>(now - m_TimePoint).count();
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
			m_File = std::fopen(filename.c_str(), mode);
		}
		~Serializer()
		{
			std::fclose(m_File);
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
			uint32_t cur_pos = std::ftell(m_File);
			std::fseek(m_File, 0, SEEK_END);
			bool is_eof = cur_pos == std::ftell(m_File);
			std::fseek(m_File, cur_pos, SEEK_SET);
			return is_eof;
		}

		std::size_t Tell() const
		{
			return std::ftell(m_File);
		}
		std::size_t FileSize() const
		{
			auto pos = Tell();
			std::fseek(m_File, 0, SEEK_END);
			auto ret_val = Tell();
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
	template<uint32_t Bits>
	class Bitfield
	{
		static_assert(Bits != 0);
	public:
		Bitfield() = default;
		Bitfield(const Bitfield&) = default;
		Bitfield(const std::array<uint8_t, BYTE_INDEX_FOR_BITS(Bits)>& arr)
			:m_Data(arr)
		{
		}

		void Set(uint32_t pos, bool state)
		{
			if (pos >= Bits)
			{
				LOG_DEBUG("Invalid pos, nothing will change");
				return;
			}

			uint8_t& cur_byte = m_Data[pos / 8];
			uint32_t wrapped_pos = 7 - (pos % 8);

			cur_byte = (cur_byte & ~(1 << wrapped_pos)) | (state << wrapped_pos);
		}
		bool Get(uint32_t pos) const
		{
			//If index over bounds, just return false
			if (pos >= Bits)
			{
				LOG_DEBUG("Invalid pos, false will be returned");
				return false;
			}

			const uint8_t& cur_byte = m_Data[pos / 8];
			uint32_t wrapped_pos = 7 - (pos % 8);

			auto val = static_cast<uint8_t>((cur_byte & (1 << wrapped_pos)) >> wrapped_pos);
			return val != 0;
		}
		uint32_t Size() const { return Bits; }
		bool operator[](uint32_t bit) const { return Get(bit); }

		uint8_t Getu8Payload(uint32_t index)
		{
			if (index >= m_Data.size())
				return 0;

			return m_Data[index];
		}

		bool AllOf(bool state) const
		{
			for (const uint8_t& byte : m_Data)
				if (byte != static_cast<uint8_t>(-state))
					return false;

			return true;
		}

	private:
		std::array<uint8_t, BYTE_INDEX_FOR_BITS(Bits)> m_Data{};
	};
}
