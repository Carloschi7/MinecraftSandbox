#include "Utils.h"

namespace Utils
{
	std::string CompletePath(const char* str)
	{
#ifdef MC_SOURCE_PATH
		return std::string(MC_SOURCE_PATH) + str;
#else
		static_assert(false, "this macro should be defined");
		return {};
#endif
	}

	u64 ChunkHasher(const glm::vec2& val)
	{
		static_assert(sizeof(glm::vec2) == sizeof(u64), "The implementation requires this expression to be true");
		u64 res;
		std::memcpy(&res, &val, sizeof(u64));
		return res;
	}

	f32 PerspectiveItemRotation(f32 fov_degrees, bool is_block)
	{
		//These constants seem to work
		constexpr f32 block_ratio = 45.0f / 70.0f;
		constexpr f32 item_ratio = 30.0f / 70.0f;

		return is_block ? glm::radians(fov_degrees * block_ratio) : glm::radians(fov_degrees * item_ratio);
	}


	Serializer::Serializer(const std::string& filename, const char* mode)
	{
#ifdef _MSC_VER
		s32 er = fopen_s(&m_File, filename.c_str(), mode);
		MC_ASSERT(er == 0, "file buffer could not be opened");
#else
		m_File = fopen(filename.c_str(), mode);
		MC_ASSERT(m_File != nullptr, "file buffer could not be opened");
#endif
	}

	Serializer::Serializer(Serializer&& rhs) noexcept
	{
		std::swap(m_File, rhs.m_File);
		rhs.m_File = nullptr;
	}

	Serializer::~Serializer()
	{
		if (!m_File)
			return;

		std::fclose(m_File);
	}

	void Serializer::Seek(std::size_t pos) const
	{
		std::fseek(m_File, pos, SEEK_SET);
	}

	bool Serializer::Eof() const
	{
		u32 cur_pos = Tell();
		std::fseek(m_File, 0, SEEK_END);
		bool is_eof = cur_pos == Tell();
		std::fseek(m_File, cur_pos, SEEK_SET);
		return is_eof;
	}

	u64 Serializer::Tell() const
	{
		return std::ftell(m_File);
	}
	u64 Serializer::FileSize() const
	{
		u64 pos = Tell();
		std::fseek(m_File, 0, SEEK_END);
		u64 ret_val = Tell();
		Seek(pos);
		return ret_val;
	}
	void Serializer::Rewind()
	{
		Seek(0);
	}

}
