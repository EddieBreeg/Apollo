#pragma once

namespace brk {
	template<class T>
	struct UnassignedT;
	
	template<class T>
	struct UnassignedT<T*>
	{
		static constexpr T* Value = nullptr;
	};

	template<class T>
	static constexpr T Unassigned = UnassignedT<T>::Value;
}