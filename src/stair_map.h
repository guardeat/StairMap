#ifndef STAIRMAP_H
#define	STAIRMAP_H

#include <memory>
#include <utility>
#include <vector>

#include "stair_vector.h"
#include "hash.h"
#include "type_traits.h"

namespace ByteC
{
	template<typename Key,typename Type>
	struct MapNode
	{
		using Value = Type;

		using NextPointer = MapNode*;

		using Pair = std::pair<const Key, Value>;

		Pair pair;

		NextPointer next{ nullptr };
		const size_t hash;

		MapNode(Key&& key, Value&& value, size_t hash)
			:pair{ std::move(key),std::move(value) }, hash{hash}
		{
		}
	};

	template<typename Key, typename Type>
	class Chain
	{
	public:
		using NodePointer = MapNode<Key,Type>*;

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
			return findBase(key, hashKey);
		}

		const NodePointer find(const Key& key, size_t hashKey) const
		{
			return findBase(key, hashKey);
		}

		const bool contains(const Key& key, size_t hashKey) const
		{
			return find(key, hashKey) != nullptr;
		}

		NodePointer remove(const Key& key, size_t hashKey)
		{
			if (head->hash == hashKey && head->pair.first == key)
			{
				NodePointer out{ head };
				head = head->next;
				return out;
			}

			NodePointer iterator{ head };
			while (iterator->next)
			{
				if (iterator->hash == hashKey && iterator->pair.first == key)
				{
					NodePointer out{ iterator->next };
					iterator->next = out->next;
					return out;
				}
				iterator = iterator->next;
			}
			return nullptr;
		}

	private:
		NodePointer findBase(const Key& key, size_t hashKey) const
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
	};

	template<typename Key, typename Type>
	class MapIterator
	{
	public:
		using Value = Type;

		using Pair = std::pair<const Key, Type>;
		using PairReference = Pair&;
		using PairPointer = Pair*;

		using Node = MapNode<Key,Value>;
		using NodePointer = Node*;

		using StairVector = ByteC::StairVector<Node>;
		using StairVectorPointer = StairVector*;

	private:
		StairVectorPointer arrays;
		size_t index;

	public:
		MapIterator(StairVector& arrays, size_t start)
			:arrays{ &arrays }, index{ start }
		{
		}

		PairReference operator*()
		{
			return arrays->at(index).pair;
		}

		PairPointer operator->()
		{
			return &arrays->at(index).pair;
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

	template<typename Key, typename Type>
	class ConstMapIterator
	{
	public:
		using Value = Type;

		using Pair = std::pair<const Key, Type>;
		using PairReference = const Pair&;
		using PairPointer = const Pair*;

		using Node = MapNode<Key, typename std::remove_const<Value>::type>;
		using NodePointer = Node*;

		using StairVector = const ByteC::StairVector<Node>;
		using StairVectorPointer = StairVector*;

	private:
		StairVectorPointer arrays;
		size_t index;

	public:
		ConstMapIterator(StairVector& arrays, size_t start)
			:arrays{ &arrays }, index{ start }
		{
		}

		PairReference operator*()
		{
			return arrays->at(index).pair;
		}

		PairPointer operator->()
		{
			return &arrays->at(index).pair;
		}

		bool operator==(const ConstMapIterator& left) const
		{
			return index == left.index;
		}

		bool operator!=(const ConstMapIterator& left) const
		{
			return index != left.index;
		}

		ConstMapIterator& operator++()
		{
			++index;
			return *this;
		}

		ConstMapIterator operator++(int)
		{
			MapIterator<Key, Value> old{ *this };
			++index;
			return old;
		}
	};

	template<typename Key, typename Type>
	class SearchResult
	{
	public:
		using Value = Type;

		using Node = MapNode<Key, typename std::remove_const<Value>::type>;
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

		const Value& operator*() const
		{
			return result->pair.second;
		}

		PairPointer operator->()
		{
			return &result->pair;
		}

		const PairPointer operator->() const
		{
			return &result->pair;
		}

		bool valid()
		{
			return result != nullptr;
		}
	};

	template<
		typename Key,
		typename Type,
		typename Hash = ByteA::Hash<Key>,
		typename Allocator = std::allocator<MapNode<Key,Type>>>
	class StairMap
	{
	public:
		using Value = Type;

		using Bucket = Chain<Key,Value>;
		using Node = MapNode<Key,Value>;
		using NodePointer = Node*;

		using BucketArray = std::vector<Bucket>;
		using NodeArray = StairVector<Node, Allocator>;

		using Iterator = MapIterator<Key,Value>;
		using ConstIterator = ConstMapIterator<Key,Value>;

		using Result = SearchResult<Key, Value>;
		using ConstResult = const SearchResult<Key, const Value>;

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
			return Iterator{ nodeArray,0};
		}

		Iterator end()
		{
			return Iterator{ nodeArray,nodeArray.size()};
		}

		ConstIterator begin() const
		{
			return ConstIterator{ nodeArray,0 };
		}

		ConstIterator end() const
		{
			return ConstIterator{ nodeArray,nodeArray.size() };
		}

		size_t size() const
		{
			return nodeArray.size();
		}

		bool empty() const
		{
			return nodeArray.empty();
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