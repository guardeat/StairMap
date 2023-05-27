#ifndef B_STAIRVECTOR_H
#define B_STAIRVECTOR_H

#include <bit>
#include <type_traits>
#include <algorithm>
#include <memory>

#include "type_traits.h"

namespace ByteC
{
	template<typename Type>
	class ArrayContainer
	{
	public:
		using Pointer = Type*;
		using Array = Type*;
		using ContainerData = Array*;
		using Container = std::unique_ptr<Array[]>;

	private:
		Container arrays{nullptr};
		size_t arrayCount{ 0 };

	public:
		ArrayContainer() = default;

		~ArrayContainer()
		{
			clear();
		}

		ArrayContainer(const ArrayContainer& left) = delete;

		ArrayContainer(ArrayContainer&& right) noexcept
		{
			arrays = std::move(right.arrays);
			right.arrayCount = 0;
		}

		ArrayContainer& operator=(const ArrayContainer& left) = delete;

		ArrayContainer& operator=(ArrayContainer&& right) noexcept
		{
			clear();
			arrays = std::move(right.arrays);
			right.arrayCount = 0;

			return *this;
		}

		void pushBack(Array newArray)
		{
			Container newArrays{ std::make_unique<Array[]>(arrayCount + 1)};
			if (arrayCount > 0)
			{
				std::copy(arrays.get(), arrays.get() + arrayCount, newArrays.get());
			}
			newArrays[arrayCount] = newArray;
			++arrayCount;
			arrays.swap(newArrays);
		}

		void popBack()
		{
			--arrayCount;
			if (arrayCount > 0)
			{
				Container newArrays{ std::make_unique<Array[]>(arrayCount)};
				std::copy(arrays.get(), arrays.get() + arrayCount, newArrays.get());
				arrays.swap(newArrays);
			}
			else
			{
				arrays.reset();
			}
		}

		Array at(size_t index)
		{
			return arrays[index];
		}

		const Array at(size_t index) const
		{
			return arrays[index];
		}

		Array back()
		{
			return arrays[arrayCount - 1];
		}

		const Array back() const
		{
			return arrays[arrayCount - 1];
		}

		void clear()
		{
			arrayCount = 0;
		}

		size_t size() const
		{
			return arrayCount;
		}

		ContainerData data()
		{
			return arrays.get();
		}

		const ContainerData data() const
		{
			return arrays.get();
		}
	};

	template<typename Type>
	class StairIterator
	{
	public:
		using Value = Type;
		using Pointer = Type*;
		using Array = Type*;
		using StairArray = std::conditional_t< ByteT::isConst<Type>::value, const Array*, Array*>;

	private:
		StairArray arrays;
		size_t index;

	public:
		StairIterator(StairArray arrays, size_t start)
			:arrays{ arrays }, index{ start }
		{
		}

		Value& operator*()
		{
			size_t arrayIndex{ std::bit_width(index + 2) - 2 };
			return arrays[arrayIndex][index + 2 - (2ULL << arrayIndex)];
		}

		Pointer operator->()
		{
			size_t arrayIndex{ std::bit_width(index + 2) - 2 };
			return arrays[arrayIndex] + (index + 2 - (2ULL << arrayIndex));
		}

		bool operator==(const StairIterator& left) const
		{
			return index == left.index;
		}

		bool operator!=(const StairIterator& left) const
		{
			return index != left.index;
		}

		StairIterator& operator++()
		{
			++index;
			return *this;
		}

		StairIterator operator++(int)
		{
			StairIterator<Type> old{*this};
			++index;
			return old;
		}
	};

	template<typename Type, typename Allocator = std::allocator<Type>>
	class StairVector
	{
	public:
		using Value = Type;
		using Array = Type*;
		using ArrayContainer = ArrayContainer<Type>;

		using AllocatorTraits = std::allocator_traits<Allocator>;

		using Iterator = StairIterator<Type>;
		using ConstIterator = StairIterator<const Type>;
	
	private:
		Allocator allocator{};
		ArrayContainer arrays{};
		size_t itemCount{};

	public:
		StairVector() = default;

		StairVector(const StairVector& left)
			:StairVector{ left.copy() }
		{
		}

		StairVector(StairVector&& right) noexcept:
			allocator{std::move(right.allocator)}, 
			arrays{std::move(right.arrays)}, 
			itemCount{right.itemCount}
		{
			right.itemCount = 0;
		}

		StairVector& operator=(const StairVector& left)
		{
			clear();
			*this = left.copy();
			return *this;
		}

		StairVector& operator=(StairVector&& right) noexcept
		{
			clear();
			allocator = std::move(right.allocator);
			arrays = std::move(right.arrays);
			itemCount = right.itemCount;
			right.itemCount = 0;
			return *this;
		}

		~StairVector()
		{
			clear();
		}

		void pushBack(const Value& value)
		{
			pushBack(Value{ value });
		}

		void pushBack(Value&& value)
		{
			increaseCapacity(itemCount + 1);
			construct(itemCount, std::move(value));
			++itemCount;
		}

		void popBack()
		{
			if constexpr (!std::is_trivially_destructible<Type>())
			{
				destroy(itemCount - 1);
			}
			--itemCount;
			decreaseCapacity(itemCount);
		}

		Value& at(size_t index)
		{
			size_t arrayIndex{ std::bit_width(index + 2) - 2 };
			return arrays.at(arrayIndex)[index + 2 - (2ULL << arrayIndex)];
		}

		const Value& at(size_t index) const
		{
			size_t arrayIndex{ std::bit_width(index + 2) - 2 };
			return arrays.at(arrayIndex)[index + 2 - (2ULL << arrayIndex)];
		}

		Value& operator[](size_t index)
		{
			return at(index);
		}

		const Value& operator[](size_t index) const
		{
			return at(index);
		}

		Value& back()
		{
			return at(itemCount - 1);
		}

		const Value& back() const
		{
			return at(itemCount - 1);
		}

		size_t size() const
		{
			return itemCount;
		}

		size_t capacity() const
		{
			return (2ULL << arrays.size()) - 2;
		}

		Iterator begin()
		{
			return Iterator{ arrays.data(),0 };
		}

		Iterator end()
		{
			return Iterator{ arrays.data(), itemCount };
		}

		ConstIterator begin() const
		{
			return ConstIterator{ arrays.data(), 0};
		}

		ConstIterator end() const
		{
			return ConstIterator{ arrays.data(), itemCount};
		}

		bool empty() const
		{
			return itemCount == 0;
		}

		void clear()
		{
			if constexpr (!std::is_trivially_destructible<Type>())
			{
				for (size_t index{}; index < itemCount; ++index)
				{
					destroy(index);
				}
			}
			decreaseCapacity(0);
			itemCount = 0;
		}

		ArrayContainer& data()
		{
			return arrays;
		}

		const ArrayContainer& data() const
		{
			return arrays;
		}
		
	private:
		StairVector copy() const
		{
			StairVector out;
			out.increaseCapacity(capacity());
			size_t count{ arrays.size() };
			for (const Value& value: *this)
			{
				out.pushBack(value);
			}
			out.itemCount = itemCount;
			return out;
		}

		void construct(size_t index, Value&& value)
		{
			AllocatorTraits::construct(allocator, &at(index), std::move(value));
		}

		void destroy(size_t index)
		{
			AllocatorTraits::destroy(allocator, &at(index));
		}

		void increaseCapacity(size_t newCapacity)
		{
			while (capacity() < newCapacity)
			{
				arrays.pushBack(AllocatorTraits::allocate(allocator, 2LL << arrays.size()));
			}
		}

		void decreaseCapacity(size_t newCapacity)
		{
			if (newCapacity)
			{
				newCapacity += (1ULL << arrays.size());
			}

			while (capacity() > newCapacity)
			{
				AllocatorTraits::deallocate(allocator, arrays.back(), (1ULL << arrays.size()));
				arrays.popBack();
			}
		}

	};
}

#endif