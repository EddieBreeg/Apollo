#include "ULID.hpp"
#include "RNG.hpp"
#include <ctime>
#include <nlohmann/json.hpp>

namespace {
	thread_local apollo::RNG g_Generator;
}

namespace apollo {
	apollo::ULID apollo::ULID::Generate()
	{
		std::timespec ts;
		std::timespec_get(&ts, TIME_UTC);
		ULID res;
		res.m_Left = static_cast<uint64>(ts.tv_sec) * 1'000 + ts.tv_nsec / 1'000'000;
		res.m_Left = (res.m_Left << 16) | (g_Generator() & 0xffff);
		res.m_Right = g_Generator();

		return res;
	}

	bool ULID::FromJson(const nlohmann::json& json) noexcept
	{
		if (!json.is_string())
			return false;

		std::string_view str;
		json.get_to(str);
		if (str.length() != 26)
			return false;

		*this = FromString(str);
		return true;
	}

	void ULID::ToJson(nlohmann::json& out_j) const
	{
		char buf[26];
		ToChars(buf);
		out_j = std::string_view{ buf, 26 };
	}
} // namespace apollo