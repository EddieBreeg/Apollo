#pragma once

#include <PCH.hpp>

#include "JsonFwd.hpp"
#include "TypeTraits.hpp"
#include <nlohmann/json.hpp>

namespace brk::json {
	template <class T>
	concept NativeJsonType = requires(T a, const T& b, nlohmann::json j)
	{
		{ j.get_to(a) };
		{ j = b };
	};

	template <class T, class K>
	bool Visit(T& out_obj, const nlohmann::json& j, K&& key, bool optional = false)
		requires(NativeJsonType<T>)
	{
		const auto it = j.find(std::forward<K>(key));
		if (it == j.end())
			return optional;

		it->get_to(out_obj);
		return true;
	}

	template <class T, class K>
	bool Visit(T& out_obj, const nlohmann::json& j, K&& key, bool optional = false) noexcept(
		noexcept(out_obj.FromJson(j))) requires(JsonEnabledType<T>)
	{
		const auto it = j.find(std::forward<K>(key));
		if (it == j.end())
			return optional;

		out_obj.FromJson(*it);
		return true;
	}

	template <class T, class K>
	static constexpr bool NoThrowVisitable = noexcept(
		Visit(std::declval<T&>(), std::declval<const nlohmann::json&>(), std::declval<K>()));

	template <class T>
	requires(HasJsonFieldList<T>) struct Converter<T>
	{
		static bool FromJson(T& out_obj, const nlohmann::json& j) noexcept(NoThrowFromJson)
		{
			return FromJsonImpl(
				out_obj,
				j,
				out_obj.JsonFields,
				std::make_index_sequence<T::JsonFields.Size>{});
		}

		static void ToJson(const T& obj, nlohmann::json& out_j)
		{
			ToJsonImpl(obj, out_j, obj.JsonFields, std::make_index_sequence<T::JsonFields.Size>{});
		}

	private:
		template <class List>
		struct ListTraits;

		template <auto... Ptr>
		struct ListTraits<const FieldList<Ptr...>>
		{
			static constexpr bool NoThrowFromJson =
				(NoThrowVisitable<
					 typename meta::MemberObjectTraits<decltype(Ptr)>::MemberType,
					 const char*> &&
				 ...);
		};

		static constexpr bool NoThrowFromJson = ListTraits<decltype(T::JsonFields)>::NoThrowFromJson;

		template <auto... M, size_t... I>
		static bool FromJsonImpl(
			T& out_obj,
			const nlohmann::json& j,
			const FieldList<M...>& fields,
			std::index_sequence<I...>) noexcept(NoThrowFromJson)
		{
			bool res = true;
			return (
				(res &= Visit(out_obj.*M, j, fields.Fields[I].m_Name, fields.Fields[I].m_IsOptional)),
				...);
		}

		template <auto... M, size_t... I>
		static void ToJsonImpl(
			const T& obj,
			nlohmann::json& out_json,
			const FieldList<M...>& fields,
			std::index_sequence<I...>)
		{
			((DumpMember(obj, out_json, fields.Fields[I].m_Name)), ...);
		}

		template <class M, T M::* Ptr>
		static void DumpMember(const T& obj, nlohmann::json& out_json, const char* name)
			requires(NativeJsonType<M> || JsonEnabledType<M>)
		{
			if constexpr (NativeJsonType<M>)
			{
				out_json[name] = obj.*Ptr;
			}
			else
			{
				nlohmann::json j;
				(obj.*Ptr).ToJson(j);
				out_json[name] = std::move(j);
			}
		}
	};
} // namespace brk::json