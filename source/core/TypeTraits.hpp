#pragma once

#include <PCH.hpp>

#include "TypeList.hpp"

/** \file TypeTraits.hpp */

/**
 * \namespace apollo::meta
 * \brief Metaprogramming library
 */
namespace apollo::meta {
	template <class T>
	concept NoThrowMovable = std::is_nothrow_move_constructible_v<T> &&
							 std::is_nothrow_move_assignable_v<T>;

	template <class T>
	struct MemberObjectTraits;

	/**
	 * \brief Provides information on a member object pointer type
	 */
	template <class T, class M>
	requires(std::is_object_v<M>) struct MemberObjectTraits<M T::*>
	{
		using ClassType = T;
		using MemberType = M;
	};

	namespace _internal {
		template <class T, class = void>
		struct IsCompleteT : std::false_type
		{};
		template <class T>
		struct IsCompleteT<T, std::void_t<decltype(sizeof(T))>> : std::true_type
		{};
	} // namespace _internal

	/** \brief Evaluates whether \b T is a complete type */
	template <class T>
	static constexpr bool IsComplete = _internal::IsCompleteT<T>::value;

	/**
	 * \name FunctionTraits
	 * \brief Meta-information for function types. This works for all function-like types, including
	 * lambdas.
	 * @{ */
	template <class F>
	struct FunctionTraits;

	template <class R, class... Args>
	struct FunctionTraits<R(Args...)>
	{
		using ReturnType = R;
		using ArgList = TypeList<Args...>;
		static constexpr bool IsNoexcept = false;
	};

	template <class R, class... Args>
	struct FunctionTraits<R(Args...) noexcept>
	{
		using ReturnType = R;
		using ArgList = TypeList<Args...>;
		static constexpr bool IsNoexcept = true;
	};

	template <class C, class R, class... Args>
	struct FunctionTraits<R (C::*)(Args...)> : public FunctionTraits<R(Args...)>
	{
		using ClassType = C;
		static constexpr bool IsConst = false;
	};
	template <class C, class R, class... Args>
	struct FunctionTraits<R (C::*)(Args...) const> : public FunctionTraits<R(Args...)>
	{
		using ClassType = C;
		static constexpr bool IsConst = true;
	};
	template <class C, class R, class... Args>
	struct FunctionTraits<R (C::*)(Args...) noexcept> : public FunctionTraits<R(Args...) noexcept>
	{
		using ClassType = C;
		static constexpr bool IsConst = false;
	};
	template <class C, class R, class... Args>
	struct FunctionTraits<R (C::*)(Args...) const noexcept>
		: public FunctionTraits<R(Args...) noexcept>
	{
		using ClassType = C;
		static constexpr bool IsConst = true;
	};
	template <class F>
	requires(std::is_member_function_pointer_v<decltype(&F::operator())>) struct FunctionTraits<F>
		: public FunctionTraits<decltype(&F::operator())>
	{};
	/** @} */

	template <class F>
	concept Function = IsComplete<FunctionTraits<F>>;

	template <class T>
	concept Trivial = std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>;

	template <class T>
	concept SmallTrivial = Trivial<T> && (sizeof(T) <= (2 * sizeof(void*)));
} // namespace apollo::meta