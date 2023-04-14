#ifndef STAIRMAP_H
#define	STAIRMAP_H

#include <memory>
#include <utility>
#include <vector>

#include "StairVector.h"
#include "Hash.h"
#include "TypeTraits.h"

namespace ByteC
{
	template<typename Key,typename Value>
	struct Node
	{
		using NextPointer = Node*;
		using Pair = std::pair<const Key, Value>;

		Pair pair;

		NextPointer next{ nullptr };
		const size_t hash;

		Node(Key&& key, Value&& value, size_t hash)
			:pair{ std::move(key),std::move(value) }, hash{hash}
		{
		}
	};

	template<typename Key, typename Value>
	class Chain
	{
	public:
		using NodePointer = Node<Key,Value>*;

	private:
		NodePointer head{};

	public:
		Chain() = default;

		void pushFront(NodePointer node)
		{
			node->next = head;
			head = node;
		}

		NodePointer find(const Key& key, size_t hashKey)
		{
			NodePointer iterator{ head };
			while (iterator)
			{
				if (iterator->hash == hashKey && iterator->pair.first == key)
				{
					return iterator;
				}
				iterator = iterator->next;
			}
			return nullptr;
		}

		const NodePointer find(const Key& key, size_t hashKey) const
		{
			NodePointer iterator{ head };
			while (iterator)
			{
				if (iterator->hash == hashKey && iterator->pair.first == key)
				{
					return iterator;
				}
				iterator = iterator->next;
			}
			return nullptr;
		}

		const bool contains(const Key& key, size_t hashKey) const
		{
			return find(key, hashKey) != nullptr;
		}

		NodePointer remove(const Key& key, size_t hashKey)
		{
			if (head->hash == hashKey && head->pair->first == key)
			{
				NodePointer out{ head };
				head = head->next;
				return out;
			}

			NodePointer iterator{ head };
			while (iterator->next)
			{
				if (iterator->hash == hashKey && iterator->pair->first == key)
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

	template<typename Key, typename Value>
	class MapIterator
	{
	public:
		using Pair = std::pair<const Key, Value>;
		using Node = std::conditional_t< ByteT::isConst<Value>::value, const Node<Key,Value>, Node<Key,Value> >;
		using Pointer = Node*;
		using Array = Node*;
		using StairArray = std::conditional_t< ByteT::isConst<Value>::value, const Array*, Array*>;

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
			MapIterator<Key,Value> old{ *this };
			++index;
			return old;
		}
	};

	template<typename Key, typename Value>
	class SearchResult
	{
	public:
		using Node = std::conditional_t<ByteT::isConst<Value>::value, const Node<Key, Value>, Node<Key, Value>>;
		using NodePointer = Node*;

		using Pair = std::pair<const Key, Value>;
		using PairPointer = Pair*;

	private:
		NodePointer result;

	public:
		SearchResult(NodePointer result)
			:result{ result }
		{
		}

		Value& operator*()
		{
			return result->pair.second;
		}

		PairPointer operator->()
		{
			return &result->pair;
		}

		bool isValid()
		{
			return result != nullptr;
		}
	};

	template<
		typename Key,
		typename Value,
		typename Hash = ByteA::Hash<Key>,
		typename Allocator = std::allocator<Node<Key,Value>>>
	class StairMap
	{
	public:
		using Bucket = Chain<Key,Value>;
		using Node = Node<Key,Value>;
		using NodePointer = Node*;

		using BucketArray = std::vector<Bucket>;
		using NodeArray = StairVector<Node, Allocator>;

		using Iterator = MapIterator<Key,Value>;
		using ConstIterator = MapIterator<Key,const Value>;

		using Result = SearchResult<Key, Value>;
		using ConstResult = SearchResult<Key, const Value>;

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
			:nodeArray{ left.nodeArray }
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
			insert(Key{ key }, Value{ value });
		}

		void insert(const Key& key, Value&& value)
		{
			insert(Key{ key }, std::move(value));
		}

		void insert(Key&& key, const Value& value)
		{
			insert(std::move(key), Value{value});
		}

		void insert(Key&& key, Value&& value)
		{
			size_t hashValue{ hasher(key) };
			nodeArray.pushBack(Node{ std::move(key),std::move(value), hashValue });
			bucketArray[hashValue % bucketArray.size()].pushFront(&nodeArray.back());
			checkLoad();
		}

		Value& at(const Key& key)
		{
			size_t hashValue{ hasher(key) };
			return bucketArray[hashValue % tableSize()].find(key,hashValue)->pair.second;
		}

		const Value& at(const Key& key) const
		{
			size_t hashValue{ hasher(key) };
			return bucketArray.at(hashValue % tableSize()).find(key, hashValue)->pair.second;
		}

		[[nodiscard]] Value& operator[](const Key& key)
		{
			size_t hashValue{ hasher(key) };
			NodePointer out{ bucketArray[hashValue % tableSize()].find(key,hashValue) };
			if (out)
			{
				return out->pair.second;
			}

			nodeArray.pushBack(Node{ Key{key}, Value{}, hashValue});
			out = &nodeArray.back();
			bucketArray[hashValue % bucketArray.size()].pushFront(out);

			checkLoad();

			return out->pair.second;
		}

		const Value& operator[](const Key& key) const
		{
			return at(key);
		}

		void erase(const Key& key)
		{
			size_t hashValue{ hasher(key) };
			NodePointer left{ bucketArray[hashValue % tableSize()].remove(key, hashValue) };
			NodePointer right{ &nodeArray.back() };
			std::swap(left, right);
			nodeArray.popBack();
		}

		Result find(const Key& key)
		{
			size_t hashValue{ hasher(key) };
			return bucketArray[hashValue % tableSize()].find(key,hashValue);
		}

		const ConstResult find(const Key& key) const
		{
			size_t hashValue{ hasher(key) };
			return bucketArray.at(hashValue % tableSize()).find(key, hashValue);
		}

		bool contains(const Key& key) const
		{
			size_t hashValue{hasher(key)};
			return bucketArray[hashValue % tableSize()].contains(key,hashValue);
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
				size_t newPosition{ node.hash % newSize };
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