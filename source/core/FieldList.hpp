#pragma once

#include <PCH.hpp>

namespace brk::meta {
	/*
	 * Helper class to provide information about struct members
	 * \tparam FieldType: The type that will be stored in the list
	 * \tparam Members: A list of member pointers. All must point point to object members of a same
	 * type
	 */
	template <class FieldType, auto... Members>
	requires(std::is_member_object_pointer_v<decltype(Members)>&&...) struct FieldList
	{
		static constexpr size_t Size = sizeof...(Members);
		FieldType Fields[Size] = {};
	};

	/**
	 * Default struct field, which contains the member's name, size and alignment
	 */
	struct BasicField
	{
		const char* m_Name = nullptr;
		uint32 m_Size = 0;
		uint32 m_Alignment = 0;
	};

	template <class T, auto... M>
	using BasicFieldList = FieldList<BasicField, M...>;
} // namespace brk::meta