#pragma once

#include <PCH.hpp>

namespace brk::meta {
	template <class FieldType, auto... Members>
	struct FieldList;

	/*
	 * Helper class to provide information about struct members
	 * \tparam FieldType: The type that will be stored in the list
	 * \tparam Members: A list of member pointers. All must point point to object members of a same
	 * type
	 */
	template <class FieldType, class T, class... M, M T::*... Members>
	requires(std::is_object_v<M>&&...) struct FieldList<FieldType, Members...>
	{
		using ClassType = T;
		static constexpr size_t Size = sizeof...(M);
		FieldType Fields[Size] = {};
	};

	struct BasicField
	{
		const char* m_Name = nullptr;
		uint32 m_Size = 0;
		uint32 m_Alignment = 0;
	};

	template<class T, auto... M>
	using BasicFieldList = FieldList<BasicField, M...>;
} // namespace brk::meta