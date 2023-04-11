#ifndef STAIRMAP_H
#define	STAIRMAP_H

#include <memory>
#include <utility>

#include "StairVector.h"
#include "Hash.h"

namespace ByteC
{
	template<typename Value>
	struct Node
	{
		using NextPointer = Node*;
		using Pair = std::pair<const size_t,Value>;

		Pair pair;
		NextPointer next{nullptr};

		Node(size_t hash, Value&& value)
			:pair{ hash,std::move(value) }
		{
		}
	};

	template<typename Value>
	class Chain
	{
	public:
		using NodePointer = Node<Value>*;

	private:
		NodePointer head{};

	public:
		Chain() = default;

		void pushFront(NodePointer node)
		{
			node->next = head;
			head = node;
		}

		NodePointer find(size_t hashKey)
		{
			NodePointer iterator{ head };
			while (iterator)
			{
				if (iterator->pair.first == hashKey)
				{
					return iterator;
				}
				iterator = iterator->next;
			}
			return nullptr;
		}

		NodePointer remove(size_t hashKey)
		{
			if (head->pair.first == hashKey)
			{
				NodePointer out{ head };
				head = head->next;
				return out;
			}

			NodePointer iterator{ head };
			while (iterator->next)
			{
				if (iterator->next->pair.first == hashKey)
				{
					NodePointer out{ iterator->next };
					iterator->next = out->next;
					return out;
				}
				iterator = iterator->next;
			}
			return nullptr;
		}
	};

	template<typename Type>
	class MapIterator
	{
	public:
		using Pair = std::pair<const size_t, Type>;
		using Node = Node<Type>;
		using Value = std::conditional_t<isConst<Type>::value, const Node, Node >;
		using Pointer = std::conditional_t<isConst<Type>::value, const Node*, Node* >;
		using Array = std::conditional_t< isConst<Type>::value, const Node*, Node* >;
		using StairArray = std::conditional_t< isConst<Type>::value, const Array*, Array*>;

	private:
		StairArray arrays;
		size_t index;

	public:
		MapIterator(StairArray arrays, size_t start)
			:arrays{ arrays }, index{ start }
		{
		}

		Pair& operator*()
		{
			size_t arrayIndex{ std::bit_width(index + 2) - 2 };
			return arrays[arrayIndex][index + 2 - (2ULL << arrayIndex)].pair;
		}

		Pointer operator->()
		{
			size_t arrayIndex{ std::bit_width(index + 2) - 2 };
			return &arrays[arrayIndex][index + 2 - (2ULL << arrayIndex)].pair;
		}

		bool operator==(const MapIterator& left) const
		{
			return index == left.index;
		}

		bool operator!=(const MapIterator& left) const
		{
			return index != left.index;
		}

		MapIterator& operator++()
		{
			++index;
			return *this;
		}

		MapIterator operator++(int)
		{
			MapIterator<Type> old{ *this };
			++index;
			return old;
		}
	};

	template<
		typename Key,
		typename Value,
		typename Hash = ByteA::Hash<Key>,
		typename Allocator = std::allocator<Node<Value>>>
	class StairMap
	{
	public:
		using Bucket = Chain<Value>;
		using Node = Node<Value>;
		using NodePointer = Node*;

		using BucketArray = std::vector<Bucket>;
		using NodeArray = StairVector<Node, Allocator>;
		
		using Iterator = MapIterator<Value>;
		using ConstIterator = MapIterator<const Value>;

		inline static constexpr double MAX_LOAD{ 0.9 };
		inline static constexpr double MIN_LOAD{ 0.1 };
	
	private:
		Hash hasher;
		NodeArray nodeArray;
		BucketArray bucketArray;

	public:
		StairMap(size_t tableSize = 2)
			:bucketArray{ tableSize }
		{
		}

		StairMap(const StairMap& left)
			:nodeArray{left.nodeArray}
		{
			rehash(left.tableSize());
		}

		StairMap(StairMap&& right) noexcept = default;

		StairMap& operator=(const StairMap& left)
		{
			nodeArray = left.nodeArray;
			rehash(left.tableSize());
			return *this;
		}

		StairMap& operator=(StairMap&& right) noexcept = default;

		~StairMap() = default;
		
		void insert(const Key& key, const Value& value)
		{
			insert(key,Value{value});
		}

		void insert(const Key& key, Value&& value)
		{
			size_t hashValue{ hasher(key) };
			nodeArray.pushBack(Node{ hashValue,std::move(value) });
			bucketArray[hashValue % bucketArray.size()].pushFront(&nodeArray.back());
			checkLoad();
		}

		Value& at(const Key& key)
		{
			size_t hashValue{ hasher(key) };
			return bucketArray[hashValue % tableSize()].find(hashValue)->pair.second;
		}

		void erase(const Key& key)
		{
			NodePointer left{ bucketArray[hasher(key) % tableSize()].remove(key)};
			NodePointer right{ &nodeArray.back() };
			std::swap(left, right);
			nodeArray.popBack();
		}

		NodePointer find(const Key& key)
		{
			size_t hashValue{ hasher(key) };
			return bucketArray[hashValue % tableSize()].find(hashValue);
		}

		bool contains(const Key& key) const
		{
			return static_cast<bool>(bucketArray[hasher(key) % tableSize()].find(key));
		}

		Iterator begin()
		{
			return Iterator{ nodeArray.data().data(),0 };
		}

		Iterator end()
		{
			return Iterator{ nodeArray.data().data(),nodeArray.size() };
		}

		ConstIterator begin() const
		{
			return ConstIterator{ nodeArray.data().data(),0 };
		}

		ConstIterator end() const
		{
			return ConstIterator{ nodeArray.data().data(),nodeArray.size() };
		}

		size_t size() const
		{
			return nodeArray.size();
		}

		size_t tableSize() const
		{
			return bucketArray.size();
		}

		void rehash(size_t newSize)
		{
			BucketArray newBuckets{ newSize };
			for (Node& node : nodeArray)
			{
				size_t newPosition{ node.pair.first % newSize };
				newBuckets[newPosition].pushFront(&node);
			}
			bucketArray = std::move(newBuckets);
		}

		void clear()
		{
			bucketArray.clear();
			nodeArray.clear();
		}

	private:
		void checkLoad()
		{
			double load{ nodeArray.size() / static_cast<double>(bucketArray.size()) };
			if (load > MAX_LOAD)
			{
				rehash(bucketArray.size() * 2);
			}
			else if (load < MIN_LOAD)
			{
				rehash(bucketArray.size() / 2);
			}
		}

	};
}

#endif