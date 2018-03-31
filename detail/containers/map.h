#ifndef AF_DETAIL_CONTAINERS_MAP_H
#define AF_DETAIL_CONTAINERS_MAP_H


#include "AF/assert.h"
#include "AF/basic_types.h"
#include "AF/defines.h"

#include "AF/detail/containers/list.h"


namespace AF
{
namespace Detail
{

/*
 * Generic map container mapping unique keys to values.
 * Supports multiple values with the same key (ie. multimap semantics).
 */
template <class KeyType, class ValueType, class HashType>
class Map {
public:
    /*
     * Node type, which is allocated and owned by the user.
     */
    struct Node : public List<Node>::Node {
        AF_FORCEINLINE Node(const KeyType &key, const ValueType &value) 
          : key_(key), value_(value) {
        }

        KeyType key_;
        ValueType value_;
    };

    // TODO: Make this private.
    struct Bucket : public List<Bucket>::Node {
        List<typename Map::Node> node_list_;
    };

    /*
     * Iterates the nodes with a given key, of which there can be any number.
     * Each node contains the key (redundantly, in this use) and the associated value.
     */
    class KeyNodeIterator {
    public:
        AF_FORCEINLINE KeyNodeIterator() 
          : node_list_(0),
            key_(0),
            it_() {
        }

        AF_FORCEINLINE KeyNodeIterator(const Map *const map, const KeyType &key) 
          : node_list_(&map->buckets_[HashType::Compute(key)].node_list_),
            key_(key),
            it_(node_list_->GetIterator()) {
        }

         // Moves to the next enumerated entry, returning false if no further entries exist.
         // This method must be called once before calling GetValue.
        AF_FORCEINLINE bool Next() {
            AF_ASSERT(node_list_);

            if (!it_.Next()) {
                return false;
            }

            // Skip nodes with different keys.
            while (it_.Get()->key_ != key_) {
                if (!it_.Next()) {
                    return false;
                }
            }

            return true;
        }

        AF_FORCEINLINE Node *Get() const {
            return it_.Get();
        }

    private:
        const List<Node> *node_list_;
        KeyType key_;
        typename List<Node>::Iterator it_;
    };

    friend class NodeIterator;
    friend class KeyNodeIterator;

    inline Map();

    inline ~Map();

    // Inserts a user-allocated node into the map.
    // Duplicates are permitted and are not checked for.
    // return True, if the node was successfully inserted.
    inline bool Insert(Node *const node);

     // Removes a node from the map, if it is present.
     // return True, if the node was successfully removed.
    inline bool Remove(Node *const node);

     // Returns true if the map contains one or more inserted nodes with the given key.
    inline bool Contains(const KeyType &key) const;

     // Searches the map for an inserted node with the given key and value.
    inline Node *Find(const KeyType &key, const ValueType &value) const;

     // Returns a pointer to the node which is arbitrarily the first node in the map.
     // Returns null if the map is empty.
    inline Node *Front() const;

     // Returns an iterator enumerating the inserted nodes whose keys match the given key.
    inline KeyNodeIterator GetKeyNodeIterator(const KeyType &key) const;

private:
    Bucket buckets_[HashType::RANGE];
    List<Bucket> non_empty_buckets_;
};


template <class KeyType, class ValueType, class HashType>
inline Map<KeyType, ValueType, HashType>::Map() : non_empty_buckets_() {
}

template <class KeyType, class ValueType, class HashType>
inline Map<KeyType, ValueType, HashType>::~Map() {
    // Check the map has been emptied otherwise we'll leak the node memory.
    AF_ASSERT(non_empty_buckets_.Empty());
}

template <class KeyType, class ValueType, class HashType>
AF_FORCEINLINE bool Map<KeyType, ValueType, HashType>::Insert(Node *const node) {
    const uint32_t hash(HashType::Compute(node->key_));
    AF_ASSERT(hash < HashType::RANGE);
    Bucket &bucket(buckets_[hash]);

    if (bucket.node_list_.Empty()) {
        non_empty_buckets_.Insert(&bucket);
    }

    bucket.node_list_.Insert(node);
    return true;
}

template <class KeyType, class ValueType, class HashType>
AF_FORCEINLINE bool Map<KeyType, ValueType, HashType>::Remove(Node *const node) {
    const uint32_t hash(HashType::Compute(node->key_));
    AF_ASSERT(hash < HashType::RANGE);
    Bucket &bucket(buckets_[hash]);

    if (bucket.node_list_.Remove(node)) {
        if (bucket.node_list_.Empty()) {
            non_empty_buckets_.Remove(&bucket);
        }

        return true;
    }

    return false;
}

template <class KeyType, class ValueType, class HashType>
AF_FORCEINLINE bool Map<KeyType, ValueType, HashType>::Contains(const KeyType &key) const {
    KeyNodeIterator key_nodes(GetKeyNodeIterator(key));
    return key_nodes.Next();
}

template <class KeyType, class ValueType, class HashType>
AF_FORCEINLINE typename Map<KeyType, ValueType, HashType>::Node *Map<KeyType, ValueType, HashType>::Find(
    const KeyType &key,
    const ValueType &value) const {
    const uint32_t hash(HashType::Compute(key));
    AF_ASSERT(hash < HashType::RANGE);
    const Bucket &bucket(buckets_[hash]);

    typename List<Node>::Iterator nodes(bucket.node_list_.GetIterator());
    while (nodes.Next()) {
        Node *const node(nodes.Get());
        if (node->key_ == key && node->value_ == value) {
            return node;
        }
    }

    return 0;
}

template <class KeyType, class ValueType, class HashType>
AF_FORCEINLINE typename Map<KeyType, ValueType, HashType>::Node *Map<KeyType, ValueType, HashType>::Front() const {
    // Return the front node from the first non-empty bucket.
    if (non_empty_buckets_.Empty()) {
        return 0;
    }

    const Bucket *const bucket(non_empty_buckets_.Front());
    
    AF_ASSERT(!bucket->node_list_.Empty());
    return bucket->node_list_.Front();
}

template <class KeyType, class ValueType, class HashType>
inline typename Map<KeyType, ValueType, HashType>::KeyNodeIterator Map<KeyType, ValueType, HashType>::GetKeyNodeIterator(const KeyType &key) const {
    return KeyNodeIterator(this, key);
}


} // namespace Detail
} // namespace AF


#endif // AF_DETAIL_CONTAINERS_MAP_H

