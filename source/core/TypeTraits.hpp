#pragma once

#include <PCH.hpp>

#include "TypeList.hpp"

namespace apollo::meta {
	template <class T>
	concept NoThrowMovable = std::is_nothrow_move_constructible_v<T> &&
							 std::is_nothrow_move_assignable_v<T>;

	template <class T>
	struct MemberObjectTraits;

	template <class T, class M>
	struct MemberObjectTraits<M T::*>
	{
		using ClassType = T;
		using MemberType = M;
	};

	template <class T, class = void>
	struct IsCompleteT : std::false_type
	{};
	template <class T>
	struct IsCompleteT<T, std::void_t<decltype(sizeof(T))>> : std::true_type
	{};

	template <class T>
	static constexpr bool IsComplete = IsCompleteT<T>::value;

	template <class F>
	struct FunctionTraits;

	template <class F>
	concept Function = IsComplete<FunctionTraits<F>>;

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
} // namespace apollo::meta