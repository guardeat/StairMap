#ifndef TYPETRAITS_H
#define TYPETRAITS_H

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

}

#endif