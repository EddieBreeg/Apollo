#include <core/TypeTraits.hpp>

namespace apollo::meta {
	static_assert(IsComplete<int>);
	static_assert(!IsComplete<void>);

	struct Incomplete1;
	static_assert(!IsComplete<Incomplete1>);

	namespace function_ut {
		static_assert(Function<void()>);
		static_assert(Function<int()>);
		static_assert(Function<int(float)>);
		static_assert(Function<void() noexcept>);
		static_assert(Function<int() noexcept>);
		static_assert(Function<int(float) noexcept>);

		using T1 = FunctionTraits<void()>;
		using T2 = FunctionTraits<int()>;
		using T3 = FunctionTraits<int(float)>;
		using T4 = FunctionTraits<void() noexcept>;
		using T5 = FunctionTraits<int() noexcept>;
		using T6 = FunctionTraits<int(float) noexcept>;

		static_assert(std::is_same_v<T1::ReturnType, void>);
		static_assert(std::is_same_v<T2::ReturnType, int>);
		static_assert(std::is_same_v<T3::ReturnType, int>);
		static_assert(std::is_same_v<T4::ReturnType, void>);
		static_assert(std::is_same_v<T5::ReturnType, int>);
		static_assert(std::is_same_v<T6::ReturnType, int>);

		static_assert(std::is_same_v<T1::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T2::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T3::ArgList, TypeList<float>>);
		static_assert(std::is_same_v<T4::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T5::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T6::ArgList, TypeList<float>>);

		static_assert(!T1::IsNoexcept);
		static_assert(!T2::IsNoexcept);
		static_assert(!T3::IsNoexcept);
		static_assert(T4::IsNoexcept);
		static_assert(T5::IsNoexcept);
		static_assert(T6::IsNoexcept);

		struct F1
		{
			void operator()();
		};
		struct F2
		{
			void operator()(int) const;
		};
		struct F3
		{
			int operator()(float) noexcept;
		};
		struct F4
		{
			int operator()(void) const noexcept;
		};

		using F1Traits = FunctionTraits<F1>;
		using F2Traits = FunctionTraits<F2>;
		using F3Traits = FunctionTraits<F3>;
		using F4Traits = FunctionTraits<F4>;

		static_assert(std::is_same_v<F1Traits::ReturnType, void>);
		static_assert(std::is_same_v<F1Traits::ArgList, TypeList<>>);
		static_assert(!F1Traits::IsConst);
		static_assert(!F1Traits::IsNoexcept);

		static_assert(std::is_same_v<F2Traits::ReturnType, void>);
		static_assert(std::is_same_v<F2Traits::ArgList, TypeList<int>>);
		static_assert(F2Traits::IsConst);
		static_assert(!F2Traits::IsNoexcept);

		static_assert(std::is_same_v<F3Traits::ReturnType, int>);
		static_assert(std::is_same_v<F3Traits::ArgList, TypeList<float>>);
		static_assert(!F3Traits::IsConst);
		static_assert(F3Traits::IsNoexcept);

		static_assert(std::is_same_v<F4Traits::ReturnType, int>);
		static_assert(std::is_same_v<F4Traits::ArgList, TypeList<>>);
		static_assert(F4Traits::IsConst);
		static_assert(F4Traits::IsNoexcept);

		using T7 = FunctionTraits<void (F1::*)()>;
		using T8 = FunctionTraits<int (F1::*)()>;
		using T9 = FunctionTraits<int (F1::*)(float)>;

		using T10 = FunctionTraits<void (F1::*)() const>;
		using T11 = FunctionTraits<int (F1::*)() const>;
		using T12 = FunctionTraits<int (F1::*)(float) const>;

		using T13 = FunctionTraits<void (F1::*)() noexcept>;
		using T14 = FunctionTraits<int (F1::*)() noexcept>;
		using T15 = FunctionTraits<int (F1::*)(float) noexcept>;

		using T16 = FunctionTraits<void (F1::*)() const noexcept>;
		using T17 = FunctionTraits<int (F1::*)() const noexcept>;
		using T18 = FunctionTraits<int (F1::*)(float) const noexcept>;

		static_assert(std::is_same_v<T7::ReturnType, void>);
		static_assert(std::is_same_v<T8::ReturnType, int>);
		static_assert(std::is_same_v<T9::ReturnType, int>);
		static_assert(std::is_same_v<T10::ReturnType, void>);
		static_assert(std::is_same_v<T11::ReturnType, int>);
		static_assert(std::is_same_v<T12::ReturnType, int>);
		static_assert(std::is_same_v<T13::ReturnType, void>);
		static_assert(std::is_same_v<T14::ReturnType, int>);
		static_assert(std::is_same_v<T15::ReturnType, int>);
		static_assert(std::is_same_v<T16::ReturnType, void>);
		static_assert(std::is_same_v<T17::ReturnType, int>);
		static_assert(std::is_same_v<T18::ReturnType, int>);

		static_assert(std::is_same_v<T7::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T8::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T9::ArgList, TypeList<float>>);
		static_assert(std::is_same_v<T10::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T11::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T12::ArgList, TypeList<float>>);
		static_assert(std::is_same_v<T13::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T14::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T15::ArgList, TypeList<float>>);
		static_assert(std::is_same_v<T16::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T17::ArgList, TypeList<>>);
		static_assert(std::is_same_v<T18::ArgList, TypeList<float>>);

		static_assert(!T7::IsConst);
		static_assert(!T8::IsConst);
		static_assert(!T9::IsConst);
		static_assert(!T7::IsNoexcept);
		static_assert(!T8::IsNoexcept);
		static_assert(!T9::IsNoexcept);

		static_assert(T10::IsConst);
		static_assert(T11::IsConst);
		static_assert(T12::IsConst);
		static_assert(!T10::IsNoexcept);
		static_assert(!T11::IsNoexcept);
		static_assert(!T12::IsNoexcept);

		static_assert(!T13::IsConst);
		static_assert(!T14::IsConst);
		static_assert(!T15::IsConst);
		static_assert(T13::IsNoexcept);
		static_assert(T14::IsNoexcept);
		static_assert(T15::IsNoexcept);

		static_assert(T16::IsConst);
		static_assert(T17::IsConst);
		static_assert(T18::IsConst);
		static_assert(T16::IsNoexcept);
		static_assert(T17::IsNoexcept);
		static_assert(T18::IsNoexcept);

		static_assert(Function<F1>);
		static_assert(Function<F2>);
		static_assert(Function<F3>);
		static_assert(Function<F4>);

		using L1 = decltype([]() {});
		static_assert(Function<L1>);
		static_assert(std::is_same_v<FunctionTraits<L1>::ReturnType, void>);
		static_assert(std::is_same_v<FunctionTraits<L1>::ArgList, TypeList<>>);
		static_assert(FunctionTraits<L1>::IsConst);
		static_assert(!FunctionTraits<L1>::IsNoexcept);
	} // namespace function_ut
} // namespace apollo::meta