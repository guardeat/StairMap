#ifndef STAIRVECTOR_H
#define STAIRVECTOR_H

#include <vector>
#include <bit>
#include <type_traits>
#include <algorithm>
#include <memory>

#include "type_traits.h"

namespace ByteC
{

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
		using ArrayContainer = std::vector<Array>;

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
			return arrays[arrayIndex][index + 2 - (2ULL << arrayIndex)];
		}

		const Value& at(size_t index) const
		{
			return at(index);
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
				arrays.push_back(AllocatorTraits::allocate(allocator, 2LL << arrays.size()));
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
				arrays.pop_back();
			}
		}

	};
}

#endif