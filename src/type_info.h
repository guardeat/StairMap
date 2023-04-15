#ifndef TYPETRAITS_H
#define TYPETRAITS_H

#include <string>

namespace ByteT
{

	template<typename Type>
	struct isConst
	{
		inline static constexpr bool value{ false };
	};

	template<typename Type>
	struct isConst<const Type>
	{
		inline static constexpr bool value{ true };
	};

	template<typename Type>
	inline std::string constexpr typeName()
	{
		return typeid(Type).name();
	}
}

#endif