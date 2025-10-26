#pragma once

#include <PCH.hpp>

#include "JsonFwd.hpp"
#include "TypeTraits.hpp"
#include <glm/detail/qualifier.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

namespace apollo::json {
	template <class T>
	concept NativeJsonType = requires(T a, const T& b, nlohmann::json j)
	{
		{ j.get_to(a) };
		{ j = b };
	};

	namespace _internal {
		template <NativeJsonType T, class = void>
		struct ValueType;

		template <>
		struct ValueType<std::string_view>
		{
			static constexpr auto Type = nlohmann::json::value_t::string;
		};
		template <>
		struct ValueType<std::string>
		{
			static constexpr auto Type = nlohmann::json::value_t::string;
		};
		template <>
		struct ValueType<std::filesystem::path>
		{
			static constexpr auto Type = nlohmann::json::value_t::string;
		};

		template <class Float>
		struct ValueType<Float, std::enable_if_t<std::is_floating_point_v<Float>>>
		{
			static constexpr auto Type = nlohmann::json::value_t::number_float;
		};

		template <class T, size_t N>
		struct ValueType<T[N]>
		{
			static constexpr auto Type = nlohmann::json::value_t::array;
		};

		template <class T>
		struct ValueType<std::vector<T>>
		{
			static constexpr auto Type = nlohmann::json::value_t::array;
		};

		template <class T, class K, class Hasher, class KeyEq, class Alloc>
		struct ValueType<std::unordered_map<T, K, Hasher, KeyEq, Alloc>>
		{
			static constexpr auto Type = nlohmann::json::value_t::object;
		};

		template <class T, class K, class LessThan, class Alloc>
		struct ValueType<std::map<T, K, LessThan, Alloc>>
		{
			static constexpr auto Type = nlohmann::json::value_t::object;
		};
	} // namespace _internal

	/**
	 * Looks for a specific key in a json object, and attempts to convert the associated value to a
	 * C++ object if it was found. This overload is only valid for type which are natively
	 * recognized by nlohmann::json
	 * \param out_obj: The destination object
	 * \param j: The JSON object to look into
	 * \param key: The key to look for
	 * \param optional: The value which gets returned if key was not found
	 * \returns true if the object was successfully converted, false otherwise
	 */
	template <class T, class K>
	bool Visit(T& out_obj, const nlohmann::json& j, K&& key, bool optional = false)
		requires(NativeJsonType<T>)
	{
		const auto it = j.find(std::forward<K>(key));
		if (it == j.end())
			return optional;

		if constexpr (std::is_enum_v<T>)
		{
			if (!it->is_number_integer() && !it->is_string())
				return false;
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			if (!it->is_boolean() && !it->is_number_integer())
				return false;
		}
		else if constexpr (std::is_integral_v<T>)
		{
			if (!it->is_number_integer())
				return false;
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			if (!it->is_number())
				return false;
		}
		else if constexpr (!std::is_same_v<T, nlohmann::json>)
		{
			if (it->type() != _internal::ValueType<T>::Type)
				return false;
		}

		it->get_to(out_obj);
		return true;
	}

	/**
	 * Looks for a specific key in a json object, and attempts to convert the associated value to a
	 * C++ object if it was found. This overload is only valid for types which meet the requirements
	 * for the JsonEnabledType concept
	 * \param out_obj: The destination object
	 * \param j: The JSON object to look into
	 * \param key: The key to look for
	 * \param optional: The value which gets returned if key was not found
	 * \returns true if the object was successfully converted, false otherwise
	 */
	template <JsonEnabledType T, class K>
	bool Visit(T& out_obj, const nlohmann::json& j, K&& key, bool optional = false) noexcept(
		noexcept(out_obj.FromJson(j)))
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

	/**
	 * Default JSON Converter implementation. This specialisation is valid for any type which meets
	 * the requirements of the HasJsonFieldList concept, provided the fields in the list are
	 * themselves JSON compatible.
	 */
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

	template <class T, int L>
	struct Converter<glm::vec<L, T>>
	{
		using VecType = glm::vec<L, T>;

		static bool FromJson(VecType& out_vec, const nlohmann::json& j) noexcept
		{
			if (!Visit(out_vec.x, j, "x"))
				return false;
			if constexpr (L >= 2)
			{
				if (!Visit(out_vec.y, j, "y"))
					return false;
			}
			if constexpr (L >= 3)
			{
				if (!Visit(out_vec.z, j, "z"))
					return false;
			}
			if constexpr (L >= 4)
			{
				if (!Visit(out_vec.w, j, "w"))
					return false;
			}
			return true;
		}

		static void ToJson(const VecType& vec, nlohmann::json& out_json)
		{
			out_json["x"] = vec.x;
			if constexpr (L >= 2)
				out_json["y"] = vec.y;
			if constexpr (L >= 3)
				out_json["z"] = vec.z;
			if constexpr (L >= 4)
				out_json["w"] = vec.w;
		}
	};

	template <>
	struct Converter<glm::quat>
	{
		static bool FromJson(glm::quat& out_q, const nlohmann::json& j) noexcept
		{
			return Visit(out_q.x, j, "x") && Visit(out_q.y, j, "y") && Visit(out_q.z, j, "z") &&
				   Visit(out_q.w, j, "w");
		}
	};

	template <class T>
	struct Converter<Rectangle<T>>
	{
		static bool FromJson(Rectangle<T>& out_rect, const nlohmann::json& json) noexcept
		{
			return Visit(out_rect.x0, json, "x0") && Visit(out_rect.x1, json, "x1") &&
				   Visit(out_rect.y0, json, "y0") && Visit(out_rect.y1, json, "y1");
		}

		static void ToJson(const Rectangle<T>& rect, nlohmann::json& out_json)
		{
			out_json["x0"] = rect.x0;
			out_json["x1"] = rect.x1;
			out_json["y0"] = rect.y0;
			out_json["y1"] = rect.y1;
		}
	};

	template <class T, class K>
	bool Visit(T& out_obj, const nlohmann::json& json, K&& key, bool isOptional = false)
		requires(std::convertible_to<decltype(Converter<T>::FromJson(out_obj, json[key])), bool>)
	{
		const auto it = json.find(std::forward<K>(key));
		if (it == json.end())
			return isOptional;
		return Converter<T>::FromJson(out_obj, *it);
	}
} // namespace apollo::json

namespace glm {
	template <int L, class T>
	void to_json(nlohmann::json& out_json, const vec<L, T>& v)
	{
		apollo::json::Converter<vec<L, T>>::ToJson(v, out_json);
	}
} // namespace glm

namespace apollo {
	template <class T>
	void to_json(nlohmann::json& out_json, const Rectangle<T>& rect)
	{
		apollo::json::Converter<Rectangle<T>>::ToJson(rect, out_json);
	}
} // namespace apollo