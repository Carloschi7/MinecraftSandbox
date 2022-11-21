#pragma once
#include <vector>
#include <list>
#include <mutex>

//Assert if the environment is x32 or x64(always 64 on linux)
#if defined _WIN64 || defined _WIN32
#	ifdef _WIN64
#		define ENV64
#	else
#		define ENV32
#	endif
#elif defined __GNUC__
#	ifdef __x86_64__ || __ppc64__
#		define ENV64
#	else
#		define ENV32
#	endif
#endif

#ifdef __linux__
#	define ENV64
#endif

#ifndef ENV64
#	define ENV32
#endif

#ifdef ENV64
#	define POINTER_BYTES 8
#else
#	define POINTER_BYTES 4
#endif

//Utilities

namespace Utils
{
	//Thread safe vector

	//Implementing only the used methods
	//the methods here will have the same format as the ones in the standard lib 
	//for example "push_back", because the container we choose depends on the
	//app being multithreaded or not. The same goes for TSList
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

		void push_back(const T& ref)
		{
			m_Container.push_back(ref);
		}
		template<class... Args>
		decltype(auto) emplace_back(Args&&... args)
		{
			return m_Container.emplace_back(std::forward<Args>(args)...);
		}
		T& operator[](std::size_t index)
		{
			std::scoped_lock lk(m_Mutex);
			return m_Container[index];
		}
		const T& operator[](std::size_t index) const
		{
			std::scoped_lock lk(m_Mutex);
			return m_Container[index];
		}
		void erase(const_iterator iter)
		{
			std::scoped_lock lk(m_Mutex);
			m_Container.erase(iter);
		}

		iterator begin()
		{
			return m_Container.begin();
		}
		iterator end()
		{
			return m_Container.end();
		}
		const_iterator begin() const
		{
			return m_Container.begin();
		}
		const_iterator end() const
		{
			return m_Container.end();
		}
		const_iterator cbegin() const { return begin(); }
		const_iterator cend() const { return end(); }

		T& front() {  return m_Container[0]; }
		T& back() {  return m_Container[m_Container.size() - 1]; }
		const T& front() const {  return m_Container[0]; }
		const T& back() const {  return m_Container[m_Container.size() - 1]; }

		std::size_t size() const
		{
			return m_Container.size();
		}
		bool empty() const
		{
			return m_Container.empty();
		}
	private:
		std::vector<T> m_Container;
		mutable std::mutex m_Mutex;
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
		std::size_t size()
		{
			std::scoped_lock lk(m_Lock);
			return m_Container.size();
		}
		bool empty() const
		{
			std::scoped_lock lk(m_Lock);
			return m_Container.empty();
		}

	private:
		std::list<T> m_Container;
		mutable std::mutex m_Lock;
	};

	//For now TSVector is disabled, mutexes soak up a lot of performance
	template<class T, bool _Cond>
	using ConditionalVector = typename std::conditional<_Cond, std::vector<T>, std::vector<T>>::type;
	template<class T, bool _Cond>
	using ConditionalList = typename std::conditional<_Cond, Utils::TSList<T>, std::list<T>>::type;

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
		operator T* () { return Get(); }
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
}