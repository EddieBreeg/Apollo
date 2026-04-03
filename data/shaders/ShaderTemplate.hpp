#pragma once

#include <slang-com-helper.h>
#include <slang.h>

#ifndef STATIC_STRING_DECL
#define STATIC_STRING_DECL
namespace {
	template <size_t N>
	struct StaticString final : public slang::IBlob
	{
		constexpr StaticString(const char (&str)[N])
		{
			char* it = m_Buf;
			for (const char c : str)
				*it++ = c;
		}
		SLANG_IUNKNOWN_ALL

		void const* SLANG_MCALL getBufferPointer() noexcept override { return m_Buf; }
		size_t SLANG_MCALL getBufferSize() noexcept override { return N - 1; }

	private:
		ISlangUnknown* getInterface(const SlangUUID& id) noexcept
		{
			return id == ISlangBlob::getTypeGuid() ? this : nullptr;
		}

		uint32 m_refCount = 1;
		char m_Buf[N];
	};
} // namespace
#endif

namespace apollo::data::shaders {
	// clang-format off
	static inline StaticString @VARNAME@ = R"slang(@SLANG_SOURCE@)slang";
	// clang-format on
} // namespace apollo::data::shaders