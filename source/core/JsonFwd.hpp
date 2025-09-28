#pragma once

#include <PCH.hpp>

#include "FieldList.hpp"
#include <nlohmann/json_fwd.hpp>

namespace brk::json {
	struct Field
	{
		const char* m_Name = nullptr;
		bool m_IsOptional = false;
	};

	template <auto... M>
	using FieldList = meta::FieldList<Field, M...>;

	namespace _internal {
		template <class>
		struct IsFieldList : std::false_type
		{};

		template <class T, class... M, M T::*... Ptr>
		struct IsFieldList<FieldList<Ptr...>> : std::true_type
		{};

		template <class T>
		struct IsFieldList<const T> : IsFieldList<T>
		{};
	} // namespace _internal

	template <class T>
	concept HasJsonFieldList = _internal::IsFieldList<decltype(T::JsonFields)>::value;

	template <class T>
	concept JsonEnabledType = requires(T a, const T b, const nlohmann::json& j1, nlohmann::json& j2)
	{
		{ a.FromJson(j1) }->std::convertible_to<bool>;
		{ b.ToJson(j2) };
	};

	template <class T>
	struct Converter;

	template <class T>
	concept JsonConvertible = requires(T a, const T& b, const nlohmann::json& j1, nlohmann::json& j2)
	{
		{ Converter<T>::FromJson(a, j1) }->std::convertible_to<bool>;
		{ Converter<T>::ToJson(b, j2) };
	};
} // namespace brk::json