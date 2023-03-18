#pragma once

//Assert if the environment is x32 or x64(always 64 on linux)
#if defined _WIN64 || defined _WIN32
#	ifdef _WIN64
#		define ENV64
#	else
#		define ENV32
#	endif
#elif defined __GNUC__
#	if defined __x86_64__ || defined __ppc64__
#		define ENV64
#	else
#		define ENV32
#	endif
#endif

//Determine if the app is multithreadable
#define MC_MULTITHREADING

#ifdef MC_MULTITHREADING
#	define MC_CHUNK_SIZE m_SafeChunkSize
#else
#	define MC_CHUNK_SIZE m_Chunks.size()
#endif

//Generic linux platforms
#if defined __linux__ && !defined ENV64
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

//Windows x64 release does not seem to have issues even without using locks
//between threads upon pushing heap reallocations on containers
//Trying to provide stronger thread safety on configurations who neeed it more

#if defined _DEBUG
#	define STRONG_THREAD_SAFETY
#endif

#ifdef _DEBUG
#	define LOG_DEBUG(fmt, ...) std::printf(fmt, __VA_ARGS__);
#else
#	define LOG_DEBUG(fmt, ...)
#endif

#ifdef _DEBUG
#	define DEBUG_MODE
#endif