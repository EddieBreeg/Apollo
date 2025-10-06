#pragma once

#include <PCH.hpp>

#include "FieldList.hpp"
#include <nlohmann/json_fwd.hpp>

/**
 * \namespace json
 * Contains all json-related code for converting objects to/from json
 */
namespace brk::json {
	/**
	 * Represents a JSON structure field
	 */
	struct Field
	{
		// This is used as they JSON key
		const char* m_Name = nullptr;
		/* This informs the behaviour when the key is not found in the json object:
		 * if false, attempting to load with field will result in an error if the key wasn't found
		 * in the JSON object. If true, loading this field will succeed even when the key wasn't
		 * found, which is useful if the struct member has a valid default value to fall back on
		 */
		bool m_IsOptional = false;
	};

	template <auto... M>
	using FieldList = meta::FieldList<Field, M...>;

	namespace _internal {
		template <class>
		struct IsFieldList : std::false_type
		{};

		template <class T>
		struct IsFieldList<const T> : IsFieldList<T>
		{};

		template <auto... Ptr>
		struct IsFieldList<FieldList<Ptr...>> : std::true_type
		{};
	} // namespace _internal

	/**
	 * Evaluates to true if T declared an accessible static FieldList object named JsonFields
	 */
	template <class T>
	concept HasJsonFieldList = _internal::IsFieldList<decltype(T::JsonFields)>::value;

	/**
	 * This concept is used to detect when a struct/class type implements JSON conversion methods
	 */
	template <class T>
	concept JsonEnabledType = requires(T a, const T b, const nlohmann::json& j1, nlohmann::json& j2)
	{
		{ a.FromJson(j1) }->std::convertible_to<bool>;
		{ b.ToJson(j2) };
	};

	/**
	 * JSON Converter. This is used to convert complex objects from/to JSON. A default
	 * implementation is available in Json.hpp
	 */
	template <class T>
	struct Converter;

	/**
	 * Used to detect whether a type has a valid specialisation of Converter available
	 */
	template <class T>
	concept JsonConvertible = requires(T a, const T& b, const nlohmann::json& j1, nlohmann::json& j2)
	{
		{ Converter<T>::FromJson(a, j1) }->std::convertible_to<bool>;
		{ Converter<T>::ToJson(b, j2) };
	};
} // namespace brk::json