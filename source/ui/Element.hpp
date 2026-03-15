#pragma once

#include <PCH.hpp>
#include <clay.h>

namespace apollo::ui {
	[[nodiscard]] constexpr Clay_ElementId CreateId(Clay_String label, uint32 hash = 0)
	{
		for (const char* ptr = label.chars; ptr != (label.chars + label.length); ++ptr)
		{
			hash += *ptr;
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}

		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return Clay_ElementId{
			.id = hash + 1,
			.baseId = hash + 1,
			.stringId = label,
		};
	}

	namespace ui_literals {
		consteval Clay_String operator""_cstr(const char* str, size_t len) noexcept
		{
			return Clay_String{
				.isStaticallyAllocated = true,
				.length = static_cast<int32>(len),
				.chars = str,
			};
		}

		consteval Clay_ElementId operator""_eid(const char* label, size_t len) noexcept
		{
			return CreateId(
				Clay_String{
					.isStaticallyAllocated = true,
					.length = static_cast<int32>(len),
					.chars = label,
				},
				0);
		}
	} // namespace ui_literals

	/// Utility class to facilitate UI element declaration
	class Element
	{
	public:
		APOLLO_API Element(Clay_ElementId id, const Clay_ElementDeclaration& config);

		template <class E>
		Element& AddChild(Clay_ElementId id, const Clay_ElementDeclaration& config) noexcept(
			noexcept(E{ id, config }))
			requires(std::is_constructible_v<E, Clay_ElementId, const Clay_ElementDeclaration&>)
		{
			[[maybe_unused]] E child{ id, config };
			return *this;
		}

		template <class F>
		Element& AddBody(F&& func) noexcept(noexcept(func(*this)))
			requires(requires { func(*this); })
		{
			func(*this);
			return *this;
		}

		APOLLO_API ~Element();
	};
} // namespace apollo::ui